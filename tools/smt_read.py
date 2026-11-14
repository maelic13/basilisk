#!/usr/bin/env python3
"""Sibling-load speed read (B.5.7, BAS-S58): node rate per engine when 15, 28
or 30 single-thread engines run at once, one per listed logical CPU.

A gate plays one-thread games; without pondering each game has one engine
thinking, so a layout of N busy logical CPUs is the load of N concurrent games.
This tool keeps every listed CPU busy for a fixed window with pinned bench
processes, records each process's node rate, and asks three questions:

  yield     how much of a full core an engine keeps when its SMT sibling is
            busy (rate in a paired layout over the rate in the one-per-core
            layout), which sets a sibling run's effective time control;
  pricing   whether a pair whose arms differ only in speed (an identical
            tree) keeps its speed ratio on siblings: a memory-latency cost is
            expected to shrink there, a compute cost to stay;
  noise     the spread of node rates within one arm and layout.

    python tools/smt_read.py run --config <read.json> --out <dir>
    python tools/smt_read.py report <dir> [--json]

The launcher (tools/run_b57_reads.ps1) proves an idle host, hashes every
binary and writes read.json; `run` refuses a binary whose hash differs.

Method:
  - A phase is one engine family; a layout is a list of logical CPUs. For
    each round, every layout runs in turn for --seconds. Each CPU of the
    layout runs a worker that launches bench processes one after another
    until the window ends, cycling through the phase's binaries from an
    offset of its own, so siblings run different arms and every arm sees
    every neighbour.
  - A binary may carry its own bench depth, so builds of an older, larger
    tree run a workload of about the same size; both arms of a contrast
    always share one depth.
  - A process is created suspended, pinned by SetProcessAffinityMask and only
    then resumed, so none of its instructions runs off its CPU and an engine
    that reads its CPU set at startup (Stockfish binds its threads) sees the
    pinned one (BAS-S58 Amendment 1; the first read pinned after the start).
  - A phase marked persistent (one Stockfish) keeps one process per CPU for
    the whole window and types its benches into the UCI loop: no launches.
  - A process counts only if it started after the window's warm-up and ended
    before the window closed, so every counted process ran with every listed
    CPU busy. The rest are kept in runs.jsonl, flagged.
  - GetSystemTimes brackets each window, and every engine process's own CPU
    time (GetProcessTimes) is summed. A window whose foreign work, the busy
    share less the engines' own CPU, exceeds 1.6 logical CPUs is marked
    disturbed, and the report refuses to read a disturbed read. (The first
    read compared the busy share with the layout's CPU count; Amendment 1.)
  - `report --phase-from NAME=DIR` reads phase NAME from another read's
    directory in place of this one's (Amendment 1: the NNUE phase re-run).

The rules applied by `report` are read.json's "rules", frozen in BAS-S58.
"""

from __future__ import annotations

import argparse
import ctypes
import hashlib
import json
import random
import re
import statistics
import subprocess
import sys
import threading
import time
from ctypes import wintypes
from pathlib import Path

DISTURBED_EXTRA_CPUS = 1.6
BOOTSTRAP_RESAMPLES = 4000
BOOTSTRAP_SEED = 5758


# --------------------------------------------------------------------------
# host

class _FileTime(ctypes.Structure):
    _fields_ = [("low", wintypes.DWORD), ("high", wintypes.DWORD)]

    def value(self) -> int:
        return (self.high << 32) | self.low


def system_times() -> tuple[int, int]:
    """(busy, total) in 100 ns units over all logical CPUs."""
    idle, kernel, user = _FileTime(), _FileTime(), _FileTime()
    if not ctypes.windll.kernel32.GetSystemTimes(
            ctypes.byref(idle), ctypes.byref(kernel), ctypes.byref(user)):
        raise OSError("GetSystemTimes failed")
    total = kernel.value() + user.value()  # kernel time includes idle time
    return total - idle.value(), total


def logical_cpu_count() -> int:
    return int(ctypes.windll.kernel32.GetActiveProcessorCount(0xFFFF))


def pin(process: subprocess.Popen, cpu: int) -> None:
    if cpu < 0 or cpu > 62:
        raise ValueError(f"logical CPU {cpu} is outside 0..62")
    handle = wintypes.HANDLE(int(process._handle))  # noqa: SLF001 - the Windows handle
    if not ctypes.windll.kernel32.SetProcessAffinityMask(handle, ctypes.c_size_t(1 << cpu)):
        raise OSError(f"SetProcessAffinityMask failed for CPU {cpu}")


