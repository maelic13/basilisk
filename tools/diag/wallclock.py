#!/usr/bin/env python3
"""Wall-clock overhead read (PLAN B.16.1, Triumviratus TT-2).

What a clocked move costs outside the search, timed from outside the engine
the way a GUI sees it. Every engine runs as one process per Threads value,
driven over stdin/stdout; the host's QPC clock (time.perf_counter) stamps the
write of each command and the arrival of each line.

Three trials, each repeated over a fixed position sample:

  nodes      `go nodes N`. wall = go written -> bestmove read; the engine's
             own report is the `time` field of its last info line; the
             overhead is wall - report. At N = 1000 the search is a fraction
             of a millisecond, so wall itself is almost all overhead.
  movetime   `go movetime M` for each M. overshoot = wall - M.
  ponderhit  `go ponder` on a short clock, then `ponderhit` after a wait
             longer than any budget the clock allows, so the search has
             nothing left to spend. latency = ponderhit written -> bestmove
             read. A bestmove before the ponderhit is counted as early.

Trials alternate between the engines in every round (the order flips each
round), so host drift falls on every engine alike. Positions, the `isready`
before each trial and process start-up are outside every timing.

The archive is <out>/runs.jsonl (one line per trial) and <out>/summary.json;
the summary is printed as a table and recomputed from the archive with
`--report <out>`. The decision quantities are the medians; each is compared
with the reference engine's (`--reference`), Basilisk minus reference.

Engines are NAME=PATH; a .py path runs under this interpreter (the planted
case, tools/diag/wallclock_fake_engine.py). Per-engine UCI options are
NAME:Option=Value (repeatable); Threads and Hash are set by the tool.

Example:
  python tools/diag/wallclock.py --engine basilisk=B.exe --engine sf=SF.exe \\
      --reference sf --threads 1,4 --cpus 2,4,6,8 --out tools/results/wallclock
"""
from __future__ import annotations

import argparse
import ctypes
import datetime
import json
import os
import platform
import queue
import statistics
import subprocess
import sys
import threading
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_BOOK = ROOT / "tools" / "books" / "UHO_Lichess_4852_v1.epd"
HIGH_PRIORITY_CLASS = 0x00000080


class EngineError(Exception):
    pass


def now_ms() -> float:
    return time.perf_counter() * 1000.0


class Engine:
    """One UCI process. A pump thread stamps every line on arrival, so the
    pipe never fills and a stamp never waits for the main thread."""

    def __init__(self, name: str, path: str, options: list[tuple[str, str]],
                 threads: int, hash_mb: int, cpus: list[int]):
        self.name = name
        argv = [sys.executable, "-I", path] if path.endswith(".py") else [path]
        self.p = subprocess.Popen(argv, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                  stderr=subprocess.DEVNULL, text=True, bufsize=1)
        self.pinned = pin(self.p, cpus[:threads]) if cpus else None
        self.q: queue.Queue[tuple[float, str | None]] = queue.Queue()
        threading.Thread(target=self._pump, daemon=True).start()
        self.send("uci")
        self.ident = ""
        for _, line in self.lines_until("uciok", 20.0):
            if line.startswith("id name "):
                self.ident = line[8:]
        for key, value in [("Threads", str(threads)), ("Hash", str(hash_mb))] + options:
            self.send(f"setoption name {key} value {value}")
        self.send("ucinewgame")
        self.ready()

    def _pump(self) -> None:
        assert self.p.stdout
        for line in self.p.stdout:
            self.q.put((now_ms(), line.strip()))
        self.q.put((now_ms(), None))

    def send(self, line: str) -> float:
        """Write a command; return the stamp taken just before the write."""
        assert self.p.stdin
        stamp = now_ms()
        try:
            self.p.stdin.write(line + "\n")
            self.p.stdin.flush()
        except OSError as exc:
            raise EngineError(f"{self.name}: write failed: {exc}") from exc
        return stamp

    def lines_until(self, prefix: str, seconds: float) -> list[tuple[float, str]]:
        """Every line up to and including the first starting with prefix."""
        out: list[tuple[float, str]] = []
        end = time.monotonic() + seconds
        while True:
            left = end - time.monotonic()
            if left <= 0:
                raise EngineError(f"{self.name}: no '{prefix}' within {seconds} s")
            try:
                stamp, line = self.q.get(timeout=left)
            except queue.Empty:
                continue
            if line is None:
                raise EngineError(f"{self.name}: process ended waiting for '{prefix}'")
            out.append((stamp, line))
            if line.startswith(prefix):
                return out

    def drain(self) -> list[tuple[float, str]]:
        out = []
        while True:
            try:
                stamp, line = self.q.get_nowait()
            except queue.Empty:
                return out
            if line is None:
                raise EngineError(f"{self.name}: process ended")
            out.append((stamp, line))

    def ready(self) -> None:
        self.send("isready")
        self.lines_until("readyok", 60.0)

    def close(self) -> None:
        try:
            self.send("quit")
            self.p.wait(5)
        except (EngineError, subprocess.TimeoutExpired):
            self.p.kill()


