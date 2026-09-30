#!/usr/bin/env python3
"""Prove every matched AblationMask bit changes fixed-depth search work."""

from __future__ import annotations

import argparse
import datetime
import hashlib
import json
import pathlib
import queue
import re
import subprocess
import threading
import time

BITS = (
    "razoring",
    "reverse_futility",
    "null_move",
    "probcut",
    "iir",
    "shallow_move_pruning",
    "extensions",
    "lmr",
)
INFO_LINE = re.compile(r"^info depth (\d+).*?\bnodes (\d+)")
OPTION_LINE = re.compile(r"^option name (.*?) type ", re.IGNORECASE)


def sha256(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def parse_engine_spec(spec: str):
    if "=" not in spec:
        raise ValueError(f"engine {spec!r} must be LABEL=PATH[|Name=Value...]")
    label, remainder = spec.split("=", 1)
    path, *raw_options = remainder.split("|")
    options = []
    for raw in raw_options:
        if "=" not in raw:
            raise ValueError(f"option {raw!r} is not Name=Value")
        options.append(tuple(raw.split("=", 1)))
    if not label or not path:
        raise ValueError("engine label and path must be non-empty")
    return label, pathlib.Path(path).resolve(), options


def load_positions(path: pathlib.Path, limit: int) -> list[str]:
    positions = []
    for line in path.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        positions.append(line.split(" ; ", 1)[0])
    return positions[:limit] if limit > 0 else positions


def completed_nodes(lines: list[str], depth: int) -> int:
    nodes = 0
    for line in lines:
        padded = f" {line} "
        if " lowerbound " in padded or " upperbound " in padded:
            continue
        match = INFO_LINE.match(line.strip())
        if match and int(match.group(1)) == depth:
            nodes = int(match.group(2))
    return nodes


class EngineSession:
    def __init__(self, path: pathlib.Path, options, mask: int,
                 hash_mb: int, timeout: int):
        self.path = path
        self.timeout = timeout
        self.process = subprocess.Popen(
            [str(path)], cwd=str(path.parent), stdin=subprocess.PIPE,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            text=True, bufsize=1,
        )
        self.lines: queue.Queue[str | None] = queue.Queue()
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
            required = [
                ("Hash", str(hash_mb)), ("Threads", "1"),
                ("AblationMask", str(mask)), *options,
            ]
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

    def until(self, predicate) -> list[str]:
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
        if nodes <= 0:
            raise RuntimeError(f"{self.path} did not complete depth {depth}")
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


def measure(path: pathlib.Path, options, positions: list[str], depth: int,
            hash_mb: int, timeout: int) -> dict:
    masks = (0, *(1 << bit for bit in range(len(BITS))))
    rows = []
    for mask in masks:
        session = EngineSession(path, options, mask, hash_mb, timeout)
        try:
            nodes = [session.search(fen, depth) for fen in positions]
        finally:
            session.close()
        rows.append({"mask": mask, "nodes": nodes, "total_nodes": sum(nodes)})

    baseline = rows[0]
    checks = []
    for bit, name in enumerate(BITS):
        row = rows[bit + 1]
        changed = [
            index + 1 for index, (base, masked) in enumerate(
                zip(baseline["nodes"], row["nodes"])
            ) if base != masked
        ]
        checks.append({
            "bit": bit,
            "name": name,
            "mask": 1 << bit,
            "baseline_total_nodes": baseline["total_nodes"],
            "masked_total_nodes": row["total_nodes"],
            "delta_nodes": row["total_nodes"] - baseline["total_nodes"],
            "changed_positions": changed,
            "live": bool(changed),
        })
    dead = [check["name"] for check in checks if not check["live"]]
    if dead:
        raise RuntimeError(f"dead AblationMask bit(s): {', '.join(dead)}")
    return {"runs": rows, "checks": checks}


def main() -> int:
    repo = pathlib.Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--engine", action="append", required=True,
                        help="LABEL=PATH[|Name=Value...] (repeatable)")
    parser.add_argument("--positions", type=pathlib.Path,
                        default=repo / "tools" / "diag" / "suite_v1.epd")
    parser.add_argument("--limit", type=int, default=8)
    parser.add_argument("--depth", type=int, default=9)
    parser.add_argument("--hash", type=int, default=64)
    parser.add_argument("--timeout", type=int, default=60)
    parser.add_argument("--out", required=True, type=pathlib.Path)
    args = parser.parse_args()

    positions_path = args.positions.resolve()
    positions = load_positions(positions_path, args.limit)
    if not positions:
        raise ValueError("position corpus is empty")

    engines = []
    for spec in args.engine:
        label, path, options = parse_engine_spec(spec)
        result = measure(path, options, positions, args.depth, args.hash, args.timeout)
        engines.append({
            "label": label,
            "path": str(path),
            "sha256": sha256(path),
            "options": dict(options),
            **result,
        })
        print(f"{label}: all {len(BITS)} bits live")

    report = {
        "schema": "basilisk-matched-ablation-liveness-v1",
        "created_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "bit_order": [{"bit": bit, "name": name} for bit, name in enumerate(BITS)],
        "positions": {
            "path": str(positions_path),
            "sha256": sha256(positions_path),
            "count": len(positions),
            "records": positions,
        },
        "depth": args.depth,
        "hash_mb": args.hash,
        "threads": 1,
        "engines": engines,
    }
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"report: {args.out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
