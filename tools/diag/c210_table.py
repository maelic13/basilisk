#!/usr/bin/env python3
"""C.2.10's results table from the finished reads (analysis/c210_family_order_2026-11-24.md).

For each finished read under tools/results/c210-<read>/ it recounts the PGN
and refuses the read when the recount disagrees with the run's own record:
the standings' wins, draws and losses for a gauntlet, the recorded
pentanomial for a match, or a game count other than 3,000. It reports:

- the first arm's Elo with a 95% interval, BAS-O08's method: the pentanomial
  mean plus and minus 1.96 standard errors, each end mapped to Elo;
- the pentanomial and the first arm's wins, draws and losses;
- each arm's nodes a second (summed nodes over summed time) and nodes a move,
  from the PGN's move comments;
- the draws with a bare-king win on the board at the end (a lone king against
  a queen, a rook, bishop and knight, or two bishops on opposite colours),
  by the arm that held the win;
- for a meter read, M(mask) = E(0) - E(mask), its two half-widths added in
  quadrature, and two flags: the Basilisk arm more than 5% faster in nodes a
  second than at mask 0, and an arm outside 20% to 80% of the points.

Writes c210-table.json and c210-table.md under the results directory.
Exit 1 when any finished read is refused; reads not yet played are listed
as missing.

Usage:
  python tools/diag/c210_table.py [--results tools/results]
"""

from __future__ import annotations

import argparse
import json
import math
import pathlib
import re
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import colosseum_recount  # noqa: E402

try:
    import chess
    import chess.pgn
except ImportError:  # pragma: no cover
    sys.exit("python-chess is required: pip install chess")

REPO = pathlib.Path(__file__).resolve().parents[2]
ORDER = ["e0", "m7", "m8", "m48", "m64", "frontier", "m127", "m2", "m1", "m4", "m16", "m32"]
MASKS = {"e0": 0, "m7": 7, "m8": 8, "m48": 48, "m64": 64, "m127": 127,
         "m2": 2, "m1": 1, "m4": 4, "m16": 16, "m32": 32}
GAMES = 3000
BASILISK_ARM = "Oracle Basilisk eval"
COMMENT = re.compile(r"\bt=(?P<t>-?\d+)ms\b.*?\bn=(?P<n>\d+)\b")


class Refused(Exception):
    pass


def elo(mu: float) -> float:
    return -400 * math.log10(1 / mu - 1)


def interval(counts: list[int]) -> dict:
    """BAS-O08's estimate: the pentanomial mean and its 95% interval, mapped to Elo."""
    est = colosseum_recount.estimates(counts)
    mu, se = est["mu"], est["sigma"] / math.sqrt(est["pairs"])
    lo, hi = mu - 1.96 * se, mu + 1.96 * se
    return {"score": mu, "elo": elo(mu),
            "low": elo(lo) if lo > 0 else -math.inf, "high": elo(hi) if hi < 1 else math.inf}


def bare_king_win(board: chess.Board) -> chess.Color | None:
    """The side with a won material balance against a lone king, if one stands on the board."""
    for strong in (chess.WHITE, chess.BLACK):
        weak = not strong
        if board.occupied_co[weak] != board.kings & board.occupied_co[weak]:
            continue
        own = lambda piece: len(board.pieces(piece, strong))  # noqa: E731
        bishops = board.pieces(chess.BISHOP, strong)
        both_colours = bool(bishops & chess.BB_LIGHT_SQUARES) and bool(bishops & chess.BB_DARK_SQUARES)
        if own(chess.QUEEN) or own(chess.ROOK) or both_colours or (own(chess.BISHOP) and own(chess.KNIGHT)):
            return strong
    return None


def pgn_stats(pgn: pathlib.Path) -> dict:
    """Per-arm nodes and time from the move comments, and the clamped draws."""
    arms: dict[str, dict] = {}
    clamped: dict[str, int] = {}
    with pgn.open(encoding="utf-8", errors="replace") as stream:
        while (game := chess.pgn.read_game(stream)) is not None:
            names = {chess.WHITE: game.headers.get("White", "?"), chess.BLACK: game.headers.get("Black", "?")}
            board = game.board()
            for node in game.mainline():
                mover = board.turn
                match = COMMENT.search(node.comment or "")
                if match:
                    arm = arms.setdefault(names[mover], {"nodes": 0, "ms": 0, "moves": 0})
                    arm["nodes"] += int(match["n"])
                    arm["ms"] += max(0, int(match["t"]))
                    arm["moves"] += 1
                board.push(node.move)
            if game.headers.get("Result") == "1/2-1/2":
                strong = bare_king_win(board)
                if strong is not None:
                    clamped[names[strong]] = clamped.get(names[strong], 0) + 1
    speed = {name: {"nps": arm["nodes"] * 1000 / arm["ms"] if arm["ms"] else None,
                    "nodes_per_move": arm["nodes"] / arm["moves"] if arm["moves"] else None}
             for name, arm in arms.items()}
    return {"speed": speed, "clamped_draws": clamped}