def pin(proc: subprocess.Popen, cpus: list[int]) -> dict | None:
    """Pin the process to the given logical CPUs at high priority (Windows)."""
    if os.name != "nt":
        return None
    mask = 0
    for cpu in cpus:
        mask |= 1 << cpu
    kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
    kernel32.SetProcessAffinityMask.argtypes = [ctypes.c_void_p, ctypes.c_size_t]
    kernel32.SetPriorityClass.argtypes = [ctypes.c_void_p, ctypes.c_uint32]
    handle = ctypes.c_void_p(int(proc._handle))  # type: ignore[attr-defined]
    if not kernel32.SetProcessAffinityMask(handle, mask):
        raise EngineError(f"SetProcessAffinityMask failed: {ctypes.get_last_error()}")
    if not kernel32.SetPriorityClass(handle, HIGH_PRIORITY_CLASS):
        raise EngineError(f"SetPriorityClass failed: {ctypes.get_last_error()}")
    return {"cpus": cpus, "mask": hex(mask), "priority": "high"}


def last_info_field(lines: list[tuple[float, str]], field: str) -> int | None:
    for _, line in reversed(lines):
        if not line.startswith("info "):
            continue
        tokens = line.split()
        if field in tokens:
            i = tokens.index(field)
            if i + 1 < len(tokens):
                try:
                    return int(tokens[i + 1])
                except ValueError:
                    return None
    return None


def bestmove_of(lines: list[tuple[float, str]]) -> tuple[float, str]:
    stamp, line = lines[-1]
    return stamp, line.split()[1] if len(line.split()) > 1 else ""


def set_position(engine: Engine, fen: str) -> None:
    engine.drain()
    engine.send(f"position fen {fen}")
    engine.ready()


def trial_nodes(engine: Engine, fen: str, nodes: int) -> dict:
    set_position(engine, fen)
    t0 = engine.send(f"go nodes {nodes}")
    lines = engine.lines_until("bestmove", 60.0)
    t1, move = bestmove_of(lines)
    reported = last_info_field(lines, "time")
    wall = t1 - t0
    return {"wall_ms": wall, "reported_ms": reported,
            "overhead_ms": None if reported is None else wall - reported,
            "nodes": last_info_field(lines, "nodes"), "bestmove": move}


def trial_movetime(engine: Engine, fen: str, movetime: int) -> dict:
    set_position(engine, fen)
    t0 = engine.send(f"go movetime {movetime}")
    lines = engine.lines_until("bestmove", 60.0 + movetime / 1000.0)
    t1, move = bestmove_of(lines)
    wall = t1 - t0
    return {"wall_ms": wall, "overshoot_ms": wall - movetime,
            "reported_ms": last_info_field(lines, "time"), "bestmove": move}


def trial_ponderhit(engine: Engine, fen: str, clock_ms: int, inc_ms: int,
                    wait_ms: int) -> dict:
    set_position(engine, fen)
    t0 = engine.send(f"go ponder wtime {clock_ms} btime {clock_ms} "
                     f"winc {inc_ms} binc {inc_ms}")
    time.sleep(wait_ms / 1000.0)
    early = [l for _, l in engine.drain() if l.startswith("bestmove")]
    if early:
        return {"early": True, "latency_ms": None, "bestmove": early[0].split()[1]}
    t_hit = engine.send("ponderhit")
    lines = engine.lines_until("bestmove", 60.0)
    t1, move = bestmove_of(lines)
    return {"early": False, "latency_ms": t1 - t_hit, "since_go_ms": t1 - t0,
            "bestmove": move}


def read_positions(path: Path, count: int) -> list[str]:
    import chess  # python-chess, already a harness dependency

    with open(path, encoding="utf-8") as handle:
        epds = [l.strip() for l in handle if l.strip()]
    if count > len(epds):
        raise SystemExit(f"{path} has {len(epds)} positions, fewer than {count}")
    step = len(epds) // count
    fens = []
    for i in range(count):
        fields = epds[i * step].split()
        # A full FEN, or an EPD (four fields, then opcodes).
        fen = " ".join(fields[:6]) if len(fields) >= 6 and fields[4].isdigit() \
            else " ".join(fields[:4]) + " 0 1"
        fens.append(chess.Board(fen).fen())  # validates
    return fens


