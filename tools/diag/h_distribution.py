#!/usr/bin/env python3
"""BAS-O10, C.2.2's H-distribution read: is the evaluation gap wider off the
game line than on it?

Three static evaluations -- Basilisk's, 9587eeeb's (the oracle with
`Use Basilisk HCE=false`) and Stockfish 16's classical one -- are scored
against Stockfish 19's expected score on two populations:

* **G**, positions from the gate games (C.0's `positions.csv`);
* **O**, each one's sibling one random legal move off the game line, a
  position a search visits and a game does not.

    python tools/diag/h_distribution.py run --out DIR [options]

`run` does every stage and resumes after an interruption:

1. `prepare`: sample the pairs and write `positions.csv`;
2. `reference`: Stockfish 19 at a fixed node count, one process a worker,
   appending to `reference-<worker>.csv` (`UCI_ShowWDL`; expected score
   `(W + D/2) / 1000` for the side to move);
3. `evaluate`: the three static evaluations, to `evals.csv`;
4. `analyse`: the read, to `report.json`.

Each stage is also its own command, and `--limit N` dry-runs the whole chain
on N pairs.

The read: for each evaluation and population, a logistic scale `k` is fitted
on the even pairs by cross-entropy against the reference, and skill
(1 - cross-entropy / the constant predictor's) is scored on the odd pairs.
`gap(E, P) = skill(E, P) - skill(Basilisk, P)`, and
`delta(E) = gap(E, O) - gap(E, G)`, each with a 95% interval from 2,000
bootstrap resamples of pairs. The verdict rule is frozen in the ledger
(BAS-O10).
"""

from __future__ import annotations

import argparse
import csv
import json
import math
import os
import random
import re
import subprocess
import sys
from multiprocessing import Pool
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ENGINES = Path("D:/chess/engines")
DEFAULTS = {
    "positions": ROOT / "tools" / "results" / "c0-screen" / "positions.csv",
    "reference": ENGINES / "stockfish" / "stockfish-19-windows-x86-64-universal.exe",
    "oracle": ROOT / "tools" / "test_engines" / "oracle-b111-4d2742f.exe",
    "sf16": ENGINES / "stockfish-16-x64-bmi2-classical-pgo.exe",
    "tuner": ROOT / "build" / "texel" / "basilisk-texel.exe",
}
EVALUATORS = ("basilisk", "d95", "d16")
POPULATIONS = ("G", "O")


# ---- 1. prepare -------------------------------------------------------------

def prepare(args) -> None:
    import chess
    out = args.out / "positions.csv"
    if out.exists():
        print(f"prepare: {out} exists; kept")
        return
    with args.positions.open(encoding="utf-8") as source:
        rows = list(csv.DictReader(source))
    rng = random.Random(args.seed)
    picked = rng.sample(range(len(rows)), min(len(rows), args.pairs * 2))
    pairs = []
    for index in picked:
        board = chess.Board(rows[index]["fen"])
        moves = list(board.legal_moves)
        rng.shuffle(moves)
        child = None
        for move in moves:
            board.push(move)
            if not board.is_game_over():
                child = board.fen()
                break
            board.pop()
        if child is None:
            continue
        pairs.append((rows[index]["row_id"], rows[index]["fen"], child))
        if len(pairs) == args.pairs:
            break
    args.out.mkdir(parents=True, exist_ok=True)
    with out.open("w", encoding="utf-8", newline="") as sink:
        writer = csv.writer(sink)
        writer.writerow(["pair", "population", "source_row", "fen"])
        for pair, (row_id, game, off) in enumerate(pairs):
            writer.writerow([pair, "G", row_id, game])
            writer.writerow([pair, "O", row_id, off])
    print(f"prepare: {len(pairs)} pairs -> {out}")


def load_positions(args) -> list[dict]:
    with (args.out / "positions.csv").open(encoding="utf-8") as source:
        rows = list(csv.DictReader(source))
    if args.limit:
        rows = [r for r in rows if int(r["pair"]) < args.limit]
    return rows


# ---- 2. reference -----------------------------------------------------------

class Uci:
    def __init__(self, exe: Path, options: dict[str, str]) -> None:
        self.p = subprocess.Popen([str(exe)], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                  stderr=subprocess.STDOUT, text=True, bufsize=1)
        self.send("uci")
        self.until("uciok")
        for name, value in options.items():
            self.send(f"setoption name {name} value {value}")
        self.send("isready")
        self.until("readyok")

    def send(self, line: str) -> None:
        self.p.stdin.write(line + "\n")
        self.p.stdin.flush()

    def until(self, token: str) -> list[str]:
        lines = []
        while True:
            line = self.p.stdout.readline()
            if line == "":
                raise RuntimeError(f"engine exited waiting for {token}")
            lines.append(line.rstrip("\n"))
            if line.startswith(token):
                return lines

    def close(self) -> None:
        try:
            self.send("quit")
            self.p.wait(timeout=10)
        except Exception:
            self.p.kill()


WDL = re.compile(r"\bwdl (\d+) (\d+) (\d+)")