def check_record(directory: pathlib.Path, counted: dict) -> None:
    """Refuse a read whose recount disagrees with the run's own record."""
    if not counted["agrees"]:
        raise Refused(f"{directory.name}: the recount's pentanomial {counted['pentanomial']} disagrees with "
                      f"the run's {counted['recorded_pentanomial']}.")
    if counted["official_games"] != GAMES or counted["incomplete_pairs"]:
        raise Refused(f"{directory.name}: {counted['official_games']} games and "
                      f"{counted['incomplete_pairs']} incomplete pairs, not {GAMES} games in full pairs.")
    result = json.loads((directory / "result.json").read_text(encoding="utf-8"))
    standings = (result.get("results") or {}).get("standings")
    if standings:
        first = next((s for s in standings if s["name"] == counted["engine_a"]), None)
        if first is None:
            raise Refused(f"{directory.name}: no standing for {counted['engine_a']!r}.")
        recorded = [first["wins"], first["draws"], first["losses"]]
        if recorded != counted["wdl"]:
            raise Refused(f"{directory.name}: the recount's {counted['wdl']} disagrees with the standings' "
                          f"{recorded} for {counted['engine_a']}.")
    elif not counted["comparable"]:
        raise Refused(f"{directory.name}: the run records neither standings nor a pentanomial to check against.")


def read_one(directory: pathlib.Path) -> dict:
    counted = colosseum_recount.recount(directory)
    check_record(directory, counted)
    stats = pgn_stats(directory / "games.pgn")
    return {"read": directory.name.removeprefix("c210-"), "first_arm": counted["engine_a"],
            "pentanomial": counted["pentanomial"], "wdl": counted["wdl"], "faults": counted["faults"],
            **interval(counted["pentanomial"]), **stats}


def half_width(row: dict) -> float:
    return (row["high"] - row["low"]) / 2


def build(results: pathlib.Path) -> tuple[dict, list[str]]:
    rows, refused, missing = {}, [], []
    for read in ORDER:
        directory = results / f"c210-{read}"
        if not (directory / "result.json").exists():
            missing.append(read)
            continue
        try:
            rows[read] = read_one(directory)
        except Refused as refusal:
            refused.append(str(refusal))
    base = rows.get("e0")
    for read, row in rows.items():
        if read not in MASKS:
            continue
        row["mask"] = MASKS[read]
        row["saturated"] = not 0.2 <= row["score"] <= 0.8
        if base is not None and read != "e0":
            row["meter"] = base["elo"] - row["elo"]
            row["meter_error"] = math.hypot(half_width(base), half_width(row))
            stock = base["speed"].get(BASILISK_ARM, {}).get("nps")
            masked = row["speed"].get(BASILISK_ARM, {}).get("nps")
            row["basilisk_faster"] = bool(stock and masked and masked > 1.05 * stock)
    return {"schema": "basilisk-c210-table-v1", "rows": rows, "missing": missing, "refused": refused}, refused


def markdown(table: dict) -> str:
    lines = ["| Read | Mask | First arm | Elo [95%] | M(mask) | Pentanomial | W-D-L | Nodes/s by arm | "
             "Nodes/move by arm | Clamped draws | Flags |",
             "|---|---:|---|---|---|---|---|---|---|---|---|"]
    for read in ORDER:
        row = table["rows"].get(read)
        if row is None:
            continue
        meter = f"{row['meter']:+.1f} ± {row['meter_error']:.1f}" if "meter" in row else "-"
        speed = "; ".join(f"{arm} {s['nps'] / 1e6:.2f}M" for arm, s in row["speed"].items() if s["nps"])
        per_move = "; ".join(f"{arm} {s['nodes_per_move']:,.0f}" for arm, s in row["speed"].items()
                             if s["nodes_per_move"])
        clamped = "; ".join(f"{arm} {n}" for arm, n in row["clamped_draws"].items()) or "0"
        flags = ", ".join(flag for flag, on in (("saturated", row.get("saturated")),
                                                ("Basilisk arm >5% faster", row.get("basilisk_faster"))) if on) or "-"
        lines.append(f"| {read} | {row.get('mask', '-')} | {row['first_arm']} | "
                     f"{row['elo']:+.1f} [{row['low']:+.1f}, {row['high']:+.1f}] | {meter} | {row['pentanomial']} | "
                     f"{'-'.join(map(str, row['wdl']))} | {speed} | {per_move} | {clamped} | {flags} |")
    if table["missing"]:
        lines.append(f"\nNot yet played: {', '.join(table['missing'])}.")
    for refusal in table["refused"]:
        lines.append(f"\nREFUSED: {refusal}")
    return "\n".join(lines) + "\n"


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--results", type=pathlib.Path, default=REPO / "tools" / "results")
    args = parser.parse_args(argv)
    table, refused = build(args.results)
    (args.results / "c210-table.json").write_text(json.dumps(table, indent=2) + "\n", encoding="utf-8")
    text = markdown(table)
    (args.results / "c210-table.md").write_text(text, encoding="utf-8")
    print(text, end="")
    return 1 if refused else 0


if __name__ == "__main__":
    sys.exit(main())