def parse_engine_options(specs: list[str], names: list[str]) -> dict[str, list[tuple[str, str]]]:
    result: dict[str, list[tuple[str, str]]] = {n: [] for n in names}
    for spec in specs:
        name, _, rest = spec.partition(":")
        key, sep, value = rest.partition("=")
        if name not in result or not sep or not key:
            raise SystemExit(f"bad --option '{spec}' (expected NAME:Option=Value)")
        result[name].append((key, value))
    return result


def quantiles(values: list[float]) -> dict:
    if not values:
        return {"n": 0}
    ordered = sorted(values)
    p90 = ordered[min(len(ordered) - 1, int(0.9 * len(ordered)))]
    return {"n": len(values), "median": statistics.median(values),
            "mean": statistics.fmean(values), "p90": p90,
            "min": ordered[0], "max": ordered[-1]}


METRICS = (("nodes", "overhead_ms"), ("nodes", "wall_ms"),
           ("movetime", "overshoot_ms"), ("ponderhit", "latency_ms"))


def summarise(runs: list[dict], reference: str | None) -> dict:
    cells: dict[str, dict] = {}
    for kind, metric in METRICS:
        for run in runs:
            if run["kind"] != kind:
                continue
            key = f"{run['threads']}T {kind}"
            if kind == "movetime":
                key += f" {run['param']}"
            label = f"{key} {metric}"
            cell = cells.setdefault(label, {})
            if run.get(metric) is not None:
                cell.setdefault(run["engine"], []).append(run[metric])
    summary: dict[str, dict] = {}
    for label, by_engine in cells.items():
        row = {name: quantiles(values) for name, values in by_engine.items()}
        if reference and reference in row:
            ref = row[reference].get("median")
            for name, stats in row.items():
                if name != reference and stats.get("n") and ref is not None:
                    stats["median_minus_reference"] = stats["median"] - ref
        summary[label] = row
    early = {}
    for run in runs:
        if run["kind"] == "ponderhit" and run.get("early"):
            key = f"{run['threads']}T {run['engine']}"
            early[key] = early.get(key, 0) + 1
    missing = sum(1 for r in runs if r["kind"] == "nodes" and r.get("reported_ms") is None)
    return {"cells": summary, "early_bestmoves": early, "nodes_without_report": missing}


def print_summary(summary: dict, reference: str | None) -> None:
    print(f"{'cell':44} {'engine':12} {'n':>4} {'median':>8} {'p90':>8} "
          f"{'mean':>8} {'max':>8} {'-ref':>7}")
    for label, row in summary["cells"].items():
        for name, s in row.items():
            if not s.get("n"):
                continue
            gap = s.get("median_minus_reference")
            gap_text = f"{gap:+7.2f}" if gap is not None else ""
            print(f"{label:44} {name:12} {s['n']:4d} {s['median']:8.2f} {s['p90']:8.2f} "
                  f"{s['mean']:8.2f} {s['max']:8.2f} {gap_text:>7}")
    print(f"early bestmoves (ponder): {summary['early_bestmoves'] or 'none'}")
    print(f"nodes trials without an info time: {summary['nodes_without_report']}")
    if reference:
        print(f"reference engine: {reference}")