def reference_worker(task) -> int:
    worker, rows, exe, nodes = task
    out = Path(rows[0]["_out"]).parent / f"reference-{worker}.csv" if rows else None
    if out is None:
        return 0
    done = set()
    if out.exists():
        with out.open(encoding="utf-8") as source:
            done = {(r["pair"], r["population"]) for r in csv.DictReader(source)}
    todo = [r for r in rows if (r["pair"], r["population"]) not in done]
    if not todo:
        return 0
    engine = Uci(exe, {"Threads": "1", "Hash": "64", "UCI_ShowWDL": "true"})
    fresh = not out.exists()
    with out.open("a", encoding="utf-8", newline="") as sink:
        writer = csv.writer(sink)
        if fresh:
            writer.writerow(["pair", "population", "expected"])
        for row in todo:
            engine.send("ucinewgame")
            engine.send(f"position fen {row['fen']}")
            engine.send(f"go nodes {nodes}")
            wdl = None
            for line in engine.until("bestmove"):
                found = WDL.search(line)
                if found and " multipv " not in line.replace(" multipv 1 ", " "):
                    wdl = found
            if wdl is None:
                raise RuntimeError(f"no wdl for {row['fen']}")
            w, d, _ = (int(x) for x in wdl.groups())
            writer.writerow([row["pair"], row["population"], (w + d / 2) / 1000])
            sink.flush()
    engine.close()
    return len(todo)


def reference(args) -> None:
    rows = load_positions(args)
    for row in rows:
        row["_out"] = str(args.out / "positions.csv")
    shards = [rows[w::args.workers] for w in range(args.workers)]
    tasks = [(w, shard, args.reference, args.nodes) for w, shard in enumerate(shards) if shard]
    with Pool(len(tasks)) as pool:
        done = sum(pool.map(reference_worker, tasks))
    print(f"reference: {done} new positions at {args.nodes:,} nodes")


def load_reference(args) -> dict[tuple[str, str], float]:
    out = {}
    for path in sorted(args.out.glob("reference-*.csv")):
        with path.open(encoding="utf-8") as source:
            for r in csv.DictReader(source):
                out[(r["pair"], r["population"])] = float(r["expected"])
    return out


# ---- 3. evaluate ------------------------------------------------------------

FINAL = {"d95": re.compile(r"^Original Stockfish HCE final evaluation:\s+([+-]?\d+\.\d+)\s+\(white side\)"),
         "d16": re.compile(r"^Classical evaluation\s+([+-]?\d+\.\d+)\s+\(white side\)")}


def donor_evals(kind: str, exe: Path, rows: list[dict]) -> list[float | None]:
    option = {"d95": {"Use Basilisk HCE": "false"}, "d16": {"Use NNUE": "false"}}[kind]
    engine = Uci(exe, option)
    out = []
    for row in rows:
        engine.send(f"position fen {row['fen']}")
        engine.send("eval")
        engine.send("isready")
        value = None
        for line in engine.until("readyok"):
            found = FINAL[kind].search(line)
            if found:
                value = float(found.group(1)) * 100.0  # pawns -> centipawns, White's view
        out.append(value)
    engine.close()
    return out


def basilisk_evals(tuner: Path, rows: list[dict], work: Path) -> list[float]:
    probe = work / "basilisk-probe.txt"
    probe.write_text("".join(f"{r['fen']};0.5\n" for r in rows), encoding="utf-8", newline="\n")
    done = subprocess.run([str(tuner), "--dump-eval", str(probe)], capture_output=True, text=True,
                          check=False)
    if done.returncode != 0:
        raise RuntimeError(f"basilisk-texel --dump-eval: {done.stderr}")
    values = [float(line.split()[0]) for line in done.stdout.splitlines() if line.strip()]
    if len(values) != len(rows):
        raise RuntimeError(f"basilisk-texel scored {len(values)} of {len(rows)} positions")
    return values


def evaluate(args) -> None:
    rows = load_positions(args)
    out = args.out / "evals.csv"
    values = {"basilisk": basilisk_evals(args.tuner, rows, args.out),
              "d95": donor_evals("d95", args.oracle, rows),
              "d16": donor_evals("d16", args.sf16, rows)}
    with out.open("w", encoding="utf-8", newline="") as sink:
        writer = csv.writer(sink)
        writer.writerow(["pair", "population", "stm", *EVALUATORS])
        for k, row in enumerate(rows):
            stm = row["fen"].split()[1]
            writer.writerow([row["pair"], row["population"], stm,
                             *("" if values[e][k] is None else values[e][k] for e in EVALUATORS)])
    print(f"evaluate: {len(rows)} positions -> {out}")


# ---- 4. analyse -------------------------------------------------------------

def cross_entropy(ys, xs, k):
    total = 0.0
    for y, x in zip(ys, xs):
        p = 1.0 / (1.0 + math.exp(-max(-60.0, min(60.0, x / k))))
        p = min(max(p, 1e-9), 1 - 1e-9)
        total -= y * math.log(p) + (1 - y) * math.log(1 - p)
    return total / len(ys)


