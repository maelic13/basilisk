#!/usr/bin/env python3
"""Build the versioned Phase-5.2 differential suite (PLAN 5.2).

The suite must be FIXED and VERSIONED: every 5.2 and 5.4-5.8 measurement is a
comparison against an earlier run, so a suite that silently changes turns a
regression into a mystery. Regenerating it with the same seed and the same
inputs reproduces it byte for byte; changing it means minting suite_v2 and
re-baselining, never editing v1 in place.

Composition follows PLAN 5.2 -- UHO openings, quiet middlegames, tactics,
checks, zugzwangs and endgames -- because tree shape is not uniform across
them. A reduction policy that looks healthy in the opening can be badly wrong
in a zugzwang endgame, and an average over the wrong mixture hides that.

Usage:  python tools/diag/build_suite.py [--out tools/diag/suite_v1.epd]
"""

import argparse
import hashlib
import pathlib
import random
import sys

REPO = pathlib.Path(__file__).resolve().parents[2]
SEED = 0x5115C0DE  # same constant as the oracle conformance test

# Zugzwang and near-zugzwang positions, where null-move and reduction policy
# are most likely to be wrong and least likely to be caught by an average.
# Hand-picked because no corpus we have is labelled for it.
ZUGZWANG = [
    "8/8/p1p5/1p5p/1P5p/8/PPP2K1p/4R1rk w - - 0 1",
    "8/6B1/p5p1/Pp4kp/1P5r/5P1Q/4q1PK/8 w - - 0 32",
    "8/8/1p1r1k2/p1pPN1p1/P3KnP1/1P6/8/3R4 b - - 0 1",
    "6k1/5ppp/8/8/8/8/5PPP/6K1 w - - 0 1",
    "8/8/8/p7/8/1P6/P7/K1k5 w - - 0 1",
    "1r6/8/8/8/8/8/1K6/kR6 w - - 0 1",
]

# Positions with the side to move in check: evasion generation, check
# extensions and the never-reduce-checking-moves gate all live here.
IN_CHECK = [
    "rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3",
    "r1bqkbnr/pppp1ppp/2n5/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 3 3",
    "4k3/8/8/8/8/8/4r3/4K3 w - - 0 1",
    "8/8/8/3k4/8/3K4/8/6q1 w - - 0 1",
]


def read_epd_fens(path, limit, rng):
    """Sample FENs from an EPD file, deterministically."""
    if not path.exists():
        print(f"  ! missing {path.relative_to(REPO)} - skipping", file=sys.stderr)
        return []
    rows = []
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        # Keep only the board/stm/castling/ep fields; drop bm/id opcodes so the
        # suite is positions, not a test with expected answers.
        parts = line.split()
        if len(parts) < 4:
            continue
        fen = " ".join(parts[:4])
        rows.append(fen + " 0 1")
    if len(rows) <= limit:
        return rows
    return rng.sample(rows, limit)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default=str(REPO / "tools" / "diag" / "suite_v1.epd"))
    args = ap.parse_args()

    rng = random.Random(SEED)
    sections = []

    sections.append(("openings", read_epd_fens(
        REPO / "tools" / "books" / "UHO_Lichess_4852_v1.epd", 40, rng)))
    sections.append(("tactics", read_epd_fens(
        REPO / "src" / "wac.epd", 40, rng)))
    sections.append(("endgames", read_epd_fens(
        REPO / "tests" / "endgames.epd", 20, rng)))
    sections.append(("zugzwang", ZUGZWANG))
    sections.append(("in_check", IN_CHECK))

    lines = [
        "# Basilisk Phase-5.2 differential suite, version 1.",
        "#",
        "# FIXED AND VERSIONED. Every 5.2-5.8 measurement compares against an",
        "# earlier run over this exact file. Do not edit it: mint suite_v2 and",
        "# re-baseline instead, or old counters become incomparable in a way",
        "# nothing will flag.",
        "#",
        "# Regenerate with: python tools/diag/build_suite.py",
        f"# Deterministic seed: 0x{SEED:X}",
        "#",
    ]
    total = 0
    for name, fens in sections:
        lines.append(f"# --- {name} ({len(fens)}) ---")
        lines.extend(fens)
        total += len(fens)

    text = "\n".join(lines) + "\n"
    out = pathlib.Path(args.out)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(text, encoding="utf-8")

    digest = hashlib.sha256(text.encode("utf-8")).hexdigest()
    print(f"wrote {out.relative_to(REPO)}: {total} positions")
    for name, fens in sections:
        print(f"  {name:10} {len(fens):4}")
    print(f"sha256 {digest}")


if __name__ == "__main__":
    main()
