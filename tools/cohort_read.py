#!/usr/bin/env python3
"""Loaded cohort NPS read: two pools searched at once on the timed-game cohort.

A single-engine `bench` read sees a third of a game's table wait and none of
the shared host's load (B.8.5): the gates run fifteen engines at once, each
with its own 64 MiB table. This read puts the arms under that load together,
so both see the same caches, memory traffic and clock in the same seconds.

    python tools/cohort_read.py run --pools A.pool.json B.pool.json \
        --contrast read=A:B:gain --out tools/results/<dir> --label "..."
    python tools/cohort_read.py report tools/results/<dir> [--json out.json]
    python tools/cohort_read.py fingerprint <engine.exe> [--jobs 8]

Method (B.8.8's packet, section 4.1):
  - Fourteen engines at once by default, each pinned to its own physical core
    (one logical CPU of it), seven from each of the two arms. The first two
    physical cores stay free for this harness, which pins itself there. The
    arms take alternate cores and swap them every cycle.
  - Each engine is a fresh process: `ucinewgame` once, then the first 200
    cohort positions, `go nodes 150000` each, Hash 64, no `ucinewgame` between
    positions (a game clears its table once). An engine that finishes its 200
    goes on searching later cohort positions, not counted, until every engine
    of the cycle has finished, so no counted search runs on a lighter host.
  - A reading is one engine's nodes over its summed search time, the time from
    sending `go` to reading `bestmove`. A search counts its limit, or its last
    line's nodes when it ends early (a mate in one, or the depth cap).
  - The seven slots of an arm run its builds in rotation, so every build is
    read at least once a cycle. A build's cycle value is the mean of its
    readings in that cycle; its value is the median over six cycles. The
    estimate and its interval are tools/diag/nps_read.py's (the delta of the
    arms' means of per-build medians, a Welch t-interval on those medians).
  - Every engine first runs one short `bench 8` (antivirus scans a fresh
    executable, RAR-M48), then the read waits for the host to be quiet.
    Before each cycle the host must be quiet again: no other engine, harness
    or build process, load under the ceiling. During a cycle the host's busy
    time less the engines' own CPU time is the foreign load; over 1.6 logical
    CPUs disturbs the cycle. A cycle level more than 1% from its arm's median
    cycle level is disturbed too; two disturbed cycles are absorbed, three
    repeat the read.
  - `--engines 1` reads one engine at a time on the last physical core: a
    cycle reads every build of every arm once, interleaved, the order rotating
    (nps_read.ps1's cycle). It may hold more than two arms.

Contrast modes:
  gain           nps_read.py's step-2 rule (+0.5% with the lower bound above
                 0), and the memory items' loaded rule beside it: +1.0% or more
                 with the lower bound above 0.
  no-regression  nps_read.py's step-2 rule.
  null           the instrument's admission (the packet's 4.1): within +-0.5%,
                 an interval that holds 0, a half-width of 1.0% or less.
  report         an estimate that decides nothing.

`fingerprint` searches every cohort position to depth 10 at Hash 16 with
`ucinewgame` before each and prints the node total and a digest over each
position's last-line node count and best move; it exits 1 when they are not
the expected pair. The positions are independent, so --jobs shards them.

The platform-specific calls (CPU topology, affinity, priority, process and
system times, the process list) are each behind one function that takes the
Linux form there.
"""

from __future__ import annotations

import argparse
import ctypes
import hashlib
import json
import os
import statistics
import subprocess
import sys
import threading
import time
from datetime import datetime, timezone
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent / "diag"))
import nps_read  # noqa: E402  - the estimate and the step-2 rule

SCHEMA = "basilisk-cohort-read-v1"
REPO = Path(__file__).resolve().parent.parent
COHORT = REPO / "tools" / "diag" / "game_cohort_v1.epd"
CYCLES = 6
POSITIONS = 200
NODES = 150_000
HASH_MB = 64
ENGINES = 14
FREE_CORES = 2
WARMUP_DEPTH = 8
BUSY_CEILING = 10.0          # percent of the host, before a cycle
FOREIGN_CPUS = 1.6           # logical CPUs of foreign work during a cycle
QUIET_SECONDS = 60
MAX_QUIET_MINUTES = 30
CYCLE_AGREEMENT = 1.0        # percent
ABSORBED = 2
LOADED_RULE = 1.0            # percent, the memory items' loaded bar
NULL_BOUND = 0.5             # percent
NULL_HALF_WIDTH = 1.0        # percent
FINGERPRINT_DEPTH = 10
FINGERPRINT_HASH = 16
FINGERPRINT_NODES = 24_421_195
FINGERPRINT_DIGEST = "8bc7f3c8d1dde0d2"
MODES = ("gain", "no-regression", "null", "report")
BUSY_NAMES = ("basilisk", "rarog", "stockfish", "fastchess", "colosseum-cli", "cutechess",
              "clang", "cmake", "ninja")