CREATE_SUSPENDED = 0x00000004


def process_cpu_seconds(process: subprocess.Popen) -> float:
    """User plus kernel CPU time of a process, running or exited."""
    created, exited, kernel, user = _FileTime(), _FileTime(), _FileTime(), _FileTime()
    handle = wintypes.HANDLE(int(process._handle))  # noqa: SLF001 - the Windows handle
    if not ctypes.windll.kernel32.GetProcessTimes(
            handle, ctypes.byref(created), ctypes.byref(exited), ctypes.byref(kernel), ctypes.byref(user)):
        raise OSError("GetProcessTimes failed")
    return (kernel.value() + user.value()) / 1e7


def start_pinned(arguments: list[str], cpu: int, **popen) -> subprocess.Popen:
    """Create the process suspended, pin it, then let it run."""
    process = subprocess.Popen(arguments, creationflags=CREATE_SUSPENDED, **popen)
    try:
        pin(process, cpu)
        handle = wintypes.HANDLE(int(process._handle))  # noqa: SLF001 - the Windows handle
        status = ctypes.windll.ntdll.NtResumeProcess(handle)
        if status != 0:
            raise OSError(f"NtResumeProcess failed with status {status:#x}")
    except BaseException:
        process.kill()
        raise
    return process


def sha256(path: str) -> str:
    digest = hashlib.sha256()
    with open(path, "rb") as handle:
        for block in iter(lambda: handle.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest().upper()


# --------------------------------------------------------------------------
# one bench process

BASILISK_RUN = re.compile(r"^run \d+/\d+\s+nodes (\d+)\s+time (\d+)ms\s+nps (\d+)", re.M)
STOCKFISH_NODES = re.compile(r"^Nodes searched\s*:\s*(\d+)", re.M)
STOCKFISH_TIME = re.compile(r"^Total time \(ms\)\s*:\s*(\d+)", re.M)


def bench_once(kind: str, binary: dict, cpu: int, workload: dict) -> dict:
    """Run one pinned bench process and return its node count and time."""
    if kind == "basilisk":
        process = start_pinned(
            [binary["path"]], cpu, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT, text=True)
        commands = (f"setoption name Hash value {workload['hash']}\n"
                    f"bench {binary.get('depth', workload['depth'])} {workload['repeats']} 1\nquit\n")
        out, _ = process.communicate(commands, timeout=600)
        cpu_s = process_cpu_seconds(process)
        runs = [(int(n), int(t)) for n, t, _ in BASILISK_RUN.findall(out)]
        if workload["repeats"] == 1:
            # A single run prints the detailed summary, not a "run 1/1" line.
            node_match, time_match = STOCKFISH_NODES.search(out), STOCKFISH_TIME.search(out)
            runs = [(int(node_match.group(1)), int(time_match.group(1)))] if node_match and time_match else []
        if process.returncode != 0 or len(runs) != workload["repeats"]:
            return {"ok": False, "exit": process.returncode, "tail": out[-400:], "cpu_s": cpu_s}
        nodes = sum(n for n, _ in runs)
        ms = sum(t for _, t in runs)
    elif kind == "stockfish":
        process = start_pinned(
            [binary["path"], "bench", str(workload["hash"]), "1", str(workload["depth"]),
             "default", "depth"], cpu,
            stdin=subprocess.DEVNULL, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        out, _ = process.communicate(timeout=600)
        cpu_s = process_cpu_seconds(process)
        node_match, time_match = STOCKFISH_NODES.search(out), STOCKFISH_TIME.search(out)
        if process.returncode != 0 or not node_match or not time_match:
            return {"ok": False, "exit": process.returncode, "tail": out[-400:], "cpu_s": cpu_s}
        nodes, ms = int(node_match.group(1)), int(time_match.group(1))
    else:
        raise ValueError(f"unknown engine kind {kind!r}")
    if ms <= 0:
        return {"ok": False, "exit": process.returncode, "tail": "zero time", "cpu_s": cpu_s}
    return {"ok": True, "nodes": nodes, "ms": ms, "nps": nodes * 1000.0 / ms, "cpu_s": cpu_s}


# --------------------------------------------------------------------------
# run

def run_window(phase: dict, layout_name: str, cpus: list[int], round_index: int,
               seconds: float, warmup: float, sink) -> dict:
    binaries = phase["binaries"]
    order = list(range(len(binaries)))
    random.Random(f"{phase['name']}:{layout_name}:{round_index}").shuffle(order)
    lock = threading.Lock()
    faults: list[str] = []
    own_cpu = [0.0]  # CPU seconds of this window's own engine processes
    busy0, total0 = system_times()
    start = time.monotonic()
    close = start + seconds

    def emit(cpu: int, binary: dict, began: float, ended: float, result: dict) -> None:
        record = {
            "phase": phase["name"], "layout": layout_name, "round": round_index,
            "cpu": cpu, "binary": binary["name"], "arm": binary["arm"],
            "start_s": round(began - start, 3), "end_s": round(ended - start, 3),
            "counted": bool(result["ok"]) and began >= start + warmup and ended <= close,
            **result,
        }
        with lock:
            sink(record)
            own_cpu[0] += result.get("cpu_s", 0.0)
            if not result["ok"]:
                faults.append(f"{binary['name']} on CPU {cpu}: exit {result.get('exit')}")

    def persistent_worker(slot: int, cpu: int) -> None:
        # One Stockfish per CPU for the whole window, its benches typed into
        # its UCI loop: no launch, so no per-launch system work (BAS-S58
        # Amendment 1).
        binary = binaries[0]
        workload = phase["workload"]
        process = start_pinned([binary["path"]], cpu, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT, text=True, bufsize=1)
        try:
            while time.monotonic() < close:
                began = time.monotonic()
                process.stdin.write(f"bench {workload['hash']} 1 {workload['depth']} default depth\n")
                process.stdin.flush()
                nodes = ms = None
                tail: list[str] = []
                for line in process.stdout:
                    tail = (tail + [line])[-5:]
                    node_match, time_match = STOCKFISH_NODES.match(line), STOCKFISH_TIME.match(line)
                    if node_match:
                        nodes = int(node_match.group(1))
                    if time_match:
                        ms = int(time_match.group(1))
                    if line.startswith("Nodes/second"):
                        break
                ended = time.monotonic()
                if nodes is None or not ms:
                    emit(cpu, binary, began, ended, {"ok": False, "exit": process.poll(), "tail": "".join(tail)})
                    return
                emit(cpu, binary, began, ended, {"ok": True, "nodes": nodes, "ms": ms, "nps": nodes * 1000.0 / ms})
        finally:
            try:
                process.stdin.write("quit\n")
                process.stdin.flush()
                process.wait(timeout=60)
            except (OSError, subprocess.TimeoutExpired):
                process.kill()
                process.wait()
            with lock:
                own_cpu[0] += process_cpu_seconds(process)

    def worker(slot: int, cpu: int) -> None:
        index = slot
        while time.monotonic() < close:
            binary = binaries[order[index % len(order)]]
            index += 1
            began = time.monotonic()
            result = bench_once(phase["kind"], binary, cpu, phase["workload"])
            ended = time.monotonic()
            emit(cpu, binary, began, ended, result)

    persistent = bool(phase.get("persistent"))
    if persistent and (phase["kind"] != "stockfish" or len(binaries) != 1):
        raise ValueError("a persistent phase is one Stockfish binary")
    target = persistent_worker if persistent else worker
    threads = [threading.Thread(target=target, args=(slot, cpu)) for slot, cpu in enumerate(cpus)]
    for thread in threads:
        thread.start()
    for thread in threads:
        thread.join()
    busy1, total1 = system_times()
    elapsed = time.monotonic() - start
    cpus_total = logical_cpu_count()
    busy_cpus = (busy1 - busy0) / max(1, total1 - total0) * cpus_total
    own_cpus = own_cpu[0] / elapsed
    foreign_cpus = busy_cpus - own_cpus
    return {
        "phase": phase["name"], "layout": layout_name, "round": round_index,
        "cpus": len(cpus), "busy_cpus": round(busy_cpus, 2),
        "own_cpus": round(own_cpus, 2), "foreign_cpus": round(foreign_cpus, 2),
        # Disturbed means work that is not the read's own engines: the busy
        # share less the engine processes' own CPU time (Amendment 1; the
        # first read compared the busy share with the layout's CPU count).
        "disturbed": foreign_cpus > DISTURBED_EXTRA_CPUS,
        "faults": faults,
        "elapsed_s": round(elapsed, 1),
    }


def command_run(args: argparse.Namespace) -> int:
    config = json.loads(Path(args.config).read_text(encoding="utf-8-sig"))
    out = Path(args.out)
    for phase in config["phases"]:
        for binary in phase["binaries"]:
            actual = sha256(binary["path"])
            if actual != binary["sha256"].upper():
                print(f"PROVENANCE MISMATCH - {binary['path']} is {actual}, not {binary['sha256']}",
                      file=sys.stderr)
                return 2
    out.mkdir(parents=True, exist_ok=False)
    (out / "read.json").write_text(json.dumps(config, indent=2), encoding="utf-8")
    runs = (out / "runs.jsonl").open("a", encoding="utf-8")
    windows = (out / "windows.jsonl").open("a", encoding="utf-8")

    def sink(record: dict) -> None:
        runs.write(json.dumps(record) + "\n")
        runs.flush()

    # One untimed bench per binary first: a fresh executable is scanned by
    # antivirus, which would otherwise contaminate the first window (RAR-M48).
    for phase in config["phases"]:
        warm = dict(phase["workload"], repeats=1) if phase["kind"] == "basilisk" else phase["workload"]
        for binary in phase["binaries"]:
            result = bench_once(phase["kind"], binary, config["warmup_cpu"], warm)
            if not result["ok"]:
                print(f"WARM-UP FAULT - {binary['name']}: {result}", file=sys.stderr)
                return 2
    time.sleep(config.get("settle_seconds", 20))

    plan = [(phase, layout) for phase in config["phases"] for layout in phase["layouts"]]
    fault = False
    for round_index in range(config["rounds"]):
        for phase, layout_name in plan:
            cpus = config["layouts"][layout_name]
            print(f"round {round_index + 1}/{config['rounds']}  {phase['name']}  {layout_name} "
                  f"({len(cpus)} CPUs, {phase['seconds']} s)", flush=True)
            summary = run_window(phase, layout_name, cpus, round_index,
                                 phase["seconds"], phase["warmup_seconds"], sink)
            windows.write(json.dumps(summary) + "\n")
            windows.flush()
            print(f"  busy {summary['busy_cpus']} CPUs, engines' own {summary['own_cpus']}, foreign "
                  f"{summary['foreign_cpus']}; disturbed {summary['disturbed']}; "
                  f"faults {len(summary['faults'])}", flush=True)
            fault = fault or bool(summary["faults"])
            time.sleep(config.get("gap_seconds", 5))
    runs.close()
    windows.close()
    print(f"Read complete: {out}. Report: python tools/smt_read.py report {out}")
    return 1 if fault else 0


# --------------------------------------------------------------------------
# report

def load(directory: Path) -> tuple[dict, list[dict], list[dict]]:
    config = json.loads((directory / "read.json").read_text(encoding="utf-8-sig"))
    runs = [json.loads(line) for line in (directory / "runs.jsonl").read_text(encoding="utf-8").splitlines() if line]
    windows = [json.loads(line) for line in (directory / "windows.jsonl").read_text(encoding="utf-8").splitlines() if line]
    return config, runs, windows


def load_combined(directory: Path, phase_from: dict[str, Path]) -> tuple[dict, list[dict], list[dict]]:
    """The read in directory, with each named phase taken from another read."""
    config, runs, windows = load(directory)
    for name, other in phase_from.items():
        other_config, other_runs, other_windows = load(other)
        replacement = [p for p in other_config["phases"] if p["name"] == name]
        if len(replacement) != 1 or not any(p["name"] == name for p in config["phases"]):
            raise SystemExit(f"phase {name!r} is not in both {directory} and {other}")
        config["phases"] = [replacement[0] if p["name"] == name else p for p in config["phases"]]
        runs = [r for r in runs if r["phase"] != name] + [r for r in other_runs if r["phase"] == name]
        windows = [w for w in windows if w["phase"] != name] + [w for w in other_windows if w["phase"] == name]
    return config, runs, windows


def mean(values: list[float]) -> float:
    return sum(values) / len(values)


def cell_rates(runs: list[dict], phase: str, layout: str, arm: str) -> list[float]:
    return [r["nps"] for r in runs if r["counted"] and r["phase"] == phase
            and r["layout"] == layout and r["arm"] == arm]


def bootstrap(samples: list[list[float]], statistic, resamples: int = BOOTSTRAP_RESAMPLES) -> tuple[float, float]:
    """95% percentile interval of statistic(*cells), resampling each cell."""
    rng = random.Random(BOOTSTRAP_SEED)
    values = []
    for _ in range(resamples):
        values.append(statistic(*[[cell[rng.randrange(len(cell))] for _ in cell] for cell in samples]))
    values.sort()
    return values[int(0.025 * resamples)], values[int(0.975 * resamples) - 1]


def judge_interval(low: float, high: float, tolerance: float) -> str:
    if -tolerance <= low and high <= tolerance:
        return "PASS"
    if low > tolerance or high < -tolerance:
        return "FAIL"
    return "UNRESOLVED"


def build_report(directory: Path, phase_from: dict[str, Path] | None = None) -> dict:
    config, runs, windows = load_combined(directory, phase_from or {})
    rules = config["rules"]
    reference = rules["reference_layout"]
    report: dict = {"directory": str(directory), "windows": windows, "phases": {}, "verdicts": {}}
    report["disturbed"] = [w for w in windows if w["disturbed"]]
    report["faults"] = [f for w in windows for f in w["faults"]]
    for phase in config["phases"]:
        arms = sorted({b["arm"] for b in phase["binaries"]})
        cells = {}
        for layout in phase["layouts"]:
            for arm in arms:
                rates = cell_rates(runs, phase["name"], layout, arm)
                if len(rates) < rules["min_processes_per_cell"]:
                    raise SystemExit(f"{phase['name']}/{layout}/{arm}: {len(rates)} counted processes, "
                                     f"fewer than the registered {rules['min_processes_per_cell']}")
                cells[(layout, arm)] = rates
        table = {}
        for (layout, arm), rates in cells.items():
            table.setdefault(layout, {})[arm] = {
                "n": len(rates), "mean_nps": round(mean(rates)),
                "cv_percent": round(100 * statistics.pstdev(rates) / mean(rates), 2),
            }
        yields = {}
        for layout in phase["layouts"]:
            if layout == reference:
                continue
            per_arm = {}
            for arm in arms:
                paired, single = cells[(layout, arm)], cells[(reference, arm)]
                low, high = bootstrap([paired, single], lambda p, s: mean(p) / mean(s))
                per_arm[arm] = {"yield": round(mean(paired) / mean(single), 4),
                                "ci": [round(low, 4), round(high, 4)]}
            all_paired = [r for arm in arms for r in cells[(layout, arm)]]
            all_single = [r for arm in arms for r in cells[(reference, arm)]]
            per_arm["all"] = {"yield": round(mean(all_paired) / mean(all_single), 4)}
            yields[layout] = per_arm
        contrasts = {}
        for contrast in phase.get("contrasts", []):
            slow, fast = contrast["slow"], contrast["fast"]

            def cost(s: list[float], f: list[float]) -> float:
                return 100.0 * (1.0 - mean(s) / mean(f))

            ref_cost = cost(cells[(reference, slow)], cells[(reference, fast)])
            ref_low, ref_high = bootstrap([cells[(reference, slow)], cells[(reference, fast)]], cost)
            entry = {"kind": contrast["kind"], "slow": slow, "fast": fast,
                     reference: {"cost_percent": round(ref_cost, 3), "ci": [round(ref_low, 3), round(ref_high, 3)]}}
            for layout in phase["layouts"]:
                if layout == reference:
                    continue
                layout_cost = cost(cells[(layout, slow)], cells[(layout, fast)])

                def shift(ps: list[float], pf: list[float], ss: list[float], sf: list[float]) -> float:
                    return cost(ps, pf) - cost(ss, sf)

                low, high = bootstrap([cells[(layout, slow)], cells[(layout, fast)],
                                       cells[(reference, slow)], cells[(reference, fast)]], shift)
                tolerance = max(rules["pricing_floor_pp"], rules["pricing_share"] * abs(ref_cost))
                entry[layout] = {
                    "cost_percent": round(layout_cost, 3),
                    "shift_pp": round(layout_cost - ref_cost, 3), "shift_ci": [round(low, 3), round(high, 3)],
                    "tolerance_pp": round(tolerance, 3),
                    "verdict": judge_interval(low, high, tolerance) if contrast["judged"] else "REPORTED",
                }
            contrasts[contrast["name"]] = entry
        report["phases"][phase["name"]] = {"table": table, "yields": yields, "contrasts": contrasts}

    # Verdicts, as registered.
    verdicts = report["verdicts"]
    if report["disturbed"] or report["faults"]:
        verdicts["read"] = "VOID (disturbed window or process fault)"
        return report
    verdicts["read"] = "VALID"
    classical, nnue = rules["classical_phase"], rules["nnue_phase"]
    for name, entry in report["phases"][classical]["contrasts"].items():
        for layout in rules["judged_layouts"]:
            verdicts[f"pricing {name} {layout}"] = entry[layout]["verdict"]
    for layout in rules["judged_layouts"]:
        y_classical = report["phases"][classical]["yields"][layout][rules["yield_arm"]]["yield"]
        y_nnue = report["phases"][nnue]["yields"][layout]["all"]["yield"]
        gap = y_classical - y_nnue
        verdicts[f"nnue yield {layout}"] = (
            "PASS" if gap <= rules["nnue_gap_pass"] else "FAIL" if gap > rules["nnue_gap_fail"] else "MARGINAL")
        verdicts[f"effective control {layout}"] = (
            f"{3000 * y_classical / 1000:.2f}+{30 * y_classical / 1000:.3f} s (classical), "
            f"{3000 * y_nnue / 1000:.2f}+{30 * y_nnue / 1000:.3f} s (NNUE proxy)")
    return report


def print_report(report: dict) -> None:
    print(f"Sibling-load speed read: {report['directory']}")
    for name, phase in report["phases"].items():
        print(f"\n[{name}] node rate per process (counted processes, mean, CV)")
        for layout, arms in phase["table"].items():
            cells = "  ".join(f"{arm} n={v['n']} {v['mean_nps'] / 1e6:.3f}M cv {v['cv_percent']}%"
                              for arm, v in arms.items())
            print(f"  {layout}: {cells}")
        for layout, per_arm in phase["yields"].items():
            cells = "  ".join(f"{arm} {v['yield']:.3f}" + (f" [{v['ci'][0]:.3f}, {v['ci'][1]:.3f}]" if "ci" in v else "")
                              for arm, v in per_arm.items())
            print(f"  yield {layout}: {cells}")
        for cname, entry in phase["contrasts"].items():
            print(f"  contrast {cname} ({entry['kind']}): slow {entry['slow']}, fast {entry['fast']}")
            for layout, value in entry.items():
                if not isinstance(value, dict):
                    continue
                if "shift_pp" in value:
                    print(f"    {layout}: cost {value['cost_percent']:+.2f}%  shift {value['shift_pp']:+.2f} pp "
                          f"[{value['shift_ci'][0]:+.2f}, {value['shift_ci'][1]:+.2f}]  tolerance "
                          f"+/-{value['tolerance_pp']:.2f}  {value['verdict']}")
                else:
                    print(f"    {layout}: cost {value['cost_percent']:+.2f}% [{value['ci'][0]:+.2f}, {value['ci'][1]:+.2f}]")
    for name, path in report.get("phase_from", {}).items():
        print(f"\nphase {name} read from {path}")
    for window in report["disturbed"]:
        extra = (f"foreign {window['foreign_cpus']} CPUs" if "foreign_cpus" in window
                 else f"busy {window['busy_cpus']} on {window['cpus']} CPUs")
        print(f"disturbed window: {window['phase']} {window['layout']} round {window['round'] + 1}, {extra}")
    print("\nverdicts:")
    for key, value in report["verdicts"].items():
        print(f"  {key}: {value}")


def command_report(args: argparse.Namespace) -> int:
    phase_from = {}
    for item in args.phase_from:
        name, _, other = item.partition("=")
        if not name or not other:
            raise SystemExit(f"--phase-from takes NAME=DIR, not {item!r}")
        phase_from[name] = Path(other)
    report = build_report(Path(args.directory), phase_from)
    report["phase_from"] = {name: str(path) for name, path in phase_from.items()}
    if args.json:
        print(json.dumps(report, indent=2))
    else:
        print_report(report)
    return 0 if report["verdicts"].get("read") == "VALID" else 1


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="command", required=True)
    run = sub.add_parser("run")
    run.add_argument("--config", required=True)
    run.add_argument("--out", required=True)
    run.set_defaults(func=command_run)
    report = sub.add_parser("report")
    report.add_argument("directory")
    report.add_argument("--json", action="store_true")
    report.add_argument("--phase-from", action="append", default=[], metavar="NAME=DIR",
                        help="read phase NAME from another read's directory")
    report.set_defaults(func=command_report)
    args = parser.parse_args()
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
