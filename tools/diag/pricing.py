#!/usr/bin/env python3
"""What pruning and reductions hide: the reader of the `Pricing` probes.

A tune or diagnostic build with `setoption name Pricing value N` searches
every N-th decision of each pruning rule again as the rule denied it and
prints one `info string price kv` line per cell (rule, remaining depth, node
type, move-number bucket) at the end of the search (src/search_pricing.h).
Quiescence's move rules have no remaining depth: their cells carry the move's
class in the depth slot, the piece taken and whether it recaptures, and the
report prints such a rule by class.
This tool drives one fresh engine process per position at `go depth D`,
sums the cells over the suite with the committed parser and reports, per
rule and per cell, the share of probes that beat the rule's bound and the
share that would have changed the node's best move.

Modes:
  read       run the suite with Pricing N; writes <out>/runs.jsonl and
             <out>/summary.json and prints the table
  isolation  run the suite with Pricing 0 and Pricing N (Diag on) and
             require identical searches: every `info depth` line (nodes,
             score, PV; not time or nps), the bestmove and every search
             counter except the work counters the probes share (below).
             Exit 1 on any difference.
  report     print the table of a finished read
  compare    two reads side by side for one rule (a planted rule against
             its honest form)

Usage:
  python tools/diag/pricing.py read ENGINE --every 8 --depth 12 --out DIR
  python tools/diag/pricing.py isolation ENGINE --every 4 --depth 10
  python tools/diag/pricing.py report DIR [--cells RULE]
  python tools/diag/pricing.py compare DIR_A DIR_B --rule quiet_futility
Options set with --option "Name=Value" apply to every process (repeatable).
"""
from __future__ import annotations

import argparse
import json
import math
import pathlib
import re
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from run_suite import REPO, bestmove, load_suite, parse_diag, uci_run  # noqa: E402

PRICE_LINE = re.compile(r"^info string price kv (.*)$")
PRICE_TOTAL = re.compile(r"^info string price total (.*)$")
PRICE_ERROR = re.compile(r"^info string price error")
TOKEN = re.compile(r"([a-z_]+)=(\S+)")
COUNTS = ("decisions", "probed", "beats", "changes", "probe_nodes")
KEYS = ("rule", "depth", "node", "moves")

# Counters the probes share with the engine's own search: the board's and the
# evaluator's work and the cross-colour history reads (src/search_pricing.h).
SHARED_WORK = {"see_ge_calls", "gives_check_calls", "eval_calls", "pawn_probes",
               "pawn_hits"}
SHARED_PREFIX = ("xc_",)

RULE_ORDER = ["razor", "rfp", "null_move", "probcut", "probcut_tt", "multicut",
              "qs_delta", "capture_futility", "capture_see", "cont_hist",
              "quiet_futility", "quiet_see", "skip_quiets", "qs_futility", "qs_see",
              "qs_late", "qs_count", "qs_evasion", "lmr_fail_low"]
NODE_RULES = {"razor", "rfp", "null_move", "probcut", "probcut_tt", "multicut", "qs_delta"}

# The rules whose cells carry a move's class in the depth slot: the piece it
# takes (price_class in src/search_pricing.h) plus 6 for a recapture.
CLASS_RULES = {"qs_futility", "qs_see", "qs_count", "qs_evasion"}
VICTIMS = ("none", "pawn", "knight", "bishop", "rook", "queen")
PRICE_RECAPTURE = 6


def parse_price(text: str) -> tuple[list[dict], dict | None]:
    """The cells and the total line of one search."""
    cells, total = [], None
    for raw in text.splitlines():
        line = raw.strip()
        if PRICE_ERROR.match(line):
            raise RuntimeError(f"engine refused pricing: {line}")
        m = PRICE_LINE.match(line)
        if m:
            fields = dict(TOKEN.findall(m.group(1)))
            cell = {k: fields[k] for k in KEYS}
            cell["depth"] = int(cell["depth"])
            for k in COUNTS:
                cell[k] = int(fields[k])
            cells.append(cell)
            continue
        m = PRICE_TOTAL.match(line)
        if m:
            total = {k: int(v) for k, v in TOKEN.findall(m.group(1))}
    return cells, total


def check_cells(cells: list[dict], total: dict | None, every: int) -> None:
    """The invariants every search's cells must satisfy."""
    if total is None:
        raise RuntimeError("no `price total` line: is this a tune or diagnostic build?")
    if total.get("every") != every:
        raise RuntimeError(f"engine priced every {total.get('every')}, asked {every}")
    for c in cells:
        if not (c["changes"] <= c["beats"] <= c["probed"] <= c["decisions"]):
            raise RuntimeError(f"cell counts out of order: {c}")
        if c["rule"] in NODE_RULES and c["changes"]:
            raise RuntimeError(f"a node rule recorded a changed best move: {c}")
    for k in ("decisions", "probed", "probe_nodes"):
        if sum(c[k] for c in cells) != total[k]:
            raise RuntimeError(f"cells do not sum to the total line on {k}")