WINDOWS = sys.platform == "win32"


# --------------------------------------------------------------------------
# platform

if WINDOWS:
    from ctypes import wintypes

    class _FileTime(ctypes.Structure):
        _fields_ = [("low", wintypes.DWORD), ("high", wintypes.DWORD)]

        def value(self) -> int:
            return (self.high << 32) | self.low


def physical_cpus() -> list[int]:
    """The first logical CPU of every physical core, in order."""
    if WINDOWS:
        kernel32 = ctypes.windll.kernel32
        length = wintypes.DWORD(0)
        kernel32.GetLogicalProcessorInformationEx(0, None, ctypes.byref(length))
        buffer = ctypes.create_string_buffer(length.value)
        if not kernel32.GetLogicalProcessorInformationEx(0, buffer, ctypes.byref(length)):
            raise OSError("GetLogicalProcessorInformationEx failed")
        raw, cpus, offset = buffer.raw, [], 0
        while offset < length.value:
            size = int.from_bytes(raw[offset + 4:offset + 8], "little")
            groups = int.from_bytes(raw[offset + 30:offset + 32], "little")
            if groups != 1 or int.from_bytes(raw[offset + 40:offset + 42], "little") != 0:
                raise OSError("a core outside processor group 0 is not supported")
            mask = int.from_bytes(raw[offset + 32:offset + 40], "little")
            cpus.append((mask & -mask).bit_length() - 1)
            offset += size
        return sorted(cpus)
    cores = set()
    for path in Path("/sys/devices/system/cpu").glob("cpu[0-9]*/topology/thread_siblings_list"):
        first = path.read_text().strip().replace("-", ",").split(",")[0]
        cores.add(int(first))
    if not cores:
        raise OSError("no CPU topology under /sys/devices/system/cpu")
    return sorted(cores)


def start_pinned(arguments: list[str], cpu: int, **popen) -> subprocess.Popen:
    """Start a process already confined to one logical CPU, at high priority
    where the platform allows it without privileges."""
    if not WINDOWS:
        return subprocess.Popen(arguments, preexec_fn=lambda: os.sched_setaffinity(0, {cpu}), **popen)
    if not 0 <= cpu <= 62:
        raise ValueError(f"logical CPU {cpu} is outside 0..62")
    create_suspended, high_priority = 0x00000004, 0x00000080
    process = subprocess.Popen(arguments, creationflags=create_suspended | high_priority, **popen)
    try:
        handle = wintypes.HANDLE(int(process._handle))  # noqa: SLF001 - the Windows handle
        if not ctypes.windll.kernel32.SetProcessAffinityMask(handle, ctypes.c_size_t(1 << cpu)):
            raise OSError(f"SetProcessAffinityMask failed for CPU {cpu}")
        status = ctypes.windll.ntdll.NtResumeProcess(handle)
        if status != 0:
            raise OSError(f"NtResumeProcess failed with status {status:#x}")
    except BaseException:
        process.kill()
        raise
    return process


def pin_self(cpus: list[int]) -> None:
    """Confine this harness to the given logical CPUs."""
    if not WINDOWS:
        os.sched_setaffinity(0, set(cpus))
        return
    mask = 0
    for cpu in cpus:
        mask |= 1 << cpu
    handle = ctypes.windll.kernel32.GetCurrentProcess()
    if not ctypes.windll.kernel32.SetProcessAffinityMask(handle, ctypes.c_size_t(mask)):
        raise OSError("SetProcessAffinityMask failed for the harness")


def system_times() -> tuple[float, float]:
    """(busy, total) CPU seconds summed over every logical CPU since boot."""
    if WINDOWS:
        idle, kernel, user = _FileTime(), _FileTime(), _FileTime()
        if not ctypes.windll.kernel32.GetSystemTimes(ctypes.byref(idle), ctypes.byref(kernel), ctypes.byref(user)):
            raise OSError("GetSystemTimes failed")
        total = kernel.value() + user.value()  # kernel time includes idle time
        return (total - idle.value()) / 1e7, total / 1e7
    fields = [int(v) for v in Path("/proc/stat").read_text().splitlines()[0].split()[1:]]
    tick = os.sysconf("SC_CLK_TCK")
    idle = fields[3] + (fields[4] if len(fields) > 4 else 0)
    return (sum(fields) - idle) / tick, sum(fields) / tick


