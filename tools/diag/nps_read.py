#!/usr/bin/env python3
"""Read a pooled-build NPS comparison archived by tools/nps_read.ps1.

The instrument is Rarog's RAR-P32 read (PROCESS *Build, bench and NPS*):
builds, not cycles, set the limit, so each arm is a pool of independent PGO
builds of one source. Each reading is the best of one `bench 13 3`
invocation. A build's value is the median of its cycle readings. The estimate
is the difference between the arms' means of per-build medians, and its
interval is a Welch t-interval on the per-build medians, never a bootstrap of
readings.

Decisions, in percent of the base arm:

  gain           step 1 (2 cycles): >= +0.9 ACCEPT, <= +0.1 NO_CHANGE, else
                 EXTEND; step 2 (6 cycles): >= +0.5 with the lower bound
                 above 0 ACCEPT, else NO_CHANGE.
  no-regression  step 1: >= -0.1 PASS, <= -0.9 FAIL, else EXTEND; step 2:
                 <= -0.5 with the upper bound below 0 FAIL, else PASS.

Disturbance: in step 1 an arm's two cycle levels must agree within 1%. Over
six cycles a cycle more than 1% from its arm's median cycle level is
disturbed; the medians absorb two, and three or more repeat the read. A cycle
whose recorded host load is over the read's ceiling, or during which an
engine, harness or build process ran, is disturbed in either step.
"""

from __future__ import annotations

import argparse
import json
import math
import pathlib
import statistics
import sys

SCHEMA = "basilisk-nps-read-v1"
STEP1_CYCLES = 2
STEP2_CYCLES = 6
CYCLE_AGREEMENT = 1.0  # percent
ABSORBED = 2


def _betacf(a: float, b: float, x: float) -> float:
    # Continued fraction for the regularized incomplete beta (Lentz).
    tiny = 1e-300
    qab, qap, qam = a + b, a + 1.0, a - 1.0
    c, d = 1.0, 1.0 - qab * x / qap
    d = 1.0 / (d if abs(d) > tiny else tiny)
    h = d
    for m in range(1, 300):
        m2 = 2 * m
        aa = m * (b - m) * x / ((qam + m2) * (a + m2))
        d = 1.0 + aa * d
        d = 1.0 / (d if abs(d) > tiny else tiny)
        c = 1.0 + aa / c
        c = c if abs(c) > tiny else tiny
        h *= d * c
        aa = -(a + m) * (qab + m) * x / ((a + m2) * (qap + m2))
        d = 1.0 + aa * d
        d = 1.0 / (d if abs(d) > tiny else tiny)
        c = 1.0 + aa / c
        c = c if abs(c) > tiny else tiny
        delta = d * c
        h *= delta
        if abs(delta - 1.0) < 1e-14:
            break
    return h


def _betai(a: float, b: float, x: float) -> float:
    if x <= 0.0:
        return 0.0
    if x >= 1.0:
        return 1.0
    front = math.exp(math.lgamma(a + b) - math.lgamma(a) - math.lgamma(b)
                     + a * math.log(x) + b * math.log1p(-x))
    if x < (a + 1.0) / (a + b + 2.0):
        return front * _betacf(a, b, x) / a
    return 1.0 - front * _betacf(b, a, 1.0 - x) / b


def t_cdf(t: float, df: float) -> float:
    tail = 0.5 * _betai(df / 2.0, 0.5, df / (df + t * t))
    return 1.0 - tail if t >= 0 else tail


def t_quantile(p: float, df: float) -> float:
    lo, hi = 0.0, 1000.0
    for _ in range(200):
        mid = (lo + hi) / 2.0
        if t_cdf(mid, df) < p:
            lo = mid
        else:
            hi = mid
    return (lo + hi) / 2.0


def welch_interval(cand: list[float], base: list[float], level: float = 0.95):
    """Delta of the means and its Welch t-interval, in percent of base's mean."""
    mean_c, mean_b = statistics.fmean(cand), statistics.fmean(base)
    var_c, var_b = statistics.variance(cand), statistics.variance(base)
    se2_c, se2_b = var_c / len(cand), var_b / len(base)
    se = math.sqrt(se2_c + se2_b)
    delta = 100.0 * (mean_c - mean_b) / mean_b
    if se == 0.0:
        return delta, delta, delta, math.inf
    df = (se2_c + se2_b) ** 2 / (se2_c ** 2 / (len(cand) - 1) + se2_b ** 2 / (len(base) - 1))
    half = 100.0 * t_quantile(0.5 + level / 2.0, df) * se / mean_b
    return delta, delta - half, delta + half, df


class ArchiveError(ValueError):
    pass


def load(directory: pathlib.Path) -> tuple[dict, list[dict]]:
    config_path = directory / "read.json"
    runs_path = directory / "runs.jsonl"
    if not config_path.is_file() or not runs_path.is_file():
        raise ArchiveError(f"{directory} has no read.json and runs.jsonl")
    config = json.loads(config_path.read_text(encoding="utf-8"))
    if config.get("schema") != SCHEMA:
        raise ArchiveError(f"{config_path} is not a {SCHEMA} archive")
    records = [json.loads(line) for line in runs_path.read_text(encoding="utf-8").splitlines() if line.strip()]
    return config, records