SEARCH_LINE = re.compile(r"^info depth \d+")


def search_lines(text: str) -> list[str]:
    """Every iteration line without its clock fields."""
    out = []
    for raw in text.splitlines():
        line = raw.strip()
        if not SEARCH_LINE.match(line):
            continue
        tokens = line.split()
        kept, i = [], 0
        while i < len(tokens):
            if tokens[i] in ("time", "nps") and i + 1 < len(tokens):
                i += 2
                continue
            kept.append(tokens[i])
            i += 1
        out.append(" ".join(kept))
    return out


def engine_commands(fen: str, depth: int, options: list[str], every: int,
                    diag: bool) -> list[str]:
    cmds = [f"setoption name {o.split('=', 1)[0]} value {o.split('=', 1)[1]}" for o in options]
    if diag:
        cmds.append("setoption name Diag value true")
    cmds += [f"setoption name Pricing value {every}", "ucinewgame", f"position fen {fen}",
             f"go depth {depth}"]
    return cmds


def wilson(k: int, n: int) -> tuple[float, float]:
    """The 95% Wilson interval of a share."""
    if n == 0:
        return (0.0, 1.0)
    z, p = 1.959964, k / n
    centre = (p + z * z / (2 * n)) / (1 + z * z / n)
    half = z * math.sqrt(p * (1 - p) / n + z * z / (4 * n * n)) / (1 + z * z / n)
    return (max(0.0, centre - half), min(1.0, centre + half))


def add_cells(total: dict, cells: list[dict]) -> None:
    for c in cells:
        key = (c["rule"], c["depth"], c["node"], c["moves"])
        slot = total.setdefault(key, dict.fromkeys(COUNTS, 0))
        for k in COUNTS:
            slot[k] += c[k]


def move_class(code: int) -> tuple[str, bool]:
    """The victim's name and whether the move recaptures, from a class cell."""
    victim, recapture = code % PRICE_RECAPTURE, code // PRICE_RECAPTURE
    if not 0 <= code < 2 * PRICE_RECAPTURE:
        raise ValueError(f"not a move class: {code}")
    return VICTIMS[victim], bool(recapture)


def depth_band(depth: int) -> str:
    return "0" if depth == 0 else "1-3" if depth <= 3 else "4-7" if depth <= 7 else "8+"


def summarise(cells: dict) -> dict:
    """Per rule, and per rule by depth band, node type and move bucket."""
    groups: dict[str, dict] = {}

    def bump(name: str, slot: dict) -> None:
        g = groups.setdefault(name, dict.fromkeys(COUNTS, 0))
        for k in COUNTS:
            g[k] += slot[k]

    for (rule, depth, node, moves), slot in cells.items():
        bump(rule, slot)
        if rule in CLASS_RULES:
            victim, recapture = move_class(depth)
            kind = "recapture" if recapture else "other"
            bump(f"{rule} class {kind}", slot)
            bump(f"{rule} victim {victim}", slot)
            bump(f"{rule} victim {victim} {kind}", slot)
            if moves != "-":
                bump(f"{rule} moves {moves} {kind}", slot)
        else:
            bump(f"{rule} depth {depth_band(depth)}", slot)
        bump(f"{rule} node {node}", slot)
        if moves != "-":
            bump(f"{rule} moves {moves}", slot)
    out = {}
    for name, g in groups.items():
        lo, hi = wilson(g["beats"], g["probed"])
        out[name] = {**g,
                     "beat_share": g["beats"] / g["probed"] if g["probed"] else None,
                     "beat_ci": [lo, hi],
                     "change_share": g["changes"] / g["probed"] if g["probed"] else None,
                     "nodes_per_probe": g["probe_nodes"] / g["probed"] if g["probed"] else None}
    return out


def fmt_share(v: float | None) -> str:
    return "-" if v is None else f"{100 * v:6.2f}%"


def hidden_per_mnode(name: str, s: dict, search_nodes: int) -> float | None:
    """Estimated hidden changes per million search nodes: decisions times the
    change share (the beat share for a node rule, which has no best move)."""
    share = s["beat_share"] if name.split()[0] in NODE_RULES else s["change_share"]
    if share is None or search_nodes <= 0:
        return None
    return s["decisions"] * share / search_nodes * 1e6