def process_cpu_seconds(process: subprocess.Popen) -> float:
    """User plus kernel CPU time of a process, running or exited."""
    if WINDOWS:
        created, exited, kernel, user = _FileTime(), _FileTime(), _FileTime(), _FileTime()
        handle = wintypes.HANDLE(int(process._handle))  # noqa: SLF001 - the Windows handle
        if not ctypes.windll.kernel32.GetProcessTimes(handle, ctypes.byref(created), ctypes.byref(exited),
                                                      ctypes.byref(kernel), ctypes.byref(user)):
            raise OSError("GetProcessTimes failed")
        return (kernel.value() + user.value()) / 1e7
    fields = Path(f"/proc/{process.pid}/stat").read_text().rsplit(")", 1)[1].split()
    return (int(fields[11]) + int(fields[12])) / os.sysconf("SC_CLK_TCK")


def running_processes() -> list[tuple[int, str]]:
    """(pid, lower-case image name) of every process."""
    if WINDOWS:
        text = subprocess.run(["tasklist", "/fo", "csv", "/nh"], capture_output=True, text=True,
                              check=True).stdout
        out = []
        for line in text.splitlines():
            parts = [p.strip('"') for p in line.split('","')]
            if len(parts) >= 2 and parts[1].isdigit():
                out.append((int(parts[1]), parts[0].lower()))
        return out
    out = []
    for comm in Path("/proc").glob("[0-9]*/comm"):
        try:
            out.append((int(comm.parent.name), comm.read_text().strip().lower()))
        except OSError:
            pass
    return out


def foreign_busy_processes(own: set[int]) -> list[str]:
    """Engine, harness and build processes this read did not start."""
    return sorted({name for pid, name in running_processes()
                   if pid not in own and pid != os.getpid() and name.startswith(BUSY_NAMES)})


def host_busy_percent(window: float = 2.0) -> float:
    busy0, total0 = system_times()
    self0 = time.process_time()
    time.sleep(window)
    busy1, total1 = system_times()
    self1 = time.process_time()
    total = total1 - total0
    if total <= 0:
        return 0.0
    return max(0.0, 100.0 * ((busy1 - busy0) - (self1 - self0)) / total)


# --------------------------------------------------------------------------
# helpers

