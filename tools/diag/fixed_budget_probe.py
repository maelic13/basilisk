#!/usr/bin/env python3
"""Run fixed-budget depth/PV and node probes over an EPD suite.

A nominal ply is not a unit of work across engines. This runner gives every
engine Hash 64, one thread and fresh-game state per position, and preserves
the per-position evidence needed by the reference-anchored canary policy.

Usage:
  python tools/diag/fixed_budget_probe.py depthpv 11 src/wac.epd out.json \
      baseline=build/release/basilisk.exe \
      "oracle=oracle.exe|Use Basilisk HCE=true"
"""

from __future__ import annotations

import json
import hashlib
import os
import re
import subprocess
import sys
import time

import chess

OPTION_LINE = re.compile(r"^option name (.*?) type ", re.IGNORECASE)


def sha256(path):
    digest = hashlib.sha256()
    with open(path, "rb") as stream:
        for block in iter(lambda: stream.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


def parse_epd(path):
    items = []
    with open(path, encoding="utf-8") as stream:
        for line in stream:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            fields = line.split(" ; ") if " ; " in line else [line]
            head = fields[0]
            parts = head.split()
            fen4 = " ".join(parts[:4])
            bm = []
            ident = None
            match = re.search(r"\bbm\s+([^;]+);", line)
            if match:
                bm = match.group(1).split()
            match = re.search(r'id\s+"([^"]+)"', line)
            if match:
                ident = match.group(1)
            if ident is None:
                ident = f"pos{len(items) + 1}"
            fen = fen4 + " 0 1"
            board = chess.Board(fen)
            bm_uci = []
            for san in bm:
                try:
                    bm_uci.append(board.parse_san(san).uci())
                except Exception:
                    pass
            items.append({"id": ident, "fen": fen, "bm": bm_uci})
    return items


def parse_engine_spec(arg):
    """LABEL=PATH|Name=Value|... -> (label, path, options)."""
    if "=" not in arg:
        raise SystemExit(f"engine {arg!r} must be LABEL=PATH[|Name=Value...]")
    label, rest = arg.split("=", 1)
    path, *pairs = rest.split("|")
    options = []
    for pair in pairs:
        if "=" not in pair:
            raise SystemExit(f"engine option {pair!r} in {arg!r} is not Name=Value")
        name, value = pair.split("=", 1)
        if name.casefold() in {"hash", "threads"}:
            raise SystemExit(f"{name} is fixed by the probe and cannot be overridden")
        options.append((name, value))
    if not label or not path:
        raise SystemExit("engine label and path must be non-empty")
    return label, path, options


def option_rejected(line):
    return "no such option" in line.lower() or "unknown option" in line.lower()


class Engine:
    def __init__(self, path, options=()):
        path = os.path.abspath(path).replace("\\", "/")
        self.p = subprocess.Popen(
            [path], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT, text=True, bufsize=1,
            cwd=path.rsplit("/", 1)[0] if "/" in path else None,
        )
        self.send("uci")
        startup = self.wait("uciok")
        advertised = {
            match.group(1).casefold()
            for line in startup
            if (match := OPTION_LINE.match(line.strip()))
        }
        required = [("Hash", "64"), ("Threads", "1"), *options]
        missing = [name for name, _ in required if name.casefold() not in advertised]
        if missing:
            self.p.kill()
            self.p.wait(timeout=10)
            raise RuntimeError(
                f"{path} does not advertise option(s): {', '.join(missing)}"
            )
        for name, value in required:
            self.send(f"setoption name {name} value {value}")
        self.send("isready")
        self.wait("readyok")

    def send(self, command):
        self.p.stdin.write(command + "\n")
        self.p.stdin.flush()

    def wait(self, token):
        seen = []
        while True:
            line = self.p.stdout.readline()
            if not line:
                raise SystemExit("engine died")
            if option_rejected(line):
                raise SystemExit(f"engine rejected an option: {line.strip()}")
            seen.append(line)
            if line.startswith(token):
                return seen

    def search(self, fen, go):
        self.send("ucinewgame")
        self.send("isready")
        self.wait("readyok")
        self.send(f"position fen {fen}")
        started = time.perf_counter()
        self.send(go)
        infos = []
        while True:
            line = self.p.stdout.readline()
            if not line:
                raise SystemExit("engine died")
            if line.startswith("info") and " depth " in line and " pv " in line:
                infos.append(line.strip())
            if line.startswith("bestmove"):
                best = line.split()[1]
                break
        return infos, best, time.perf_counter() - started

    def quit(self):
        self.send("quit")
        self.p.wait(timeout=10)


def info_fields(line):
    tokens = line.split()
    fields = {}
    for key in ("depth", "seldepth", "nodes", "time"):
        if key in tokens:
            fields[key] = int(tokens[tokens.index(key) + 1])
    if "pv" in tokens:
        fields["pv1"] = tokens[tokens.index("pv") + 1]
    if "multipv" in tokens:
        fields["multipv"] = int(tokens[tokens.index("multipv") + 1])
    return fields


def last_completed(infos):
    """Return the last completed iteration, excluding aspiration bounds."""
    exact = [
        line for line in infos
        if " lowerbound " not in f" {line} " and " upperbound " not in f" {line} "
    ]
    return info_fields(exact[-1]) if exact else {}


def require_complete_depths(depths, budget, label, ident):
    expected = list(range(1, budget + 1))
    if depths != expected:
        raise RuntimeError(
            f"{label} {ident} completed depths {depths}, "
            f"expected every depth 1..{budget}"
        )


def run(mode, budget, suite, engines):
    items = parse_epd(suite)
    result = {
        "mode": mode, "budget": budget, "suite": suite,
        "engines": {}, "engine_meta": {}, "options": {},
        "hash_mb": 64, "threads": 1,
    }
    for label, (path, options) in engines.items():
        resolved = os.path.abspath(path)
        result["engine_meta"][label] = {
            "path": resolved.replace("\\", "/"),
            "sha256": sha256(resolved),
        }
        result["options"][label] = dict(options)
        engine = Engine(path, options)
        per_position = {}
        try:
            for item in items:
                if mode == "depthpv":
                    infos, best, seconds = engine.search(item["fen"], f"go depth {budget}")
                    by_depth = {}
                    for line in infos:
                        if " lowerbound " in f" {line} " or " upperbound " in f" {line} ":
                            continue
                        fields = info_fields(line)
                        if fields.get("multipv", 1) != 1 or "pv1" not in fields:
                            continue
                        by_depth[fields["depth"]] = fields["pv1"]
                    depths = sorted(by_depth)
                    require_complete_depths(depths, budget, label, item["id"])
                    first = next(
                        (depth for depth in depths if by_depth[depth] in item["bm"]), None
                    ) if item["bm"] else None
                    stable = next(
                        (depth for depth in depths
                         if all(by_depth[later] in item["bm"]
                                for later in depths if later >= depth)),
                        None,
                    ) if item["bm"] else None
                    per_position[item["id"]] = {
                        "by_depth": {str(depth): by_depth[depth] for depth in depths},
                        "bestmove": best, "bm": item["bm"], "first": first,
                        "stable": stable, "secs": round(seconds, 3),
                    }
                else:
                    infos, best, seconds = engine.search(item["fen"], f"go nodes {budget}")
                    last = last_completed(infos)
                    if not last:
                        raise RuntimeError(f"{label} {item['id']} completed no iteration")
                    per_position[item["id"]] = {
                        "depth": last.get("depth"), "seldepth": last.get("seldepth"),
                        "nodes": last.get("nodes"), "ms": last.get("time"),
                        "bestmove": best, "secs": round(seconds, 3), "bm": item["bm"],
                        "solved": (best in item["bm"]) if item["bm"] else None,
                    }
                print(f"{label} {item['id']}", file=sys.stderr)
        finally:
            engine.quit()
        result["engines"][label] = per_position

    summary = {}
    for label, per_position in result["engines"].items():
        if mode == "depthpv":
            summary[label] = {
                "positions": len(per_position),
                "solved_first_by_depth": {
                    str(depth): sum(
                        1 for value in per_position.values()
                        if value["first"] is not None and value["first"] <= depth
                    )
                    for depth in range(1, budget + 1)
                },
                "stable_by_depth": {
                    str(depth): sum(
                        1 for value in per_position.values()
                        if value["stable"] is not None and value["stable"] <= depth
                    )
                    for depth in range(1, budget + 1)
                },
                "secs": round(sum(value["secs"] for value in per_position.values()), 1),
            }
        else:
            depths = [value["depth"] for value in per_position.values()]
            summary[label] = {
                "positions": len(per_position),
                "mean_depth": round(sum(depths) / len(depths), 2),
                "solved": sum(1 for value in per_position.values() if value["solved"]),
                "mean_seldepth": round(
                    sum(value["seldepth"] for value in per_position.values())
                    / len(per_position), 2
                ),
                "total_nodes": sum(value["nodes"] for value in per_position.values()),
                "total_ms": sum(value["ms"] for value in per_position.values()),
            }
    result["summary"] = summary
    return result


def main(argv):
    if len(argv) < 5:
        raise SystemExit(__doc__)
    mode, budget, suite, out = argv[0], int(argv[1]), argv[2], argv[3]
    if mode not in ("depthpv", "nodes"):
        raise SystemExit(f"unknown mode {mode!r}; expected depthpv or nodes")
    engines = {}
    for arg in argv[4:]:
        label, path, options = parse_engine_spec(arg)
        engines[label] = (path, options)
    if not engines:
        raise SystemExit("no engines given (label=path ...)")
    result = run(mode, budget, suite, engines)
    with open(out, "w", encoding="utf-8") as stream:
        json.dump(result, stream, indent=2)
        stream.write("\n")
    print(json.dumps(result["summary"], indent=2))


if __name__ == "__main__":
    main(sys.argv[1:])