def fit_k(ys, xs):
    lo, hi = math.log(10.0), math.log(2000.0)
    for _ in range(80):
        m1, m2 = lo + (hi - lo) / 3, hi - (hi - lo) / 3
        if cross_entropy(ys, xs, math.exp(m1)) < cross_entropy(ys, xs, math.exp(m2)):
            hi = m2
        else:
            lo = m1
    return math.exp((lo + hi) / 2)


def skill(ys, xs, k, base):
    const = cross_entropy(ys, [0.0] * len(ys), 1.0) if base is None else base
    return 1.0 - cross_entropy(ys, xs, k) / const


def analyse(args) -> None:
    ref = load_reference(args)
    with (args.out / "evals.csv").open(encoding="utf-8") as source:
        evals = list(csv.DictReader(source))
    table: dict[str, dict[str, dict[str, float]]] = {}
    for row in evals:
        key = (row["pair"], row["population"])
        if key not in ref or any(row[e] == "" for e in EVALUATORS):
            continue
        sign = 1.0 if row["stm"] == "w" else -1.0
        table.setdefault(row["pair"], {})[row["population"]] = {
            "y": ref[key], **{e: sign * float(row[e]) for e in EVALUATORS}}
    pairs = sorted(p for p, v in table.items() if set(v) == set(POPULATIONS))
    fit = [p for p in pairs if int(p) % 2 == 0]
    report_pairs = [p for p in pairs if int(p) % 2 == 1]

    def const_ce(ys):
        mean = sum(ys) / len(ys)
        mean = min(max(mean, 1e-9), 1 - 1e-9)
        return -sum(y * math.log(mean) + (1 - y) * math.log(1 - mean) for y in ys) / len(ys)

    ks = {}
    for pop in POPULATIONS:
        ys = [table[p][pop]["y"] for p in fit]
        for e in EVALUATORS:
            ks[(e, pop)] = fit_k(ys, [table[p][pop][e] for p in fit])

    def skills(sample):
        out = {}
        for pop in POPULATIONS:
            ys = [table[p][pop]["y"] for p in sample]
            base = const_ce(ys)
            for e in EVALUATORS:
                out[(e, pop)] = 1.0 - cross_entropy(ys, [table[p][pop][e] for p in sample], ks[(e, pop)]) / base
        return out

    def contrasts(s):
        gaps = {(e, pop): s[(e, pop)] - s[("basilisk", pop)] for e in ("d95", "d16") for pop in POPULATIONS}
        deltas = {e: gaps[(e, "O")] - gaps[(e, "G")] for e in ("d95", "d16")}
        return gaps, deltas

    point = skills(report_pairs)
    gaps, deltas = contrasts(point)
    rng = random.Random(20261122)
    boot_gaps = {k: [] for k in gaps}
    boot_deltas = {k: [] for k in deltas}
    for _ in range(args.bootstrap):
        sample = [rng.choice(report_pairs) for _ in report_pairs]
        g, d = contrasts(skills(sample))
        for k in g:
            boot_gaps[k].append(g[k])
        for k in d:
            boot_deltas[k].append(d[k])

    def interval(values):
        values = sorted(values)
        return [values[int(0.025 * len(values))], values[int(0.975 * len(values)) - 1]]

    report = {
        "pairs": len(pairs), "fit_pairs": len(fit), "report_pairs": len(report_pairs),
        "nodes": args.nodes,
        "k": {f"{e}/{pop}": round(v, 2) for (e, pop), v in ks.items()},
        "skill": {f"{e}/{pop}": round(v, 5) for (e, pop), v in point.items()},
        "gap": {f"{e}/{pop}": {"point": round(v, 5), "ci95": [round(x, 5) for x in interval(boot_gaps[(e, pop)])]}
                for (e, pop), v in gaps.items()},
        "delta": {e: {"point": round(v, 5), "ci95": [round(x, 5) for x in interval(boot_deltas[e])]}
                  for e, v in deltas.items()},
    }
    (args.out / "report.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8", newline="\n")
    print(json.dumps(report, indent=2))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("stage", choices=("run", "prepare", "reference", "evaluate", "analyse"))
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--pairs", type=int, default=10_000)
    parser.add_argument("--seed", type=int, default=20261122)
    parser.add_argument("--nodes", type=int, default=1_000_000)
    parser.add_argument("--workers", type=int, default=14)
    parser.add_argument("--bootstrap", type=int, default=2000)
    parser.add_argument("--limit", type=int, default=0, help="dry run: only the first N pairs")
    for name, value in DEFAULTS.items():
        parser.add_argument(f"--{name}", type=Path, default=value)
    args = parser.parse_args()
    for name in DEFAULTS:
        if not getattr(args, name).exists():
            print(f"error: --{name} {getattr(args, name)} does not exist", file=sys.stderr)
            return 2
    stages = ("prepare", "reference", "evaluate", "analyse") if args.stage == "run" else (args.stage,)
    for stage in stages:
        {"prepare": prepare, "reference": reference, "evaluate": evaluate, "analyse": analyse}[stage](args)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