def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with open(path, "rb") as handle:
        for block in iter(lambda: handle.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def utc() -> str:
    return datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")


def cohort(path: Path) -> list[str]:
    """The cohort's positions as FENs with zeroed counters, as B.8.5 read them."""
    fens = []
    for line in path.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if line:
            fields = line.split(";")[0].split()
            fens.append(" ".join(fields[:4]) + " 0 1")
    return fens


class Engine:
    """One UCI engine process on one logical CPU."""

    def __init__(self, exe: Path, cpu: int):
        self.process = start_pinned([str(exe)], cpu, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                    stderr=subprocess.DEVNULL, text=True, bufsize=1)

    def send(self, line: str) -> None:
        self.process.stdin.write(line + "\n")
        self.process.stdin.flush()

    def read_until(self, prefix: str) -> list[str]:
        lines = []
        while True:
            line = self.process.stdout.readline()
            if not line:
                raise RuntimeError(f"engine exited while waiting for {prefix!r}")
            lines.append(line)
            if line.startswith(prefix):
                return lines

    def handshake(self, hash_mb: int) -> None:
        self.send("uci")
        self.read_until("uciok")
        self.send(f"setoption name Hash value {hash_mb}")
        self.send("isready")
        self.read_until("readyok")

    def search(self, fen: str, go: str) -> tuple[list[str], float]:
        """The search's output lines and the time from `go` to `bestmove`."""
        self.send("position fen " + fen)
        start = time.perf_counter()
        self.send(go)
        lines = self.read_until("bestmove")
        return lines, time.perf_counter() - start

    def quit(self) -> float:
        try:
            self.send("quit")
        except OSError:
            pass
        self.process.wait(timeout=60)
        return process_cpu_seconds(self.process)


def last_info(lines: list[str]) -> dict:
    """Depth, nodes and score of the last `info` line that carries nodes."""
    out = {"depth": 0, "nodes": 0, "mate": None}
    for line in lines:
        if not line.startswith("info") or " nodes " not in line:
            continue
        words = line.split()
        out["nodes"] = int(words[words.index("nodes") + 1])
        if "depth" in words:
            out["depth"] = int(words[words.index("depth") + 1])
        out["mate"] = int(words[words.index("mate") + 1]) if "mate" in words else None
    return out


def counted_nodes(lines: list[str], limit: int, depth_cap: int) -> tuple[int, bool]:
    """A node-limited search's nodes: its limit, or its last line's when it
    ended before the limit (a mate in one, or the depth cap)."""
    info = last_info(lines)
    if info["mate"] == 1 or info["depth"] >= depth_cap:
        return info["nodes"], True
    return limit, False


def bench_once(exe: Path, cpu: int, depth: int) -> int:
    process = start_pinned([str(exe)], cpu, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                           stderr=subprocess.DEVNULL, text=True)
    out, _ = process.communicate(f"bench {depth} 1 1\nquit\n", timeout=600)
    if process.returncode != 0:
        raise RuntimeError(f"{exe} bench exited {process.returncode}")
    for line in out.splitlines():
        if line.startswith("Nodes searched"):
            return int(line.split(":")[1])
    raise RuntimeError(f"{exe} printed no bench total")


# --------------------------------------------------------------------------
# run

def read_pool(path: Path) -> dict:
    pool = json.loads(path.read_text(encoding="utf-8-sig"))
    if pool.get("schema") != "basilisk-nps-pool-v1":
        raise SystemExit(f"{path} is not an nps_pool.ps1 pool")
    if pool.get("small_pool"):
        raise SystemExit(f"{path} is a small pool; a read needs four builds or more")
    builds = []
    for b in pool["builds"]:
        exe = REPO / b["path"]
        if sha256(exe) != b["sha256"].upper():
            raise SystemExit(f"PROVENANCE MISMATCH - {exe} does not hash to its pool's {b['sha256'][:12]}")
        builds.append({"path": b["path"], "sha256": b["sha256"].upper()})
    return {"name": pool["name"], "pool": str(path.resolve().relative_to(REPO)).replace("\\", "/"),
            "pool_sha256": sha256(path), "revision": pool["revision"], "compiler": pool["compiler"],
            "bench": pool["bench"], "builds": builds}


def parse_contrast(spec: str, arms: list[str]) -> dict:
    name, rest = spec.split("=", 1)
    parts = rest.split(":")
    if len(parts) not in (2, 3):
        raise SystemExit(f"contrast {spec!r} is not name=base:cand[:mode]")
    mode = parts[2] if len(parts) == 3 else "gain"
    if mode not in MODES or parts[0] not in arms or parts[1] not in arms or parts[0] == parts[1]:
        raise SystemExit(f"contrast {spec!r} must name two of {arms} and a mode of {MODES}")
    return {"name": name, "base": parts[0], "cand": parts[1], "mode": mode}


def wait_quiet(own: set[int], seconds: int, minutes: int, log) -> None:
    deadline = time.monotonic() + 60 * minutes
    quiet_since = None
    while True:
        procs = foreign_busy_processes(own)
        busy = host_busy_percent()
        if not procs and busy <= BUSY_CEILING:
            quiet_since = quiet_since or time.monotonic()
            if time.monotonic() - quiet_since >= seconds:
                return
        else:
            quiet_since = None
            log(f"  host not quiet ({', '.join(procs) or f'{busy:.1f}%'}); waiting")
        if time.monotonic() > deadline:
            raise SystemExit(f"HOST NOT QUIET for {minutes} min: {', '.join(procs) or f'{busy:.1f}%'}")


def run_session(exe: Path, cpu: int, fens: list[str], config: dict, barrier: threading.Barrier | None,
                done: threading.Event | None, finished: list, lock: threading.Lock, slots: int) -> dict:
    """One engine's reading: its counted positions, then filler until the cycle ends."""
    engine = Engine(exe, cpu)
    try:
        engine.handshake(config["hash_mb"])
        engine.send("ucinewgame")
        engine.send("isready")
        engine.read_until("readyok")
        if barrier is not None:
            barrier.wait()
        go = f"go nodes {config['nodes']}"
        nodes, seconds, early = 0, 0.0, 0
        digest = hashlib.sha256()
        for fen in fens[:config["positions"]]:
            lines, elapsed = engine.search(fen, go)
            counted, ended = counted_nodes(lines, config["nodes"], config["depth_cap"])
            nodes += counted
            seconds += elapsed
            early += ended
            digest.update(f"{counted} {lines[-1].split()[1]}\n".encode())
        filler = 0
        if done is not None:
            with lock:
                finished.append(cpu)
                if len(finished) == slots:
                    done.set()
            index = config["positions"]
            while not done.is_set():
                engine.search(fens[index % len(fens)], go)
                index += 1
                filler += 1
        cpu_seconds = engine.quit()
    except BaseException:
        engine.process.kill()
        if barrier is not None:
            barrier.abort()
        if done is not None:
            done.set()
        raise
    return {"nodes": nodes, "seconds": seconds, "nps": nodes / seconds, "early_ends": early,
            "tree": digest.hexdigest()[:16], "filler": filler, "cpu_seconds": cpu_seconds}


def cycle_plan(config: dict, cycle: int) -> list[dict]:
    """Which build of which arm runs on which CPU in this cycle."""
    arms = [a["name"] for a in config["arms"]]
    counts = {a["name"]: len(a["builds"]) for a in config["arms"]}
    cpus = config["cpus"]
    if config["engines"] == 1:
        pairs = counts[arms[0]]
        plan = []
        for k in range(pairs):
            index = (k + cycle - 1) % pairs
            first = (cycle + k) % len(arms)
            for j in range(len(arms)):
                plan.append({"arm": arms[(first + j) % len(arms)], "build": index, "cpu": cpus[0], "slot": 0})
        return plan
    plan = []
    per_arm = {arm: 0 for arm in arms}
    for slot, cpu in enumerate(cpus):
        arm = arms[(slot + cycle) % 2]
        j = per_arm[arm]
        per_arm[arm] += 1
        plan.append({"arm": arm, "build": (j + cycle) % counts[arm], "cpu": cpu, "slot": slot})
    return plan


def command_run(args: argparse.Namespace) -> int:
    out = Path(args.out).resolve()
    if out.exists() and any(out.iterdir()):
        raise SystemExit(f"{out} exists and is not empty; a read never overwrites one")
    arms = [read_pool(Path(p)) for p in args.pools]
    names = [a["name"] for a in arms]
    if len(set(names)) != len(names):
        raise SystemExit(f"two pools share a name: {names}")
    if len({len(a["builds"]) for a in arms}) != 1:
        raise SystemExit("every arm needs the same number of builds")
    if len({a["bench"] for a in arms}) != 1:
        raise SystemExit("the arms' bench fingerprints differ; this read is for an identical tree")
    contrasts = [parse_contrast(c, names) for c in args.contrast]
    if not contrasts:
        raise SystemExit("a read needs one or more --contrast")
    cores = physical_cpus()
    if args.engines == 1:
        cpus = [cores[-1]]
        harness = [cores[0]]
    else:
        if len(arms) != 2 or args.engines % 2:
            raise SystemExit("a loaded read holds two arms and an even number of engines")
        if len(cores) < args.engines + FREE_CORES:
            raise SystemExit(f"{args.engines} engines and {FREE_CORES} free cores need more than {len(cores)} cores")
        cpus = cores[FREE_CORES:FREE_CORES + args.engines]
        harness = list(range(0, cores[FREE_CORES]))
    fens = cohort(Path(args.cohort))
    config = {
        "schema": SCHEMA, "label": args.label, "engines": args.engines, "cycles": CYCLES,
        "positions": args.positions, "nodes": args.nodes, "hash_mb": args.hash, "depth_cap": args.depth_cap,
        "cohort": str(Path(args.cohort).resolve().relative_to(REPO)).replace("\\", "/"),
        "cohort_sha256": sha256(Path(args.cohort)), "cpus": cpus, "harness_cpus": harness,
        "busy_ceiling_percent": BUSY_CEILING, "foreign_cpus": FOREIGN_CPUS,
        "arms": arms, "contrasts": contrasts, "created_utc": utc(),
        "repo_revision": subprocess.run(["git", "-C", str(REPO), "rev-parse", "HEAD"], capture_output=True,
                                        text=True, check=True).stdout.strip(),
    }
    out.mkdir(parents=True, exist_ok=True)
    (out / "read.json").write_text(json.dumps(config, indent=2) + "\n", encoding="utf-8")
    runs = out / "runs.jsonl"
    log_path = out / "log.txt"

    def log(text: str) -> None:
        print(text, flush=True)
        with open(log_path, "a", encoding="utf-8") as handle:
            handle.write(text + "\n")

    def record(entry: dict) -> None:
        with open(runs, "a", encoding="utf-8") as handle:
            handle.write(json.dumps(entry) + "\n")

    pin_self(harness)
    sys.setswitchinterval(1e-4)
    log(f"Cohort read [{args.label}]: arms {', '.join(names)}, {args.engines} engine(s) on CPUs "
        f"{','.join(map(str, cpus))}, harness on {','.join(map(str, harness))}")
    own: set[int] = set()
    if foreign_busy_processes(own):
        raise SystemExit(f"HOST NOT IDLE - {', '.join(foreign_busy_processes(own))}")
    log("  warm-up: one bench 8 per build (antivirus scan), not counted")
    for arm in arms:
        for i, b in enumerate(arm["builds"]):
            record({"kind": "warmup", "arm": arm["name"], "build": i, "sha256": b["sha256"],
                    "nodes": bench_once(REPO / b["path"], cpus[-1], WARMUP_DEPTH)})
    log(f"  waiting for {QUIET_SECONDS} s of quiet host")
    wait_quiet(own, QUIET_SECONDS, MAX_QUIET_MINUTES, log)
    by_name = {a["name"]: a for a in arms}
    for cycle in range(1, CYCLES + 1):
        if cycle > 1:
            wait_quiet(own, 5, MAX_QUIET_MINUTES, log)
        plan = cycle_plan(config, cycle)
        started = utc()
        busy0, total0 = system_times()
        results: list[dict] = []
        if args.engines == 1:
            engine_cpu = 0.0
            for p in plan:
                b = by_name[p["arm"]]["builds"][p["build"]]
                r = run_session(REPO / b["path"], p["cpu"], fens, config, None, None, [], threading.Lock(), 1)
                engine_cpu += r["cpu_seconds"]
                results.append({**p, "sha256": b["sha256"], **r})
        else:
            barrier = threading.Barrier(len(plan))
            done = threading.Event()
            finished: list = []
            lock = threading.Lock()
            slots: list = [None] * len(plan)
            errors: list = []

            def work(k: int, p: dict) -> None:
                b = by_name[p["arm"]]["builds"][p["build"]]
                try:
                    slots[k] = {**p, "sha256": b["sha256"],
                                **run_session(REPO / b["path"], p["cpu"], fens, config, barrier, done,
                                              finished, lock, len(plan))}
                except BaseException as error:  # noqa: BLE001 - reported below
                    errors.append(error)

            threads = [threading.Thread(target=work, args=(k, p)) for k, p in enumerate(plan)]
            for t in threads:
                t.start()
            for t in threads:
                t.join()
            if errors:
                raise SystemExit(f"cycle {cycle} failed: {errors[0]!r}")
            results = slots
            engine_cpu = sum(r["cpu_seconds"] for r in results)
        busy1, total1 = system_times()
        foreign = max(0.0, (busy1 - busy0) - engine_cpu) / max(1e-9, (total1 - total0)) * os.cpu_count()
        procs = foreign_busy_processes(own)
        for r in results:
            record({"kind": "reading", "cycle": cycle, **r})
        record({"kind": "cycle", "cycle": cycle, "started_utc": started, "foreign_cpus": foreign,
                "busy_processes": procs})
        levels = {}
        for name in names:
            values = [r["nps"] for r in results if r["arm"] == name]
            levels[name] = statistics.fmean(values)
        log(f"  cycle {cycle}/{CYCLES}: " + "  ".join(f"{n} {levels[n] / 1e6:.4f}M" for n in names)
            + f"  foreign {foreign:.2f} CPUs" + (f"  BUSY: {', '.join(procs)}" if procs else ""))
    report = analyse(*load(out))
    (out / "verdict.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    log(render(report))
    return 0


# --------------------------------------------------------------------------
# report

def load(directory: Path) -> tuple[dict, list[dict]]:
    config = json.loads((directory / "read.json").read_text(encoding="utf-8"))
    if config.get("schema") != SCHEMA:
        raise nps_read.ArchiveError(f"{directory} is not a {SCHEMA} archive")
    records = [json.loads(line) for line in (directory / "runs.jsonl").read_text(encoding="utf-8").splitlines()
               if line.strip()]
    return config, records


def decide(mode: str, delta: float, lower: float, upper: float, repeat: bool) -> dict:
    if repeat:
        return {"verdict": "REPEAT_READ"}
    if mode == "null":
        admitted = abs(delta) <= NULL_BOUND and lower <= 0.0 <= upper and (upper - lower) / 2 <= NULL_HALF_WIDTH
        return {"verdict": "ADMITTED" if admitted else "NOT_ADMITTED"}
    if mode == "report":
        return {"verdict": "REPORTED"}
    verdict = nps_read.decide(mode, nps_read.STEP2_CYCLES, delta, lower, upper, {})
    out = {"verdict": verdict}
    if mode == "gain":
        out["loaded_rule"] = "MET" if delta >= LOADED_RULE and lower > 0.0 else "NOT_MET"
    return out


def analyse(config: dict, records: list[dict]) -> dict:
    arms = {a["name"]: [b["sha256"] for b in a["builds"]] for a in config["arms"]}
    readings = [r for r in records if r.get("kind") == "reading"]
    cycles = sorted({r["cycle"] for r in readings})
    if cycles != list(range(1, CYCLES + 1)):
        raise nps_read.ArchiveError(f"a read is {CYCLES} cycles numbered from 1; this one has {cycles}")
    cell: dict[tuple[str, int, int], list[float]] = {}
    trees: dict[str, set[str]] = {arm: set() for arm in arms}
    for r in readings:
        if r["sha256"] != arms[r["arm"]][r["build"]]:
            raise nps_read.ArchiveError(f"{r['arm']}[{r['build']}] was read from another binary")
        cell.setdefault((r["arm"], r["build"], r["cycle"]), []).append(r["nps"])
        trees[r["arm"]].add(f"{r['nodes']}:{r['tree']}")
    per_build = {}
    for arm, builds in arms.items():
        per_build[arm] = []
        for i in range(len(builds)):
            values = [statistics.fmean(cell[(arm, i, c)]) for c in cycles if (arm, i, c) in cell]
            if len(values) != len(cycles):
                raise nps_read.ArchiveError(f"{arm}[{i}] is missing from a cycle")
            per_build[arm].append(statistics.median(values))
    levels = {arm: [statistics.fmean([r["nps"] for r in readings if r["arm"] == arm and r["cycle"] == c])
                    for c in cycles] for arm in arms}
    host = set()
    for r in records:
        if r.get("kind") == "cycle" and (r["foreign_cpus"] > config["foreign_cpus"] or r["busy_processes"]):
            host.add(r["cycle"])
    disturbed = {}
    for arm in arms:
        middle = statistics.median(levels[arm])
        disturbed[arm] = sorted(host | {c for c, v in zip(cycles, levels[arm])
                                        if 100.0 * abs(v - middle) / middle > CYCLE_AGREEMENT})
    contrasts = []
    for c in config["contrasts"]:
        delta, lower, upper, df = nps_read.welch_interval(per_build[c["cand"]], per_build[c["base"]])
        repeat = any(len(disturbed[a]) > ABSORBED for a in (c["base"], c["cand"]))
        contrasts.append({**c, "delta_percent": delta, "interval_percent": [lower, upper], "welch_df": df,
                          **decide(c["mode"], delta, lower, upper, repeat)})
    return {"label": config.get("label", ""), "engines": config["engines"], "cycles": len(cycles),
            "contrasts": contrasts, "per_build_median": per_build,
            "arm_mean": {a: statistics.fmean(v) for a, v in per_build.items()},
            "cycle_levels": levels, "disturbed_cycles": disturbed, "host_disturbed_cycles": sorted(host),
            "trees": {a: sorted(v) for a, v in trees.items()},
            "early_ends": sorted({r["early_ends"] for r in readings})}


def render(report: dict) -> str:
    lines = [f"Cohort read [{report['label']}]: {report['engines']} engine(s), {report['cycles']} cycles"]
    width = max(len(a) for a in report["arm_mean"])
    for arm, mean in report["arm_mean"].items():
        medians = ", ".join(f"{v / 1e6:.4f}M" for v in report["per_build_median"][arm])
        levels = ", ".join(f"{v / 1e6:.4f}M" for v in report["cycle_levels"][arm])
        lines.append(f"  {arm:<{width}}  mean of build medians {mean / 1e6:.4f}M  ({medians})")
        lines.append(f"  {'':<{width}}  cycle levels {levels}; disturbed {report['disturbed_cycles'][arm] or 'none'}")
    for c in report["contrasts"]:
        lower, upper = c["interval_percent"]
        extra = f"  loaded rule {c['loaded_rule']}" if "loaded_rule" in c else ""
        lines.append(f"  {c['name']} ({c['mode']}, {c['cand']} over {c['base']}): delta {c['delta_percent']:+.2f}%"
                     f"  95% t-interval [{lower:+.2f}%, {upper:+.2f}%]  (Welch df {c['welch_df']:.1f})"
                     f"  verdict: {c['verdict']}{extra}")
    trees = report["trees"]
    if len({tuple(v) for v in trees.values()}) != 1 or len(next(iter(trees.values()))) != 1:
        lines.append(f"  TREES DIFFER: {trees} - not pure speed")
    return "\n".join(lines)


def command_report(args: argparse.Namespace) -> int:
    try:
        report = analyse(*load(Path(args.directory)))
    except (nps_read.ArchiveError, KeyError, statistics.StatisticsError) as error:
        print(f"cohort_read: {error}", file=sys.stderr)
        return 2
    print(render(report))
    if args.json:
        Path(args.json).write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return 0


# --------------------------------------------------------------------------
# fingerprint

def fingerprint_shard(exe: Path, fens: list[tuple[int, str]], depth: int, hash_mb: int,
                      out: dict[int, tuple[int, str]]) -> None:
    process = subprocess.Popen([str(exe)], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                               stderr=subprocess.DEVNULL, text=True, bufsize=1)
    engine = Engine.__new__(Engine)
    engine.process = process
    try:
        engine.handshake(hash_mb)
        for index, fen in fens:
            engine.send("ucinewgame")
            lines, _ = engine.search(fen, f"go depth {depth}")
            out[index] = (last_info(lines)["nodes"], lines[-1].split()[1])
        engine.send("quit")
        process.wait(timeout=60)
    finally:
        if process.poll() is None:
            process.kill()


def command_fingerprint(args: argparse.Namespace) -> int:
    exe = Path(args.engine).resolve()
    fens = cohort(Path(args.cohort))
    out: dict[int, tuple[int, str]] = {}
    shards = [[(i, f) for i, f in enumerate(fens) if i % args.jobs == k] for k in range(args.jobs)]
    start = time.monotonic()
    threads = [threading.Thread(target=fingerprint_shard, args=(exe, s, args.depth, args.hash, out))
               for s in shards]
    for t in threads:
        t.start()
    for t in threads:
        t.join()
    if len(out) != len(fens):
        print(f"cohort_read: {len(fens) - len(out)} positions returned no result", file=sys.stderr)
        return 2
    digest = hashlib.sha256()
    total = 0
    for i in range(len(fens)):
        nodes, move = out[i]
        total += nodes
        digest.update(f"{nodes} {move}\n".encode())
    text = digest.hexdigest()[:16]
    print(f"positions {len(fens)} depth {args.depth} hash {args.hash} nodes {total} digest {text} "
          f"seconds {time.monotonic() - start:.1f} engine {exe.name}")
    if args.no_expect:
        return 0
    if total != args.expect_nodes or text != args.expect_digest:
        print(f"FINGERPRINT MISMATCH - expected {args.expect_nodes} {args.expect_digest}", file=sys.stderr)
        return 1
    print("fingerprint OK")
    return 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = parser.add_subparsers(dest="command", required=True)
    run = sub.add_parser("run", help="run a read")
    run.add_argument("--pools", nargs="+", required=True)
    run.add_argument("--contrast", action="append", default=[], help="name=base:cand[:mode]")
    run.add_argument("--out", required=True)
    run.add_argument("--label", default="")
    run.add_argument("--engines", type=int, default=ENGINES)
    run.add_argument("--positions", type=int, default=POSITIONS)
    run.add_argument("--nodes", type=int, default=NODES)
    run.add_argument("--hash", type=int, default=HASH_MB)
    run.add_argument("--depth-cap", type=int, default=100, help="the engine's maximum search depth")
    run.add_argument("--cohort", default=str(COHORT))
    report = sub.add_parser("report", help="decide an archived read again")
    report.add_argument("directory")
    report.add_argument("--json")
    fp = sub.add_parser("fingerprint", help="the cohort fingerprint of one engine")
    fp.add_argument("engine")
    fp.add_argument("--jobs", type=int, default=8)
    fp.add_argument("--depth", type=int, default=FINGERPRINT_DEPTH)
    fp.add_argument("--hash", type=int, default=FINGERPRINT_HASH)
    fp.add_argument("--cohort", default=str(COHORT))
    fp.add_argument("--expect-nodes", type=int, default=FINGERPRINT_NODES)
    fp.add_argument("--expect-digest", default=FINGERPRINT_DIGEST)
    fp.add_argument("--no-expect", action="store_true")
    args = parser.parse_args(argv)
    return {"run": command_run, "report": command_report, "fingerprint": command_fingerprint}[args.command](args)


if __name__ == "__main__":
    sys.exit(main())