def screen(summary: dict, search_nodes: int) -> dict:
    """BAS-S47's screening rule: the three rules with the most estimated
    hidden changes per million search nodes, and every rule whose beat
    share's lower 95% bound exceeds 5%."""
    rules = [r for r in RULE_ORDER if r in summary and summary[r]["probed"]]
    ranked = sorted(rules, key=lambda r: hidden_per_mnode(r, summary[r], search_nodes),
                    reverse=True)
    return {"top_hidden": ranked[:3],
            "beat_floor_above_5pct": [r for r in rules if summary[r]["beat_ci"][0] > 0.05]}


def print_table(summary: dict, detail: str | None, search_nodes: int = 0) -> None:
    print(f"{'rule':40} {'decisions':>11} {'probed':>8} {'beats':>7} {'beat%':>8} "
          f"{'95% CI':>17} {'change%':>8} {'nodes/probe':>12} {'hidden/Mn':>10}")
    names = [r for r in RULE_ORDER if r in summary]
    if detail:
        names = sorted((n for n in summary if n.split()[0] == detail),
                       key=lambda n: (n != detail, n))
    for name in names:
        s = summary[name]
        lo, hi = s["beat_ci"]
        change = "-" if name.split()[0] in NODE_RULES else fmt_share(s["change_share"])
        npp = "-" if s["nodes_per_probe"] is None else f"{s['nodes_per_probe']:.0f}"
        hidden = hidden_per_mnode(name, s, search_nodes)
        hid = "-" if hidden is None else f"{hidden:.1f}"
        print(f"{name:40} {s['decisions']:>11,} {s['probed']:>8,} {s['beats']:>7,} "
              f"{fmt_share(s['beat_share']):>8} {100 * lo:7.2f}-{100 * hi:6.2f}% "
              f"{change:>8} {npp:>12} {hid:>10}")


def class_shares(summary: dict, rule: str) -> dict | None:
    """Recaptures' share of a class rule's decisions and of its beats."""
    whole, recaptures = summary.get(rule), summary.get(f"{rule} class recapture")
    if rule not in CLASS_RULES or not whole or not whole["decisions"]:
        return None
    if not recaptures:
        return {"decisions": 0.0, "beats": 0.0}
    return {"decisions": recaptures["decisions"] / whole["decisions"],
            "beats": recaptures["beats"] / whole["beats"] if whole["beats"] else 0.0}


def run_read(args) -> int:
    fens = load_suite(pathlib.Path(args.suite))[: args.limit or None]
    out = pathlib.Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    runs = out / "runs.jsonl"
    if runs.exists():
        sys.exit(f"{runs} already holds a read; move it aside first")
    options = [f"Hash={args.hash}", *args.option]
    cells: dict = {}
    search_nodes = probe_nodes = 0
    with runs.open("w", encoding="utf-8") as fh:
        for i, fen in enumerate(fens, 1):
            text = uci_run(args.engine, engine_commands(fen, args.depth, options, args.every,
                                                        False), timeout=args.timeout)
            found, total = parse_price(text)
            check_cells(found, total, args.every)
            nodes = last_nodes(text)
            search_nodes += nodes
            probe_nodes += total["probe_nodes"]
            add_cells(cells, found)
            fh.write(json.dumps({"position": i, "fen": fen, "bestmove": bestmove(text),
                                 "nodes": nodes, "total": total, "cells": found}) + "\n")
            if i % 10 == 0:
                print(f"  {i}/{len(fens)}", flush=True)
    summary = summarise(cells)
    report = {"engine": str(args.engine), "suite": pathlib.Path(args.suite).name,
              "positions": len(fens), "depth": args.depth, "every": args.every,
              "options": options, "search_nodes": search_nodes, "probe_nodes": probe_nodes,
              "rules": summary}
    (out / "summary.json").write_text(json.dumps(report, indent=1), encoding="utf-8")
    print(f"{len(fens)} positions at depth {args.depth}, every {args.every}: "
          f"{search_nodes:,} search nodes, {probe_nodes:,} probe nodes")
    print_table(summary, None, search_nodes)
    return 0


def last_nodes(text: str) -> int:
    nodes = 0
    for line in search_lines(text):
        m = re.search(r"\bnodes (\d+)", line)
        if m and " lowerbound " not in f" {line} " and " upperbound " not in f" {line} ":
            nodes = int(m.group(1))
    return nodes


