#!/usr/bin/env python3
"""C.3.1's six zero-game checks of the transplant oracle (analysis/c31_transplant_2026-11-25.md §4).

  1. The old instrument is unchanged: with the transplant and matched cost
     off, `bench 16 1 8 default depth` gives the C.2.9 oracle's node counts
     at masks 0 and 127 under both values of `Use Basilisk HCE`.
  2. Matched cost changes time only: the same four counts with it on.
  3. The wires are live: transplant mask 40 moves the count against mask 0,
     each scale moves it against its default (at mask 40), and mask 127
     moves it against mask 40.
  4. X is the donor's own terms: on game positions the oracle's `transplant`
     line gives, for every family in S (40 and 127), the donor's own trace
     row (`eval` of the stock 9587eeeb build) to the trace's printed
     precision, 0.005 of a pawn. The line's mg(t), eg(t), X and T are
     recomputed here from its fields by section 2's formula, and its sf is
     the donor's applied factor wherever the donor's whole endgame score is
     not zero (Amendment 1).
  5. Colour symmetry: B, X and T of each position equal those of its
     colour-flipped mirror, at S = 40 and S = 127.
  6. A refused combination is refused: a transplant mask with
     `Use Basilisk HCE=false`, and one with a non-zero `EvalAblationMask`,
     each end the engine with a failure at `isready` and at `go`, while the
     valid combination answers `readyok`.

Checks 1 to 3 and 6 are cheap and the reads' launcher repeats them before
every read; 4 and 5 run here once on the pinned binary.

Usage:
  python tools/diag/c31_checks.py --oracle <exe> [--out <json>] [--count N]
  python tools/diag/c31_checks.py --oracle <exe> --wires-only
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import pathlib
import re
import subprocess
import sys

try:
    import chess
except ImportError:  # pragma: no cover
    sys.exit("python-chess is required: pip install chess")

REPO = pathlib.Path(__file__).resolve().parents[2]
ENGINES = REPO / "tools" / "test_engines"
# The C.2.9 oracle whose node counts check 1 reproduces (run_c210_reads.py's pin).
REFERENCE = {"path": ENGINES / "oracle-c29-e56f1e1-evalablate.exe",
             "sha": "CAA8B70BEBA72B84AE118F5ECB29A8803256A2DC25C836DA7334F91E5DF8FF79"}
# The stock 9587eeeb build whose trace rows check 4 reads (C.0's donor).
DONOR = {"path": ENGINES / "donor-sf9587-stock.exe",
         "sha": "2F2FE66BCDE3A205EE586E6F802554B3C5E496EC9A5C647C8206129823E635E3"}
# C.0's gate-game positions, the population the reads are played from.
POSITIONS = {"path": REPO / "tools" / "results" / "c0-screen" / "positions.csv",
             "sha": "1308BD0ED1A737DD818C27AC4975DB223BC9DA6B41E59A683A0445F485455494"}

WIRE_BENCH = "bench 16 1 8 default depth"
HCE, ABLATE, TRANSPLANT = "Use Basilisk HCE", "EvalAblationMask", "EvalTransplantMask"
MG_SCALE, EG_SCALE, MATCHED = "EvalTransplantMgScale", "EvalTransplantEgScale", "EvalMatchedCost"
DEFAULT_MG, DEFAULT_EG = 697, 2022
PAWN_EG = 206  # Stockfish 9587eeeb's PawnValueEg, the trace's unit
PRINTED = 0.005 + 1e-9  # half the trace's last printed digit
FAMILIES = ["space", "mobility", "threats", "king", "pawns", "passed", "pieces"]
PIECES = ["knights", "bishops", "rooks", "queens"]
TRACE_ROWS = {"space": ["Space"], "mobility": ["Mobility"], "threats": ["Threats"], "king": ["King safety"],
              "pawns": ["Pawns"], "passed": ["Passed"], "pieces": ["Knights", "Bishops", "Rooks", "Queens"]}
PIECE_ROW = dict(zip(PIECES, TRACE_ROWS["pieces"]))
SCALAR_FIELDS = ["mask", "mgscale", "egscale", "B", "mg", "eg", "ph", "sf", "donorsf", "egzero", "X", "T", "donor",
                 "lazy", "specialised"]


class Failed(Exception):
    pass


def sha256(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def pinned(item: dict) -> pathlib.Path:
    if not item["path"].exists():
        raise Failed(f"missing {item['path']}")
    actual = sha256(item["path"])
    if actual != item["sha"]:
        raise Failed(f"{item['path']} is {actual}, not {item['sha']}")
    return item["path"]


def run_uci(exe: pathlib.Path, lines: list[str]) -> subprocess.CompletedProcess:
    return subprocess.run([str(exe)], input="\n".join(lines + ["quit"]) + "\n",
                          capture_output=True, text=True)


def setoptions(options: dict) -> list[str]:
    return [f"setoption name {name} value {value}" for name, value in options.items()]


def bench_nodes(exe: pathlib.Path, options: dict) -> int:
    done = run_uci(exe, setoptions(options) + [WIRE_BENCH])
    match = re.search(r"Nodes searched\s*:\s*(\d+)", done.stdout + done.stderr)
    if done.returncode != 0 or not match:
        raise Failed(f"{exe.name}: no bench node count with {options} (exit {done.returncode})")
    return int(match.group(1))


# ---- checks 1 to 3 -----------------------------------------------------------------------------

def check_old_instrument(oracle: pathlib.Path) -> dict:
    """1 and 2: the four C.2.9 counts, with matched cost off and on."""
    reference = pinned(REFERENCE)
    rows = []
    for hce in ("true", "false"):
        for mask in (0, 127):
            base = {HCE: hce, ABLATE: mask}
            want = bench_nodes(reference, base)
            off = bench_nodes(oracle, base)
            on = bench_nodes(oracle, {MATCHED: "true", **base})
            rows.append({"hce": hce, "mask": mask, "reference": want, "matched_off": off, "matched_on": on})
    bad1 = [r for r in rows if r["matched_off"] != r["reference"]]
    bad2 = [r for r in rows if r["matched_on"] != r["reference"]]
    if bad1:
        raise Failed(f"check 1: node counts differ from the C.2.9 oracle: {bad1}")
    if bad2:
        raise Failed(f"check 2: matched cost changes node counts: {bad2}")
    return {"rows": rows}


def check_wires(oracle: pathlib.Path) -> dict:
    """3: each transplant wire moves the search, as the reads set it (matched cost on)."""
    def nodes(**extra) -> int:
        return bench_nodes(oracle, {MATCHED: "true", HCE: "true", ABLATE: 0, **extra})
    counts = {"mask 0": nodes(), "mask 40": nodes(**{TRANSPLANT: 40}),
              "mask 40, mg scale 1024": nodes(**{TRANSPLANT: 40, MG_SCALE: 1024}),
              "mask 40, eg scale 1024": nodes(**{TRANSPLANT: 40, EG_SCALE: 1024}),
              "mask 127": nodes(**{TRANSPLANT: 127})}
    pairs = [("mask 40", "mask 0"), ("mask 40, mg scale 1024", "mask 40"),
             ("mask 40, eg scale 1024", "mask 40"), ("mask 127", "mask 40")]
    dead = [f"{a} = {b} ({counts[a]})" for a, b in pairs if counts[a] == counts[b]]
    if dead:
        raise Failed(f"check 3: dead wires: {dead}")
    return {"counts": counts}


# ---- check 6 -----------------------------------------------------------------------------------

def check_refusals(oracle: pathlib.Path) -> dict:
    invalid = {"transplant with Use Basilisk HCE=false": {TRANSPLANT: 40, HCE: "false"},
               "transplant with EvalAblationMask=8": {TRANSPLANT: 40, ABLATE: 8}}
    results = {}
    for name, options in invalid.items():
        for trigger in (["isready"], ["position startpos", "go depth 1"]):
            done = run_uci(oracle, setoptions(options) + trigger)
            refused = done.returncode != 0 and "refused" in done.stdout and "readyok" not in done.stdout \
                and "bestmove" not in done.stdout
            results[f"{name} at {trigger[-1]}"] = {"exit": done.returncode, "refused": refused}
            if not refused:
                raise Failed(f"check 6: {name} was not refused at {trigger[-1]} (exit {done.returncode})")
    # The valid combination, set in an order that passes through an invalid one.
    done = run_uci(oracle, setoptions({HCE: "false", TRANSPLANT: 40, ABLATE: 8}) +
                   setoptions({ABLATE: 0, HCE: "true"}) + ["isready", "position startpos", "go depth 1"])
    accepted = done.returncode == 0 and "readyok" in done.stdout and "bestmove" in done.stdout
    results["valid combination reached through an invalid one"] = {"exit": done.returncode, "accepted": accepted}
    if not accepted:
        raise Failed(f"check 6: the valid combination was refused (exit {done.returncode})")
    return results


# ---- checks 4 and 5 ----------------------------------------------------------------------------

def sample_positions(count: int) -> tuple[list[str], dict]:
    """Every k-th row of C.0's gate-game positions that is not in check, k fixed by the count."""
    path = pinned(POSITIONS)
    with path.open(encoding="utf-8", newline="") as stream:
        rows = list(csv.DictReader(stream))
    stride = max(1, len(rows) // count)
    fens, in_check = [], 0
    for row in rows[::stride]:
        board = chess.Board(row["fen"])
        if board.is_check():
            in_check += 1
            continue
        fens.append(board.fen())
    return fens, {"rows": len(rows), "stride": stride, "sampled": len(fens), "in_check_skipped": in_check}


def parse_line(line: str) -> dict:
    tokens = line.split()
    if not tokens or tokens[0] != "transplant" or "refused" in line or "none" in tokens:
        raise Failed(f"not a transplant line: {line!r}")
    values, i = {}, 1
    while i < len(tokens):
        key = tokens[i]
        if key in FAMILIES or key in PIECES:
            values[key] = (int(tokens[i + 1]), int(tokens[i + 2]))
            i += 3
        else:
            values[key] = int(tokens[i + 1])
            i += 2
    missing = [k for k in SCALAR_FIELDS + FAMILIES + PIECES if k not in values]
    if missing:
        raise Failed(f"transplant line lacks {missing}: {line!r}")
    return values


def transplant_lines(oracle: pathlib.Path, fens: list[str], mask: int) -> list[dict]:
    lines = setoptions({HCE: "true", ABLATE: 0, TRANSPLANT: mask})
    for fen in fens:
        lines += [f"position fen {fen}", "transplant"]
    done = run_uci(oracle, lines)
    out = [l for l in done.stdout.splitlines() if l.startswith("transplant")]
    if done.returncode != 0 or len(out) != len(fens):
        raise Failed(f"transplant lines: {len(out)} of {len(fens)} (exit {done.returncode})")
    return [parse_line(l) for l in out]


NUMBER = re.compile(r"[+-]?\d+\.\d+")


def donor_traces(fens: list[str]) -> list[dict]:
    """The stock donor's trace: each row's Total column (White minus Black), in pawns."""
    donor = pinned(DONOR)
    lines = []
    for fen in fens:
        lines += [f"position fen {fen}", "eval"]
    done = run_uci(donor, lines)
    if done.returncode != 0:
        raise Failed(f"donor trace exit {done.returncode}")
    traces, current = [], {}
    for line in done.stdout.splitlines():
        if "|" in line:
            name = line.split("|")[0].strip()
            numbers = NUMBER.findall(line.split("|")[-1])
            if len(numbers) == 2:
                current[name] = (float(numbers[0]), float(numbers[1]))
        elif line.startswith("Final evaluation"):
            current["Final"] = float(NUMBER.findall(line)[0])
            traces.append(current)
            current = {}
    if len(traces) != len(fens):
        raise Failed(f"donor traces: {len(traces)} of {len(fens)}")
    return traces


def trunc_div(a: int, b: int) -> int:
    """C++ integer division, truncating toward zero."""
    q = abs(a) // abs(b)
    return q if (a >= 0) == (b > 0) else -q


def expected_x(v: dict, fen: str) -> tuple[int, int, int]:
    """Section 2's X from the line's own fields, independently of the engine's arithmetic."""
    mask = v["mask"]
    mg = sum(v[f][0] for bit, f in enumerate(FAMILIES) if (mask >> bit) & 1)
    eg = sum(v[f][1] for bit, f in enumerate(FAMILIES) if (mask >> bit) & 1)
    num = v["mgscale"] * mg * v["ph"] * 64 + v["egscale"] * eg * (128 - v["ph"]) * v["sf"]
    x = trunc_div(num, 1024 * 64 * 128)
    board = chess.Board(fen)
    x = x if board.turn == chess.WHITE else -x
    x = trunc_div(x * (100 - board.halfmove_clock), 100)
    return mg, eg, x


def check_terms(fens: list[str], lines: dict[int, list[dict]], traces: list[dict]) -> dict:
    """4: every family in S equals its trace row; mg(t), eg(t), X and T follow section 2."""
    report = {}
    for mask, rows in lines.items():
        families = [f for bit, f in enumerate(FAMILIES) if (mask >> bit) & 1]
        cells, gap, where, aggregate_gap, bad = 0, 0.0, None, 0.0, []
        lazy = {0: 0, 1: 0, 2: 0}
        specialised = egzero = 0
        for index, (fen, v, trace) in enumerate(zip(fens, rows, traces)):
            lazy[v["lazy"]] += 1
            specialised += v["specialised"]
            for family in families:
                parts = PIECES if family == "pieces" else [family]
                for part in parts:
                    row = PIECE_ROW[part] if family == "pieces" else TRACE_ROWS[family][0]
                    if row not in trace:
                        bad.append((index, f"no trace row {row}"))
                        continue
                    for phase in (0, 1):
                        cells += 1
                        diff = abs(v[part][phase] / PAWN_EG - trace[row][phase])
                        if diff > gap:
                            gap, where = diff, (index, row, "mg" if phase == 0 else "eg")
                        if diff > PRINTED:
                            bad.append((index, f"{row} {'mg' if phase == 0 else 'eg'} {v[part][phase]} vs {trace[row][phase]}"))
            if sum(v[p][0] for p in PIECES) != v["pieces"][0] or sum(v[p][1] for p in PIECES) != v["pieces"][1]:
                bad.append((index, "pieces is not the sum of the piece types"))
            mg, eg, x = expected_x(v, fen)
            if not v["egzero"] and v["sf"] != v["donorsf"]:
                bad.append((index, f"sf {v['sf']} is not the donor's {v['donorsf']} with a non-zero endgame score"))
            if v["egzero"] and trace.get("Total", (0.0, 0.0))[1] != 0.0:
                bad.append((index, f"a zero endgame score where the trace's total is {trace.get('Total')}"))
            egzero += v["egzero"]
            if (mg, eg, x, v["B"] + x) != (v["mg"], v["eg"], v["X"], v["T"]):
                bad.append((index, f"section 2 gives mg {mg} eg {eg} X {x}; the line {v['mg']} {v['eg']} {v['X']} {v['T']}"))
            rows_in_s = [r for f in families for r in TRACE_ROWS[f] if r in trace]
            for phase, value in ((0, v["mg"]), (1, v["eg"])):
                aggregate_gap = max(aggregate_gap, abs(value / PAWN_EG - sum(trace[r][phase] for r in rows_in_s)))
        report[str(mask)] = {"positions": len(rows), "cells": cells, "largest_gap_pawns": gap,
                             "largest_gap_at": {"position": where[0], "fen": fens[where[0]], "row": where[1],
                                                "phase": where[2]} if where else None,
                             "aggregate_gap_pawns": aggregate_gap, "lazy_exits": lazy, "specialised": specialised,
                             "donor_endgame_score_zero": egzero,
                             "max_abs_X": max(abs(v["X"]) for v in rows), "max_abs_T": max(abs(v["T"]) for v in rows),
                             "failures": len(bad), "first_failures": bad[:10]}
        if bad:
            raise Failed(f"check 4 at S={mask}: {len(bad)} failures, first {bad[:3]}")
    return report


def check_symmetry(oracle: pathlib.Path, fens: list[str], lines: dict[int, list[dict]]) -> dict:
    """5: B, X and T of the colour-flipped mirror equal the position's."""
    mirrors = [chess.Board(fen).mirror().fen() for fen in fens]
    report = {}
    for mask, rows in lines.items():
        flipped = transplant_lines(oracle, mirrors, mask)
        bad = [(i, fens[i], {k: (a[k], b[k]) for k in ("B", "X", "T") if a[k] != b[k]})
               for i, (a, b) in enumerate(zip(rows, flipped)) if (a["B"], a["X"], a["T"]) != (b["B"], b["X"], b["T"])]
        report[str(mask)] = {"positions": len(rows), "asymmetric": len(bad), "first": bad[:10]}
        if bad:
            raise Failed(f"check 5 at S={mask}: {len(bad)} asymmetric positions, first {bad[:3]}")
    return report


def wire_checks(oracle: pathlib.Path) -> dict:
    """The cheap checks the launcher repeats before every read: 1, 2, 3 and 6."""
    return {"1-2": check_old_instrument(oracle), "3": check_wires(oracle), "6": check_refusals(oracle)}


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--oracle", type=pathlib.Path, required=True)
    parser.add_argument("--out", type=pathlib.Path)
    parser.add_argument("--count", type=int, default=12000)
    parser.add_argument("--wires-only", action="store_true")
    args = parser.parse_args(argv)
    report = {"oracle": str(args.oracle), "oracle_sha256": sha256(args.oracle)}
    try:
        report.update(wire_checks(args.oracle))
        if not args.wires_only:
            fens, sample = sample_positions(args.count)
            report["sample"] = {**sample, "positions_sha256": POSITIONS["sha"]}
            lines = {mask: transplant_lines(args.oracle, fens, mask) for mask in (40, 127)}
            report["4"] = check_terms(fens, lines, donor_traces(fens))
            report["5"] = check_symmetry(args.oracle, fens, lines)
        report["verdict"] = "PASS"
        status = 0
    except Failed as failure:
        report["verdict"] = f"FAIL: {failure}"
        status = 1
    text = json.dumps(report, indent=2)
    if args.out:
        args.out.parent.mkdir(parents=True, exist_ok=True)
        args.out.write_text(text + "\n", encoding="utf-8")
    print(text)
    return status


if __name__ == "__main__":
    sys.exit(main())