def analyse(config: dict, records: list[dict]) -> dict:
    arms = {"base": config["base"], "cand": config["cand"]}
    builds = {arm: [b["sha256"] for b in spec["builds"]] for arm, spec in arms.items()}
    readings = [r for r in records if r.get("kind") == "reading"]
    cycles = sorted({r["cycle"] for r in readings})
    if cycles != list(range(1, len(cycles) + 1)):
        raise ArchiveError(f"cycles are not numbered 1..n: {cycles}")
    table: dict[tuple[str, int, int], float] = {}
    nodes: dict[str, set[int]] = {"base": set(), "cand": set()}
    for r in readings:
        arm, index, cycle = r["arm"], r["build"], r["cycle"]
        if arm not in arms or not 0 <= index < len(builds[arm]):
            raise ArchiveError(f"reading for unknown build {arm}[{index}]")
        if r["sha256"] != builds[arm][index]:
            raise ArchiveError(f"{arm}[{index}] was read from {r['sha256'][:12]}, not its pool's binary")
        key = (arm, index, cycle)
        if key in table:
            raise ArchiveError(f"{arm}[{index}] has two readings in cycle {cycle}")
        if not r["nps"]:
            raise ArchiveError(f"{arm}[{index}] cycle {cycle} has no runs")
        table[key] = float(max(r["nps"]))
        nodes[arm].add(int(r["nodes"]))
    for arm in arms:
        for index in range(len(builds[arm])):
            for cycle in cycles:
                if (arm, index, cycle) not in table:
                    raise ArchiveError(f"{arm}[{index}] has no reading in cycle {cycle}")

    ceiling = float(config.get("busy_ceiling_percent", 10.0))
    host_disturbed = set()
    for r in records:
        if r.get("kind") != "cycle":
            continue
        busy = r.get("busy_percent")
        if (busy is not None and busy > ceiling) or r.get("busy_processes"):
            host_disturbed.add(r["cycle"])

    levels = {arm: [statistics.fmean(table[(arm, i, c)] for i in range(len(builds[arm]))) for c in cycles]
              for arm in arms}
    per_build = {arm: [statistics.median(table[(arm, i, c)] for c in cycles) for i in range(len(builds[arm]))]
                 for arm in arms}
    delta, lower, upper, df = welch_interval(per_build["cand"], per_build["base"])

    disturbed = {arm: set(host_disturbed) for arm in arms}
    if len(cycles) == STEP1_CYCLES:
        for arm in arms:
            a, b = levels[arm]
            if 100.0 * abs(a - b) / ((a + b) / 2.0) > CYCLE_AGREEMENT:
                disturbed[arm].update(cycles)
    else:
        for arm in arms:
            middle = statistics.median(levels[arm])
            for cycle, level in zip(cycles, levels[arm]):
                if 100.0 * abs(level - middle) / middle > CYCLE_AGREEMENT:
                    disturbed[arm].add(cycle)

    mode = config.get("mode", "gain")
    verdict = decide(mode, len(cycles), delta, lower, upper, disturbed)
    return {
        "label": config.get("label", ""),
        "mode": mode,
        "cycles": len(cycles),
        "verdict": verdict,
        "delta_percent": delta,
        "interval_percent": [lower, upper],
        "welch_df": df,
        "per_build_median": per_build,
        "arm_mean": {arm: statistics.fmean(v) for arm, v in per_build.items()},
        "cycle_levels": levels,
        "disturbed_cycles": {arm: sorted(v) for arm, v in disturbed.items()},
        "host_disturbed_cycles": sorted(host_disturbed),
        "fingerprints": {arm: sorted(v) for arm, v in nodes.items()},
    }


def decide(mode: str, cycles: int, delta: float, lower: float, upper: float,
           disturbed: dict[str, set[int]]) -> str:
    if cycles == STEP1_CYCLES:
        if any(disturbed.values()):
            return "REPEAT_STEP_1"
        if mode == "gain":
            return "ACCEPT" if delta >= 0.9 else "NO_CHANGE" if delta <= 0.1 else "EXTEND"
        return "PASS" if delta >= -0.1 else "FAIL" if delta <= -0.9 else "EXTEND"
    if cycles == STEP2_CYCLES:
        if any(len(v) > ABSORBED for v in disturbed.values()):
            return "REPEAT_READ"
        if mode == "gain":
            return "ACCEPT" if delta >= 0.5 and lower > 0.0 else "NO_CHANGE"
        return "FAIL" if delta <= -0.5 and upper < 0.0 else "PASS"
    return "NONSTANDARD"


def render(report: dict) -> str:
    lines = [f"NPS read{' [' + report['label'] + ']' if report['label'] else ''}: "
             f"{report['mode']}, {report['cycles']} cycles"]
    for arm in ("base", "cand"):
        medians = ", ".join(f"{v / 1e6:.3f}M" for v in report["per_build_median"][arm])
        lines.append(f"  {arm}: mean of build medians {report['arm_mean'][arm] / 1e6:.4f}M  ({medians})")
        levels = ", ".join(f"{v / 1e6:.3f}M" for v in report["cycle_levels"][arm])
        lines.append(f"        cycle levels {levels}; disturbed {report['disturbed_cycles'][arm] or 'none'}")
    lower, upper = report["interval_percent"]
    lines.append(f"  delta {report['delta_percent']:+.2f}%  95% t-interval [{lower:+.2f}%, {upper:+.2f}%]"
                 f"  (Welch df {report['welch_df']:.1f})")
    prints = report["fingerprints"]
    if prints["base"] != prints["cand"] or len(prints["base"]) != 1:
        lines.append(f"  fingerprints differ: base {prints['base']} cand {prints['cand']} - not pure speed")
    lines.append(f"  verdict: {report['verdict']}")
    return "\n".join(lines)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--report", required=True, type=pathlib.Path, help="read directory written by nps_read.ps1")
    parser.add_argument("--json", type=pathlib.Path, help="also write the report here")
    args = parser.parse_args(argv)
    try:
        report = analyse(*load(args.report))
    except (ArchiveError, KeyError, statistics.StatisticsError) as error:
        print(f"nps_read: {error}", file=sys.stderr)
        return 2
    print(render(report))
    if args.json:
        args.json.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
