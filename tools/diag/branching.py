#!/usr/bin/env python3
"""Reference-anchored consecutive-depth branching profile.

Every arm and depth gets a fresh engine process. Positions within that depth
are separated by ``ucinewgame`` and readiness, so neither a prior depth nor a
prior position contributes TT/history state. The report retains every
position's sequence: aggregate endpoint growth alone is not robust to the one
position that invalidated the historical BAS-X13 profile.

Examples:
  python tools/diag/branching.py --engine basilisk.exe --label basilisk \
    --reference "oracle=oracle.exe|Use Basilisk HCE=true" \
    --reference stockfish=stockfish.exe --out tools/results/branching.json
"""

from __future__ import annotations

import argparse
import datetime
import hashlib
import json
import pathlib
import queue
import re
import statistics
import subprocess
import threading
import time

REPO = pathlib.Path(__file__).resolve().parents[2]
INFO_LINE = re.compile(r"^info depth (\d+).*?\bnodes (\d+)")
OPTION_LINE = re.compile(r"^option name (.*?) type ", re.IGNORECASE)


def file_sha256(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def parse_arm_spec(spec: str):
    if "=" not in spec:
        raise ValueError(f"arm {spec!r} must be LABEL=PATH[|Name=Value...]")
    label, remainder = spec.split("=", 1)
    path, *raw_options = remainder.split("|")
    if not label or not path:
        raise ValueError(f"arm {spec!r} has an empty label or path")
    options = []
    for raw in raw_options:
        if "=" not in raw:
            raise ValueError(f"option {raw!r} in {spec!r} is not Name=Value")
        option = tuple(raw.split("=", 1))
        if option[0].casefold() in {"hash", "threads"}:
            raise ValueError(f"{option[0]} is fixed globally, not per arm")
        options.append(option)
    return label, pathlib.Path(path).resolve(), options


def load_positions(path: pathlib.Path, limit: int):
    positions = []
    for line in path.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        positions.append(line.split(" ; ", 1)[0])
    return positions[:limit] if limit > 0 else positions


def completed_nodes(lines, requested_depth: int) -> int:
    nodes = 0
    for line in lines:
        padded = f" {line} "
        if " lowerbound " in padded or " upperbound " in padded:
            continue
        match = INFO_LINE.match(line.strip())
        if match and int(match.group(1)) == requested_depth:
            nodes = int(match.group(2))
    return nodes


class EngineSession:
    def __init__(self, path: pathlib.Path, options, hash_mb: int, timeout: int):
        self.path = path
        self.timeout = timeout
        self.process = subprocess.Popen(
            [str(path)], cwd=str(path.parent), stdin=subprocess.PIPE,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            text=True, bufsize=1,
        )
        self.lines = queue.Queue()
        self.reader = threading.Thread(target=self._read, daemon=True)
        self.reader.start()

        try:
            self.send("uci")
            startup = self.until(lambda line: line.strip() == "uciok")
            advertised = {
                match.group(1).casefold()
                for line in startup
                if (match := OPTION_LINE.match(line.strip()))
            }
            required = [("Hash", str(hash_mb)), ("Threads", "1"), *options]
            missing = [name for name, _ in required if name.casefold() not in advertised]
            if missing:
                raise RuntimeError(
                    f"{path} does not advertise option(s): {', '.join(missing)}"
                )
            for name, value in required:
                self.send(f"setoption name {name} value {value}")
            self.ready()
        except Exception:
            self.close()
            raise

    def _read(self):
        try:
            for line in self.process.stdout:
                self.lines.put(line.rstrip("\r\n"))
        finally:
            self.lines.put(None)

    def send(self, command: str):
        self.process.stdin.write(command + "\n")
        self.process.stdin.flush()

    def until(self, predicate):
        deadline = time.monotonic() + self.timeout
        seen = []
        while True:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise TimeoutError(f"{self.path} timed out")
            try:
                line = self.lines.get(timeout=remaining)
            except queue.Empty as error:
                raise TimeoutError(f"{self.path} timed out") from error
            if line is None:
                raise RuntimeError(f"{self.path} closed stdout")
            lowered = line.lower()
            if "no such option" in lowered or "unknown option" in lowered:
                raise RuntimeError(f"{self.path} rejected an option: {line}")
            seen.append(line)
            if predicate(line):
                return seen

    def ready(self):
        self.send("isready")
        self.until(lambda line: line.strip() == "readyok")

    def search(self, fen: str, depth: int) -> int:
        self.send("ucinewgame")
        self.ready()
        self.send(f"position fen {fen}")
        self.send(f"go depth {depth}")
        lines = self.until(lambda line: line.startswith("bestmove"))
        nodes = completed_nodes(lines, depth)
        return nodes

    def close(self):
        if not hasattr(self, "process"):
            return
        if self.process.poll() is None:
            try:
                self.send("quit")
                self.process.wait(timeout=10)
            except (OSError, ValueError, subprocess.TimeoutExpired):
                self.process.kill()
                self.process.wait(timeout=10)


def summarize_profile(position_rows, min_depth: int, max_depth: int):
    depths = list(range(min_depth, max_depth + 1))
    totals = {depth: sum(row["nodes"][str(depth)] for row in position_rows)
              for depth in depths}
    rows = []
    previous = previous_iteration = 0
    for depth in depths:
        nodes = totals[depth]
        iteration = nodes - previous if previous else None
        rows.append({
            "depth": depth,
            "nodes": nodes,
            "ratio": nodes / previous if previous else None,
            "iteration_nodes": iteration,
            "iteration_growth": (
                iteration / previous_iteration
                if iteration is not None and previous_iteration > 0 else None
            ),
        })
        if iteration is not None and iteration > 0:
            previous_iteration = iteration
        previous = nodes

    span = max_depth - min_depth
    details = []
    span_ratios = []
    for position in position_rows:
        nodes = position["nodes"]
        ratio = (nodes[str(max_depth)] / nodes[str(min_depth)]) ** (1.0 / span)
        span_ratios.append(ratio)
        details.append({**position, "span_ratio": ratio})

    return {
        "geometric_mean_ratio": (
            totals[max_depth] / totals[min_depth]
        ) ** (1.0 / span),
        "per_position_span": {
            "median": statistics.median(span_ratios),
            "min": min(span_ratios),
            "max": max(span_ratios),
        },
        "rows": rows,
        "positions_detail": details,
    }


def measure_arm(label, path, options, positions, min_depth, max_depth,
                hash_mb, timeout):
    position_rows = [
        {"index": index, "fen": fen, "nodes": {}}
        for index, fen in enumerate(positions, 1)
    ]
    for depth in range(min_depth, max_depth + 1):
        print(f"{label}: depth {depth}")
        session = EngineSession(path, options, hash_mb, timeout)
        try:
            for index, fen in enumerate(positions):
                position_rows[index]["nodes"][str(depth)] = session.search(fen, depth)
        finally:
            session.close()
    return {
        "label": label,
        "engine": str(path),
        "engine_sha256": file_sha256(path),
        "options": dict(options),
        "position_rows": position_rows,
    }


def finalize_arm(raw_arm, included_indices, min_depth, max_depth):
    rows = [row for row in raw_arm["position_rows"]
            if row["index"] in included_indices]
    return {
        key: value for key, value in raw_arm.items() if key != "position_rows"
    } | summarize_profile(rows, min_depth, max_depth)


def common_positions(raw_arms, min_depth, max_depth):
    included = set()
    excluded = []
    depths = range(min_depth, max_depth + 1)
    for row_index, primary_row in enumerate(raw_arms[0]["position_rows"]):
        missing_by_arm = {}
        for arm in raw_arms:
            row = arm["position_rows"][row_index]
            missing = [depth for depth in depths if row["nodes"][str(depth)] <= 0]
            if missing:
                missing_by_arm[arm["label"]] = missing
        if missing_by_arm:
            excluded.append({"index": primary_row["index"], "fen": primary_row["fen"],
                             "missing_depths": missing_by_arm})
        else:
            included.add(primary_row["index"])
    return included, excluded


def comparison(primary, reference):
    primary_rows = {row["depth"]: row["nodes"] for row in primary["rows"]}
    reference_rows = {row["depth"]: row["nodes"] for row in reference["rows"]}
    return {
        "reference": reference["label"],
        "branching_ratio": (
            primary["geometric_mean_ratio"] / reference["geometric_mean_ratio"]
        ),
        "nodes_ratio_by_depth": {
            str(depth): primary_rows[depth] / reference_rows[depth]
            for depth in primary_rows
        },
    }


def print_profile(profile):
    print(f"\n{profile['label']}")
    print(f"{'depth':>6} {'nodes':>16} {'ratio':>10}")
    for row in profile["rows"]:
        ratio = "-" if row["ratio"] is None else f"{row['ratio']:.3f}"
        print(f"{row['depth']:6d} {row['nodes']:16,d} {ratio:>10}")
    span = profile["per_position_span"]
    print(f"  aggregate {profile['geometric_mean_ratio']:.3f}; "
          f"per-position median {span['median']:.3f} "
          f"[{span['min']:.3f}, {span['max']:.3f}]")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--engine", required=True)
    parser.add_argument("--label", default="basilisk")
    parser.add_argument("--option", action="append", default=[], metavar="Name=Value")
    parser.add_argument("--reference", action="append", default=[],
                        metavar="LABEL=PATH[|Name=Value...]")
    parser.add_argument("--positions", default=str(REPO / "tools/diag/suite_v1.epd"))
    parser.add_argument("--limit", type=int, default=0, help="0 uses the full corpus")
    parser.add_argument("--min-depth", type=int, default=4)
    parser.add_argument("--max-depth", type=int, default=14)
    parser.add_argument("--hash", type=int, default=64)
    parser.add_argument("--timeout", type=int, default=1800, help="seconds per search")
    parser.add_argument("--out")
    args = parser.parse_args()

    if args.min_depth < 1 or args.max_depth <= args.min_depth:
        parser.error("require 1 <= min-depth < max-depth")
    if args.hash < 1 or args.limit < 0 or args.timeout < 1:
        parser.error("hash and timeout must be positive; limit must be non-negative")

    primary_options = []
    for option in args.option:
        if "=" not in option:
            parser.error(f"option {option!r} is not Name=Value")
        parsed = tuple(option.split("=", 1))
        if parsed[0].casefold() in {"hash", "threads"}:
            parser.error(f"{parsed[0]} is fixed globally, not per arm")
        primary_options.append(parsed)
    arms = [(args.label, pathlib.Path(args.engine).resolve(), primary_options)]
    try:
        arms.extend(parse_arm_spec(spec) for spec in args.reference)
    except ValueError as error:
        parser.error(str(error))
    labels = [label for label, _, _ in arms]
    if len(labels) != len({label.casefold() for label in labels}):
        parser.error("arm labels must be unique")
    for _, path, _ in arms:
        if not path.is_file():
            parser.error(f"engine does not exist: {path}")

    positions_path = pathlib.Path(args.positions).resolve()
    positions = load_positions(positions_path, args.limit)
    if not positions:
        parser.error("position corpus is empty")

    raw_arms = [
        measure_arm(label, path, options, positions, args.min_depth,
                    args.max_depth, args.hash, args.timeout)
        for label, path, options in arms
    ]
    included, excluded = common_positions(raw_arms, args.min_depth, args.max_depth)
    if not included:
        parser.error("no position completed every requested depth on every arm")
    profiles = [finalize_arm(arm, included, args.min_depth, args.max_depth)
                for arm in raw_arms]
    for profile in profiles:
        print_profile(profile)

    report = {
        "schema": "basilisk-branching-profile-v2",
        "created_utc": datetime.datetime.now(datetime.timezone.utc)
                                      .replace(microsecond=0).isoformat(),
        "positions": str(positions_path),
        "positions_sha256": file_sha256(positions_path),
        "position_count": len(positions),
        "analyzed_position_count": len(included),
        "excluded_positions": excluded,
        "hash_mb": args.hash,
        "threads": 1,
        "min_depth": args.min_depth,
        "max_depth": args.max_depth,
        "arms": profiles,
        "comparisons": [comparison(profiles[0], ref) for ref in profiles[1:]],
    }
    for item in report["comparisons"]:
        print(f"  {profiles[0]['label']}/{item['reference']}: branching "
              f"{item['branching_ratio']:.3f}")
    if args.out:
        output = pathlib.Path(args.out)
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        print(f"wrote {output}")


if __name__ == "__main__":
    main()
