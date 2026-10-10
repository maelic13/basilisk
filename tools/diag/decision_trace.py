#!/usr/bin/env python3
"""Capture and validate Basilisk's bounded diagnostic decision trace."""

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

PREFIX = "info string trace "
# Version 1 is the legacy kernel's record; version 2 appends the reduced depth
# move-loop pruning read, whether the move-count rule had ended quiet delivery,
# and the pruning family, and says in its header whether cutoff_count has a
# producer.
V1_FIELDS = {
    "seq", "event", "ply", "depth", "move", "alpha", "beta",
    "estimated_score", "improving", "correction", "history",
    "move_count", "reduction", "cutoff_count", "window_alpha",
    "window_beta", "margin", "result",
}
V2_FIELDS = V1_FIELDS | {"lmr_depth", "skip_quiets", "family"}
REQUIRED_FIELDS = {"1": V1_FIELDS, "2": V2_FIELDS}
TEXT_FIELDS = {"event", "move", "family"}
FAMILIES = {
    "none", "razor", "rfp", "skip_quiets", "cont_hist", "quiet_futility",
    "quiet_see", "capture_futility", "capture_see",
}


def sha256(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def key_values(text: str) -> dict[str, str]:
    fields: dict[str, str] = {}
    for token in text.split():
        if "=" not in token:
            raise ValueError(f"malformed trace token: {token!r}")
        key, value = token.split("=", 1)
        if key in fields:
            raise ValueError(f"duplicate trace field: {key}")
        fields[key] = value
    return fields


def parse_trace(lines: list[str]) -> dict:
    trace = [line[len(PREFIX):] for line in lines if line.startswith(PREFIX)]
    if not trace or not trace[0].startswith("begin "):
        raise ValueError("trace begin record missing")
    if not trace[-1].startswith("end "):
        raise ValueError("trace end record missing")

    header = key_values(trace[0][len("begin "):])
    footer = key_values(trace[-1][len("end "):])
    version = header.get("version")
    if version not in REQUIRED_FIELDS or header.get("plies") != "1-2":
        raise ValueError("unsupported trace contract")
    availability = header.get("cutoff_count")
    if availability not in ("unavailable", "available") or (
        version == "1" and availability != "unavailable"
    ):
        raise ValueError("cutoff_count availability changed; update the consumer")
    required = REQUIRED_FIELDS[version]
    if header.get("value_none") != "32002" or header.get("bool_unknown") != "-1":
        raise ValueError("trace sentinel contract changed")
    if footer.get("status") != "ok":
        raise ValueError(f"trace is incomplete: status={footer.get('status', 'missing')}")

    records = []
    for expected_seq, text in enumerate(trace[1:-1]):
        if not text.startswith("record "):
            raise ValueError(f"unexpected trace line: {text!r}")
        fields = key_values(text[len("record "):])
        missing = required - fields.keys()
        extra = fields.keys() - required
        if missing or extra:
            raise ValueError(
                f"trace schema mismatch: missing={sorted(missing)} extra={sorted(extra)}"
            )
        record = {
            key: value if key in TEXT_FIELDS else int(value)
            for key, value in fields.items()
        }
        if record["seq"] != expected_seq:
            raise ValueError(f"non-consecutive trace sequence at {expected_seq}")
        if record["ply"] not in (1, 2):
            raise ValueError(f"out-of-contract trace ply: {record['ply']}")
        if availability == "unavailable" and record["cutoff_count"] != -1:
            raise ValueError("cutoff_count must use the documented -1 sentinel")
        if record["cutoff_count"] < -1:
            raise ValueError("cutoff_count below the -1 sentinel")
        if version == "2":
            if record["skip_quiets"] not in (-1, 0, 1):
                raise ValueError("skip_quiets must be -1, 0 or 1")
            if record["family"] not in FAMILIES:
                raise ValueError(f"unknown pruning family: {record['family']}")
        if (record["alpha"], record["beta"]) != (
            record["window_alpha"], record["window_beta"]
        ):
            raise ValueError("window fields disagree with alpha/beta")
        records.append(record)

    if int(footer.get("records", "-1")) != len(records):
        raise ValueError("trace footer record count mismatch")
    return {"header": header, "records": records, "footer": footer}


class EngineSession:
    def __init__(self, engine: pathlib.Path, timeout: int):
        self.engine = engine
        self.timeout = timeout
        self.process = subprocess.Popen(
            [str(engine)], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT, text=True, bufsize=1,
        )
        self.lines: queue.Queue[str | None] = queue.Queue()
        self.reader = threading.Thread(target=self._read, daemon=True)
        self.reader.start()

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
                raise TimeoutError(f"{self.engine} timed out")
            try:
                line = self.lines.get(timeout=remaining)
            except queue.Empty as error:
                raise TimeoutError(f"{self.engine} timed out") from error
            if line is None:
                raise RuntimeError(f"{self.engine} closed stdout")
            seen.append(line)
            if predicate(line):
                return seen

    def close(self):
        if self.process.poll() is None:
            try:
                self.send("quit")
                self.process.wait(timeout=10)
            except (OSError, ValueError, subprocess.TimeoutExpired):
                self.process.kill()
                self.process.wait(timeout=10)


def capture(engine: pathlib.Path, position: str, root_move: str,
            depth: int, timeout: int) -> tuple[dict, str]:
    position_command = "position startpos" if position == "startpos" \
        else f"position fen {position}"
    session = EngineSession(engine, timeout)
    all_lines = []
    try:
        session.send("uci")
        startup = session.until(lambda line: line == "uciok")
        all_lines.extend(startup)
        if "option name DecisionTrace type check default false" not in startup:
            raise RuntimeError("engine does not advertise DecisionTrace")
        for command in (
            "setoption name Threads value 1",
            "setoption name Diag value true",
            "setoption name DecisionTrace value true",
            "isready",
        ):
            session.send(command)
        all_lines.extend(session.until(lambda line: line == "readyok"))
        session.send(position_command)
        session.send(f"go depth {depth} searchmoves {root_move}")
        all_lines.extend(session.until(lambda line: line.startswith("bestmove")))
    finally:
        session.close()
    return parse_trace(all_lines), "\n".join(all_lines)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--engine", required=True, type=pathlib.Path)
    parser.add_argument("--position", default="startpos",
                        help="'startpos' or a complete FEN")
    parser.add_argument("--root-move", required=True)
    parser.add_argument("--depth", type=int, default=8)
    parser.add_argument("--timeout", type=int, default=60)
    parser.add_argument("--out", required=True, type=pathlib.Path)
    args = parser.parse_args()

    engine = args.engine.resolve()
    trace, _ = capture(engine, args.position, args.root_move,
                       args.depth, args.timeout)
    report = {
        "schema": f"basilisk-decision-trace-v{trace['header']['version']}",
        "created_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "engine": {"path": str(engine), "sha256": sha256(engine)},
        "request": {
            "position": args.position,
            "root_move": args.root_move,
            "depth": args.depth,
            "threads": 1,
            "plies": [1, 2],
        },
        "trace": trace,
    }
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"trace: {len(trace['records'])} records -> {args.out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