def run_isolation(args) -> int:
    fens = load_suite(pathlib.Path(args.suite))[: args.limit or None]
    options = [f"Hash={args.hash}", *args.option]
    differences = 0
    probed = 0
    for i, fen in enumerate(fens, 1):
        off = uci_run(args.engine, engine_commands(fen, args.depth, options, 0, True),
                      timeout=args.timeout)
        on = uci_run(args.engine, engine_commands(fen, args.depth, options, args.every, True),
                     timeout=args.timeout)
        cells, total = parse_price(on)
        check_cells(cells, total, args.every)
        probed += total["probed"]
        problems = []
        if search_lines(off) != search_lines(on):
            problems.append("iteration lines differ")
        if bestmove(off) != bestmove(on):
            problems.append(f"bestmove {bestmove(off)} vs {bestmove(on)}")
        a, b = parse_diag(off), parse_diag(on)
        if not a:
            problems.append("no diag counters")
        for k in sorted(set(a) | set(b)):
            if k in SHARED_WORK or k.startswith(SHARED_PREFIX):
                continue
            if a.get(k) != b.get(k):
                problems.append(f"{k} {a.get(k)} vs {b.get(k)}")
        if problems:
            differences += 1
            print(f"position {i}: " + "; ".join(problems[:6]) + f"  [{fen}]")
    print(f"isolation: {len(fens)} positions at depth {args.depth}, every {args.every}, "
          f"{probed:,} probes; {differences} positions differ")
    if probed == 0:
        print("no probe ran: the check proves nothing")
        return 1
    return 1 if differences else 0


def load_summary(path: str) -> dict:
    return json.loads((pathlib.Path(path) / "summary.json").read_text(encoding="utf-8"))


def run_report(args) -> int:
    report = load_summary(args.dir)
    print(f"{report['positions']} positions at depth {report['depth']}, every "
          f"{report['every']}: {report['search_nodes']:,} search nodes, "
          f"{report['probe_nodes']:,} probe nodes")
    print_table(report["rules"], args.cells, report["search_nodes"])
    shares = class_shares(report["rules"], args.cells) if args.cells else None
    if shares:
        print(f"recaptures: {fmt_share(shares['decisions'])} of decisions, "
              f"{fmt_share(shares['beats'])} of beats")
    if not args.cells:
        s = screen(report["rules"], report["search_nodes"])
        print(f"most hidden changes per million search nodes: {', '.join(s['top_hidden'])}")
        print("beat share's lower 95% bound above 5%: "
              + (", ".join(s["beat_floor_above_5pct"]) or "none"))
    return 0


def run_compare(args) -> int:
    a, b = load_summary(args.a), load_summary(args.b)
    for label, report in (("A", a), ("B", b)):
        s = report["rules"].get(args.rule)
        if not s:
            sys.exit(f"{label} has no {args.rule} decisions")
        lo, hi = s["beat_ci"]
        print(f"{label}: {args.rule} decisions {s['decisions']:,}, probed {s['probed']:,}, "
              f"beats {s['beats']:,} = {fmt_share(s['beat_share'])} "
              f"[{100 * lo:.2f}, {100 * hi:.2f}]")
    sa, sb = a["rules"][args.rule], b["rules"][args.rule]
    disjoint = sb["beat_ci"][0] > sa["beat_ci"][1]
    print("B's beat share is above A's with disjoint 95% intervals" if disjoint
          else "the intervals overlap")
    return 0 if disjoint else 1


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    sub = ap.add_subparsers(dest="mode", required=True)
    for name in ("read", "isolation"):
        p = sub.add_parser(name)
        p.add_argument("engine")
        p.add_argument("--every", type=int, required=True)
        p.add_argument("--depth", type=int, required=True)
        p.add_argument("--suite", default=str(REPO / "tools" / "diag" / "suite_v2.epd"))
        p.add_argument("--limit", type=int, default=0, help="first N positions only")
        p.add_argument("--hash", type=int, default=64)
        p.add_argument("--timeout", type=float, default=3600)
        p.add_argument("--option", action="append", default=[], metavar="Name=Value")
        if name == "read":
            p.add_argument("--out", required=True)
    p = sub.add_parser("report")
    p.add_argument("dir")
    p.add_argument("--cells", default=None, metavar="RULE", help="one rule's breakdown")
    p = sub.add_parser("compare")
    p.add_argument("a")
    p.add_argument("b")
    p.add_argument("--rule", required=True)
    args = ap.parse_args()
    if args.mode in ("read", "isolation"):
        if args.every < 1:
            ap.error("--every must be at least 1")
        args.engine = str(pathlib.Path(args.engine).resolve())
    return {"read": run_read, "isolation": run_isolation, "report": run_report,
            "compare": run_compare}[args.mode](args)


if __name__ == "__main__":
    sys.exit(main())