def report(out: Path) -> int:
    meta = json.loads((out / "summary.json").read_text(encoding="utf-8"))
    with open(out / "runs.jsonl", encoding="utf-8") as handle:
        runs = [json.loads(l) for l in handle if l.strip()]
    summary = summarise(runs, meta.get("reference"))
    print_summary(summary, meta.get("reference"))
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--engine", action="append", default=[], help="NAME=PATH")
    parser.add_argument("--option", action="append", default=[], help="NAME:Option=Value")
    parser.add_argument("--reference", default=None, help="engine NAME the medians are compared with")
    parser.add_argument("--threads", default="1,4")
    parser.add_argument("--hash", type=int, default=64)
    parser.add_argument("--cpus", default="", help="logical CPUs; Threads N pins to the first N")
    parser.add_argument("--book", default=str(DEFAULT_BOOK))
    parser.add_argument("--positions", type=int, default=50)
    parser.add_argument("--nodes", type=int, default=1000)
    parser.add_argument("--nodes-rounds", type=int, default=200)
    parser.add_argument("--movetimes", default="50,100,300")
    parser.add_argument("--movetime-rounds", type=int, default=40)
    parser.add_argument("--ponder-rounds", type=int, default=40)
    parser.add_argument("--ponder-clock", default="1000+10", help="ms+ms for both sides")
    parser.add_argument("--ponder-wait", type=int, default=500, help="ms before ponderhit")
    parser.add_argument("--out", default=None)
    parser.add_argument("--report", default=None, help="recompute the table from an archive")
    a = parser.parse_args()

    if a.report:
        return report(Path(a.report))
    if not a.engine or not a.out:
        parser.error("--engine and --out are required")

    engines: dict[str, str] = {}
    for spec in a.engine:
        name, sep, path = spec.partition("=")
        if not sep or not name or name in engines:
            parser.error(f"bad or repeated --engine '{spec}'")
        if not Path(path).is_file():
            parser.error(f"no engine file {path}")
        engines[name] = str(Path(path).resolve())
    if a.reference and a.reference not in engines:
        parser.error(f"--reference {a.reference} is not an engine")
    options = parse_engine_options(a.option, list(engines))
    threads = [int(t) for t in a.threads.split(",")]
    cpus = [int(c) for c in a.cpus.split(",")] if a.cpus else []
    if cpus and max(threads) > len(cpus):
        parser.error(f"Threads {max(threads)} needs {max(threads)} CPUs, --cpus has {len(cpus)}")
    movetimes = [int(m) for m in a.movetimes.split(",") if m]
    clock_ms, inc_ms = (int(x) for x in a.ponder_clock.split("+"))
    fens = read_positions(Path(a.book), a.positions)

    out = Path(a.out)
    out.mkdir(parents=True, exist_ok=True)
    if (out / "runs.jsonl").exists():
        parser.error(f"{out / 'runs.jsonl'} exists; choose a new --out")

    meta = {
        "schema": "basilisk-wallclock-v1",
        "started_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "host": platform.node(), "python": sys.version.split()[0],
        "engines": engines, "options": {k: [f"{o}={v}" for o, v in opts]
                                         for k, opts in options.items()},
        "reference": a.reference, "threads": threads, "hash": a.hash, "cpus": cpus,
        "book": str(a.book), "positions": a.positions, "nodes": a.nodes,
        "nodes_rounds": a.nodes_rounds, "movetimes": movetimes,
        "movetime_rounds": a.movetime_rounds, "ponder_rounds": a.ponder_rounds,
        "ponder_clock": a.ponder_clock, "ponder_wait_ms": a.ponder_wait,
        "argv": sys.argv,
    }
    runs: list[dict] = []
    names = list(engines)
    with open(out / "runs.jsonl", "w", encoding="utf-8") as log:
        def record(run: dict) -> None:
            runs.append(run)
            log.write(json.dumps(run) + "\n")
            log.flush()

        for t in threads:
            procs: dict[str, Engine] = {}
            try:
                for name in names:
                    procs[name] = Engine(name, engines[name], options[name], t, a.hash, cpus)
                    meta.setdefault("ident", {})[name] = procs[name].ident
                    meta.setdefault("pinning", {})[f"{t}T {name}"] = procs[name].pinned

                def rounds(kind: str, count: int, param, fn) -> None:
                    for r in range(count):
                        order = names if r % 2 == 0 else list(reversed(names))
                        fen = fens[r % len(fens)]
                        for name in order:
                            result = fn(procs[name], fen)
                            record({"kind": kind, "param": param, "threads": t,
                                    "engine": name, "round": r, "fen": fen, **result})

                # A warm-up round per engine, not recorded: first-touch costs
                # (page faults in the hash, a lazily started pool) are not overhead.
                for name in names:
                    trial_nodes(procs[name], fens[0], a.nodes)
                rounds("nodes", a.nodes_rounds, a.nodes,
                       lambda e, f: trial_nodes(e, f, a.nodes))
                for m in movetimes:
                    rounds("movetime", a.movetime_rounds, m,
                           lambda e, f, m=m: trial_movetime(e, f, m))
                rounds("ponderhit", a.ponder_rounds, a.ponder_clock,
                       lambda e, f: trial_ponderhit(e, f, clock_ms, inc_ms, a.ponder_wait))
                print(f"Threads {t}: {sum(1 for r in runs if r['threads'] == t)} trials",
                      flush=True)
            finally:
                for engine in procs.values():
                    engine.close()

    meta["finished_utc"] = datetime.datetime.now(datetime.timezone.utc).isoformat()
    summary = summarise(runs, a.reference)
    (out / "summary.json").write_text(json.dumps({**meta, **summary}, indent=1),
                                      encoding="utf-8")
    print_summary(summary, a.reference)
    return 0


if __name__ == "__main__":
    sys.exit(main())
