#!/usr/bin/env python3
"""Is the narrow-window rook failure resolvable at all? (PLAN 6.5.a)

6.5.a's characterisation established the signature: the engine holds a won rook
ending while about fourteen of roughly twenty legal moves preserve the win, and
throws it almost only at nodes where about two and a half do
(`rook_ending_failure_profile.py`). The conclusion drawn from that was "no
gradient can reliably resolve two moves out of twenty".

That conclusion is an assertion, not a measurement, and it has a decisive test.
If a strong reference evaluator resolves those same narrow nodes at the SAME
node budget, then the class is resolvable and the deficit is Basilisk's
knowledge, not an inherent limit. If nobody resolves them at 60k but Basilisk
does at 600k, the limit is search depth. If nothing resolves them, the family
should be descoped to draw scaling.

The comparison must be PAIRED, and that is why this tool exists rather than
another whole-game replay. Two engines replaying the same root diverge at the
first differing move, after which they are answering different questions and
any preservation-rate difference is partly a difference of trajectory. So:

  extract   replay the clean wins ONCE with one engine, and freeze every
            White-to-move node whose WDL is still a clean win, together with
            how many legal moves preserve it (the narrowness).
  arm       ask an engine, at some node budget, for a single move at each
            frozen node, and record whether that move preserved the win.
  summary   compare arms on the SAME nodes, bucketed by narrowness, with the
            wide buckets acting as the control: an arm that is simply better
            everywhere will show it there too.

Every arm runs with a fresh game token per node, so no arm gets TT carry-over
that another lacks, and with `SyzygyPath` cleared, so this measures evaluators
and not tables. The frozen node set comes from one engine's trajectory and is
therefore a sample of the positions THAT engine reaches; that is the intended
population -- the question is whether the nodes Basilisk actually faces are
resolvable -- but it is a selection and is recorded as one.

Node-budget parity across different engines is approximate: a node is not the
same amount of work in two programs. This follows the 6.0.b convention of
giving both engines 60,000 nodes and inherits its caveat.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

import chess
import chess.engine
import chess.syzygy

# Pre-registered, fixed before any arm was run. NARROW is the failure class the
# profile identified (threw at ~2.5 winning moves); WIDE is the control.
NARROW_MAX = 3
WIDE_MIN = 10
BUCKETS = (("narrow", 1, NARROW_MAX), ("mid", NARROW_MAX + 1, WIDE_MIN - 1),
           ("wide", WIDE_MIN, 10 ** 6))


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def open_tablebases(paths):
    tb = chess.syzygy.open_tablebase(str(paths[0]))
    for extra in paths[1:]:
        tb.add_directory(str(extra))
    return tb


def configure(engine, hash_mb: int) -> None:
    """One thread, small hash, and NO tablebase access. See endgame_truth.py."""
    options = {}
    if "Hash" in engine.options:
        options["Hash"] = hash_mb
    if "Threads" in engine.options:
        options["Threads"] = 1
    if "SyzygyPath" in engine.options:
        options["SyzygyPath"] = ""
    if options:
        engine.configure(options)


def winning_moves(board: chess.Board, tb):
    """Legal moves that leave the opponent lost, i.e. that keep the clean win.

    None means the tablebase could not answer for some move, in which case the
    node is unusable and is dropped rather than guessed at.
    """
    keep = []
    for candidate in board.legal_moves:
        board.push(candidate)
        try:
            ok = -tb.probe_wdl(board) == 2
        except (chess.syzygy.MissingTableError, KeyError, ValueError):
            board.pop()
            return None
        board.pop()
        if ok:
            keep.append(candidate)
    return keep


def bucket_of(win_moves: int) -> str:
    for name, low, high in BUCKETS:
        if low <= win_moves <= high:
            return name
    raise ValueError(win_moves)


def cmd_extract(args) -> int:
    tb = open_tablebases(args.syzygy)
    records = []
    for path in args.cohort:
        manifest = json.loads(path.read_text(encoding="utf-8"))
        for record in manifest["records"]:
            if record["family"] in args.family and record["theory_wdl"] == 2:
                records.append(record)
    if not records:
        raise SystemExit("no clean wins matched the requested families")

    engine = chess.engine.SimpleEngine.popen_uci(str(args.engine))
    configure(engine, args.hash)
    nodes = []
    dropped = 0
    try:
        nodes, dropped = _replay(engine, tb, records, args)
    finally:
        engine.quit()                         # see the note in cmd_arm
        tb.close()

    out = {
        "schema": "basilisk-narrow-node-set-v1",
        "purpose": "PLAN 6.5.a: frozen decision nodes for a paired narrow-window test",
        "extract_engine": str(args.engine),
        "extract_engine_sha256": sha256(args.engine),
        "extract_nodes": args.nodes,
        "hash_mb": args.hash,
        "max_plies": args.max_plies,
        "families": sorted(args.family),
        "cohorts": [{"path": str(p), "sha256": sha256(p)} for p in args.cohort],
        "selection_note": "nodes lie on the extract engine's own trajectory; "
                          "this samples the positions THAT engine reaches",
        "unusable_nodes_dropped": dropped,
        "buckets": {name: [low, high] for name, low, high in BUCKETS},
        "node_count": len(nodes),
        "nodes": nodes,
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(out, indent=2) + "\n",
                           encoding="utf-8", newline="\n")
    counts = {name: sum(bucket_of(n["win_moves"]) == name for n in nodes)
              for name, _, _ in BUCKETS}
    print("\nFroze %d decision nodes (%d dropped as unprobeable)"
          % (len(nodes), dropped))
    print("  by narrowness: " + ", ".join("%s %d" % kv for kv in counts.items()))
    print("Wrote %s" % args.output.resolve())
    return 0


def _replay(engine, tb, records, args):
    nodes = []
    dropped = 0
    for index, record in enumerate(records, 1):
        board = chess.Board(record["fen"])
        token = object()                      # persistent TT within one game
        for ply in range(args.max_plies):
            if board.is_game_over() or chess.popcount(board.occupied) > 7:
                break
            white = board.turn == chess.WHITE
            try:
                wdl = tb.probe_wdl(board)
            except (chess.syzygy.MissingTableError, KeyError, ValueError):
                break
            still_won = (wdl if white else -wdl) == 2
            move = engine.play(board, chess.engine.Limit(nodes=args.nodes),
                               game=token).move
            if move is None:
                break
            if white and still_won:
                keep = winning_moves(board, tb)
                if keep is None:
                    dropped += 1
                    board.push(move)
                    continue
                if keep:
                    nodes.append({
                        "id": record["id"],
                        "family": record["family"],
                        "ply": ply,
                        "fen": board.fen(),
                        "win_moves": len(keep),
                        "legal_moves": board.legal_moves.count(),
                        "extract_move": move.uci(),
                        "extract_preserved": move in keep,
                    })
                board.push(move)
                if keep and move not in keep:
                    break                     # win is gone; no further win nodes
                continue
            board.push(move)
        print("  [%d/%d] %s: %d nodes so far"
              % (index, len(records), record["id"], len(nodes)), flush=True)
    return nodes, dropped


def cmd_arm(args) -> int:
    node_set = json.loads(args.nodes_file.read_text(encoding="utf-8"))
    tb = open_tablebases(args.syzygy)
    engine = chess.engine.SimpleEngine.popen_uci(str(args.engine))
    configure(engine, args.hash)

    # The engine is a child process and python-chess services it from a
    # background event loop, so an exception that escapes this block leaves the
    # interpreter hanging at shutdown rather than reporting the fault. Every
    # exit path must quit the engine.
    results = []
    try:
        for index, node in enumerate(node_set["nodes"], 1):
            board = chess.Board(node["fen"])
            keep = winning_moves(board, tb)
            if keep is None:
                raise SystemExit("node %s@%d became unprobeable"
                                 % (node["id"], node["ply"]))
            if len(keep) != node["win_moves"]:
                raise SystemExit(
                    "node %s@%d win_moves changed %d -> %d; tablebase set differs"
                    % (node["id"], node["ply"], node["win_moves"], len(keep)))
            # Fresh game token per node: every arm decides from a blank slate.
            move = engine.play(board, chess.engine.Limit(nodes=args.nodes),
                               game=object()).move
            results.append({
                "id": node["id"], "ply": node["ply"], "family": node["family"],
                "win_moves": node["win_moves"], "legal_moves": node["legal_moves"],
                "move": move.uci() if move else None,
                "preserved": bool(move is not None and move in keep),
            })
            if index % 100 == 0:
                print("  %d/%d" % (index, node_set["node_count"]), flush=True)
    finally:
        engine.quit()
        tb.close()

    kept = sum(r["preserved"] for r in results)
    out = {
        "schema": "basilisk-narrow-node-arm-v1",
        "label": args.label,
        "engine": str(args.engine),
        "engine_sha256": sha256(args.engine),
        "nodes_per_move": args.nodes,
        "hash_mb": args.hash,
        "node_set_sha256": sha256(args.nodes_file),
        "node_count": len(results),
        "preserved": kept,
        "results": results,
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(out, indent=2) + "\n",
                           encoding="utf-8", newline="\n")
    print("\n%s: preserved %d/%d (%.2f%%)"
          % (args.label, kept, len(results), 100 * kept / len(results)))
    print("Wrote %s" % args.output.resolve())
    return 0


def mcnemar(a, b):
    """Paired discordant counts and a z for two arms over the same nodes."""
    only_a = sum(1 for x, y in zip(a, b) if x and not y)
    only_b = sum(1 for x, y in zip(a, b) if y and not x)
    n = only_a + only_b
    z = (only_a - only_b) / (n ** 0.5) if n else 0.0
    return only_a, only_b, z


def cmd_summary(args) -> int:
    arms = [json.loads(p.read_text(encoding="utf-8")) for p in args.arm]
    ref = arms[0]
    for arm in arms[1:]:
        if arm["node_set_sha256"] != ref["node_set_sha256"]:
            raise SystemExit("%s ran on a different node set" % arm["label"])
        if arm["node_count"] != ref["node_count"]:
            raise SystemExit("%s has a different node count" % arm["label"])

    keys = [(r["id"], r["ply"]) for r in ref["results"]]
    for arm in arms[1:]:
        if [(r["id"], r["ply"]) for r in arm["results"]] != keys:
            raise SystemExit("%s nodes are not in the same order" % arm["label"])

    families = sorted({r["family"] for r in ref["results"]})
    out = {"buckets": {}, "families": {}, "pairs": {}}

    def table(select, title):
        print("\n%s" % title)
        header = "%-26s" % "arm"
        for name, _, _ in BUCKETS:
            header += "%18s" % name
        header += "%14s" % "all"
        print(header)
        rows = {}
        for arm in arms:
            sel = [r for r in arm["results"] if select(r)]
            line = "%-26s" % arm["label"]
            cells = {}
            for name, low, high in BUCKETS:
                grp = [r for r in sel if low <= r["win_moves"] <= high]
                kept = sum(r["preserved"] for r in grp)
                cells[name] = (kept, len(grp))
                line += "%18s" % ("%d/%d %.1f%%" % (kept, len(grp),
                                                    100 * kept / len(grp))
                                  if grp else "-")
            kept = sum(r["preserved"] for r in sel)
            cells["all"] = (kept, len(sel))
            line += "%14s" % ("%.1f%%" % (100 * kept / len(sel)) if sel else "-")
            print(line)
            rows[arm["label"]] = {k: list(v) for k, v in cells.items()}
        return rows

    out["buckets"] = table(lambda r: True,
                           "Win-preservation by narrowness (all families)")
    for family in families:
        out["families"][family] = table(
            lambda r, f=family: r["family"] == f,
            "Win-preservation by narrowness (%s)" % family)

    print("\nPaired comparison against %s on NARROW nodes only (win_moves <= %d)"
          % (ref["label"], NARROW_MAX))
    print("%-26s %10s %10s %8s" % ("arm", "arm only", "ref only", "z"))
    narrow = [i for i, r in enumerate(ref["results"]) if r["win_moves"] <= NARROW_MAX]
    base = [ref["results"][i]["preserved"] for i in narrow]
    for arm in arms[1:]:
        cand = [arm["results"][i]["preserved"] for i in narrow]
        only_c, only_r, z = mcnemar(cand, base)
        print("%-26s %10d %10d %8.2f" % (arm["label"], only_c, only_r, z))
        out["pairs"][arm["label"]] = {"arm_only": only_c, "ref_only": only_r,
                                      "z": z, "narrow_nodes": len(narrow)}

    print("\nReading: the WIDE bucket is the control. An arm that is simply")
    print("stronger everywhere improves there too; an arm that resolves the")
    print("failure class improves on NARROW while WIDE stays near ceiling.")
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(out, indent=2) + "\n",
                               encoding="utf-8", newline="\n")
        print("\nWrote %s" % args.output.resolve())
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="cmd", required=True)

    ex = sub.add_parser("extract", help="freeze decision nodes from one engine's play")
    ex.add_argument("--engine", required=True, type=Path)
    ex.add_argument("--syzygy", required=True, action="append", type=Path)
    ex.add_argument("--cohort", required=True, action="append", type=Path)
    ex.add_argument("--family", required=True, action="append")
    ex.add_argument("--nodes", type=int, default=60000)
    ex.add_argument("--max-plies", type=int, default=100)
    ex.add_argument("--hash", type=int, default=16)
    ex.add_argument("--output", required=True, type=Path)
    ex.set_defaults(func=cmd_extract)

    ar = sub.add_parser("arm", help="answer every frozen node with one engine")
    ar.add_argument("--engine", required=True, type=Path)
    ar.add_argument("--syzygy", required=True, action="append", type=Path)
    ar.add_argument("--nodes-file", required=True, type=Path)
    ar.add_argument("--nodes", type=int, default=60000)
    ar.add_argument("--hash", type=int, default=16)
    ar.add_argument("--label", required=True)
    ar.add_argument("--output", required=True, type=Path)
    ar.set_defaults(func=cmd_arm)

    su = sub.add_parser("summary", help="compare arms on the same nodes")
    su.add_argument("--arm", required=True, action="append", type=Path,
                    help="first --arm is the reference for pairing")
    su.add_argument("--output", type=Path)
    su.set_defaults(func=cmd_summary)

    args = parser.parse_args()
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
