#!/usr/bin/env python3
"""Build and enforce immutable classical-oracle WAC canaries."""

from __future__ import annotations

import argparse
import datetime
import hashlib
import json
import pathlib
import sys

import chess

import fixed_budget_probe as probe

SCHEMA = "basilisk-reference-canaries-v1"
ORACLE_STABLE_MAX = 6
DEPTH_SLACK = 2
NODE_BUDGET = 100_000
DEPTH_BUDGET = 12
QUIET_MATE_THREATS = {"WAC.001"}


def sha256(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


def load_json(path: pathlib.Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def quiet_key(item: dict) -> bool:
    if not item["bm"]:
        return False
    board = chess.Board(item["fen"])
    for uci in item["bm"]:
        move = chess.Move.from_uci(uci)
        if move not in board.legal_moves:
            raise ValueError(f"{item['id']} has illegal bm {uci}")
        if board.is_capture(move) or board.gives_check(move):
            return False
    return True


def validate_reports(depth: dict, nodes: dict, suite_items: list[dict],
                     labels: tuple[str, ...]) -> None:
    if depth.get("mode") != "depthpv" or depth.get("budget") != DEPTH_BUDGET:
        raise ValueError(f"depth report must be depthpv {DEPTH_BUDGET}")
    if nodes.get("mode") != "nodes" or nodes.get("budget") != NODE_BUDGET:
        raise ValueError(f"node report must be nodes {NODE_BUDGET}")
    for report in (depth, nodes):
        if report.get("hash_mb") != 64 or report.get("threads") != 1:
            raise ValueError("canary reports require Hash 64 and Threads 1")
        for label in labels:
            if label not in report.get("engines", {}):
                raise ValueError(f"report is missing engine {label!r}")
            if set(report["engines"][label]) != {item["id"] for item in suite_items}:
                raise ValueError(f"{label} report does not cover the exact suite")
    for label in labels:
        dmeta = depth.get("engine_meta", {}).get(label)
        nmeta = nodes.get("engine_meta", {}).get(label)
        if not dmeta or dmeta != nmeta:
            raise ValueError(f"{label} binary identity differs between reports")


def meets(depth_row: dict, node_row: dict, anchor_depth: int) -> bool:
    stable = depth_row.get("stable")
    return stable is not None and stable <= anchor_depth + DEPTH_SLACK \
        and node_row.get("solved") is True


def build_manifest(depth: dict, nodes: dict, suite: pathlib.Path,
                   baseline: str = "baseline", oracle: str = "oracle") -> dict:
    items = probe.parse_epd(str(suite))
    if len(items) != 300:
        raise ValueError(f"WAC suite has {len(items)} positions, expected 300")
    validate_reports(depth, nodes, items, (baseline, oracle))

    records = []
    for item in items:
        ident = item["id"]
        oracle_depth = depth["engines"][oracle][ident]
        oracle_nodes = nodes["engines"][oracle][ident]
        stable = oracle_depth.get("stable")
        reference_anchor = (
            stable is not None
            and (stable <= ORACLE_STABLE_MAX or ident in QUIET_MATE_THREATS)
            and oracle_nodes.get("solved") is True
        )
        if not reference_anchor:
            continue
        baseline_depth = depth["engines"][baseline][ident]
        baseline_nodes = nodes["engines"][baseline][ident]
        admitted = meets(baseline_depth, baseline_nodes, stable)
        records.append({
            "id": ident,
            "fen": item["fen"],
            "bm": item["bm"],
            "oracle_stable_depth": stable,
            "max_stable_depth": stable + DEPTH_SLACK,
            "oracle_nodes_solved": True,
            "baseline_stable_depth": baseline_depth.get("stable"),
            "baseline_nodes_solved": baseline_nodes.get("solved") is True,
            "quiet_key": quiet_key(item),
            "quiet_mate_threat": ident in QUIET_MATE_THREATS,
            "required": admitted,
        })

    canaries = [record for record in records if record["required"]]
    if not canaries:
        raise ValueError("no baseline-qualified canaries")
    if not any(record["quiet_key"] for record in canaries):
        raise ValueError("canary set has no quiet key moves")
    if not any(record["quiet_mate_threat"] for record in records):
        raise ValueError("reference anchors do not include the quiet mate threat")

    return {
        "schema": SCHEMA,
        "created_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "suite": {
            "path": str(suite).replace("\\", "/"),
            "sha256": sha256(suite),
            "positions": len(items),
        },
        "contract": {
            "oracle_stable_max": ORACLE_STABLE_MAX,
            "depth_slack": DEPTH_SLACK,
            "depth_budget": DEPTH_BUDGET,
            "node_budget": NODE_BUDGET,
            "hash_mb": 64,
            "threads": 1,
            "quiet_mate_threats": sorted(QUIET_MATE_THREATS),
            "new_passes_are_required": False,
        },
        "binaries": {
            baseline: depth["engine_meta"][baseline],
            oracle: depth["engine_meta"][oracle],
        },
        "summary": {
            "reference_anchors": len(records),
            "required_canaries": len(canaries),
            "baseline_gaps": len(records) - len(canaries),
            "quiet_key_canaries": sum(record["quiet_key"] for record in canaries),
            "quiet_mate_threat_canaries": sum(
                record["quiet_mate_threat"] for record in canaries
            ),
            "quiet_mate_threat_anchors": sum(
                record["quiet_mate_threat"] for record in records
            ),
        },
        "records": records,
    }


def evaluate(manifest: dict, depth: dict, nodes: dict,
             suite: pathlib.Path, label: str) -> dict:
    if manifest.get("schema") != SCHEMA:
        raise ValueError("unsupported canary manifest schema")
    if sha256(suite) != manifest["suite"]["sha256"]:
        raise ValueError("WAC suite differs from the frozen canary manifest")
    items = probe.parse_epd(str(suite))
    validate_reports(depth, nodes, items, (label,))
    regressions = []
    new_passes = []
    for record in manifest["records"]:
        ident = record["id"]
        passed = meets(
            depth["engines"][label][ident],
            nodes["engines"][label][ident],
            record["oracle_stable_depth"],
        )
        result = {
            "id": ident,
            "stable_depth": depth["engines"][label][ident].get("stable"),
            "nodes_solved": nodes["engines"][label][ident].get("solved") is True,
            "max_stable_depth": record["max_stable_depth"],
        }
        if record["required"] and not passed:
            regressions.append(result)
        elif not record["required"] and passed:
            new_passes.append(result)
    return {
        "schema": "basilisk-canary-check-v1",
        "manifest_schema": SCHEMA,
        "candidate": depth["engine_meta"][label],
        "required": manifest["summary"]["required_canaries"],
        "passed": manifest["summary"]["required_canaries"] - len(regressions),
        "regressions": regressions,
        "new_passes": new_passes,
        "pass": not regressions,
    }


def write_result(path: pathlib.Path, result: dict, exclusive: bool) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    mode = "x" if exclusive else "w"
    try:
        with path.open(mode, encoding="utf-8") as stream:
            json.dump(result, stream, indent=2)
            stream.write("\n")
    except FileExistsError as error:
        raise SystemExit(
            f"refusing to overwrite frozen canary manifest {path}; mint a new version"
        ) from error


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)
    build = sub.add_parser("build")
    build.add_argument("--depth", required=True, type=pathlib.Path)
    build.add_argument("--nodes", required=True, type=pathlib.Path)
    build.add_argument("--suite", default="src/wac.epd", type=pathlib.Path)
    build.add_argument("--baseline", default="baseline")
    build.add_argument("--oracle", default="oracle")
    build.add_argument("--out", required=True, type=pathlib.Path)
    check = sub.add_parser("check")
    check.add_argument("--manifest", required=True, type=pathlib.Path)
    check.add_argument("--depth", required=True, type=pathlib.Path)
    check.add_argument("--nodes", required=True, type=pathlib.Path)
    check.add_argument("--suite", default="src/wac.epd", type=pathlib.Path)
    check.add_argument("--engine", default="candidate")
    check.add_argument("--out", required=True, type=pathlib.Path)
    args = parser.parse_args()

    if args.command == "build":
        result = build_manifest(
            load_json(args.depth), load_json(args.nodes), args.suite.resolve(),
            args.baseline, args.oracle,
        )
        print(json.dumps(result["summary"], indent=2))
    else:
        result = evaluate(
            load_json(args.manifest), load_json(args.depth),
            load_json(args.nodes), args.suite.resolve(), args.engine,
        )
        print(json.dumps({
            "required": result["required"], "passed": result["passed"],
            "regressions": len(result["regressions"]),
            "new_passes": len(result["new_passes"]), "pass": result["pass"],
        }, indent=2))
    write_result(args.out, result, exclusive=args.command == "build")
    return 0 if args.command == "build" or result["pass"] else 1


if __name__ == "__main__":
    sys.exit(main())
