#!/usr/bin/env python3
"""Syzygy-truth endgame corpus and conversion baseline (PLAN 6.0).

The earlier fixed-seed conversion checks answer one bit per game: did the
engine mate inside the budget. At 100 positions per family that is a
binomial standard error of about 3.5 points. This tool fixes the
resolution problem by grading every strong-side move against the tablebase
instead of every game against the clock, which turns ~100 binary outcomes per
family into thousands of graded decisions.

It separates the four things the endgame audit requires be kept apart:

  theoretical verdict  Syzygy WDL for the position, before anything is played.
  decision quality     per move: did the engine preserve the theoretical win,
                       and did it make DTZ progress toward the zeroing move.
  conversion           did the game actually finish inside the node/ply budget.
  game strength        left to a real match; this tool never reports Elo.

A played draw is statistical evidence, not theoretical truth: only the
`theory_*` fields are truth here, and they come from the tablebase.

Cursed wins matter and are kept distinct. Syzygy WDL 2 is a clean win, 1 is a
win that the fifty-move rule turns into a draw. Downgrading a 2 to a 1 is a
KBN-K failure mode: the engine can still "see" a win that the fifty-move rule
has already turned into a draw. Such a move is scored as discarding the clean
win, not as preserving it.

With `--per-position`, every position carries its generation seed and is
written to the report, so two runs over the same seed are paired
position-by-position and a later comparison can use a paired test rather than
comparing two aggregates.

Example:

  python tools/diag/endgame_truth.py \
      --engine tools/test_engines/basilisk-hce-refit-candidate-pext-pgo.exe \
      --syzygy D:/chess/tablebases/syzygy3456 \
      --positions 100 --nodes 60000 --max-plies 100 \
      --output tools/results/hce-accepted/endgame-truth-accepted.json
"""

from __future__ import annotations

import argparse
import hashlib
import json
import random
import statistics
import sys
from collections import Counter
from pathlib import Path

import chess
import chess.engine
import chess.syzygy

PIECE_OF = {
    "Q": chess.QUEEN,
    "R": chess.ROOK,
    "B": chess.BISHOP,
    "N": chess.KNIGHT,
    "P": chess.PAWN,
}

# Families are written strong-side first, as "KQR-KP". The tables here cover
# 6 men, so a spec may not exceed that. The bare-king set reproduces
# endgame_conversion.py's four families; the rest are the reference functions
# from the inventory that Syzygy can adjudicate.
DEFAULT_FAMILIES = [
    "KQ-K", "KR-K", "KBB-K", "KBN-K", "KNN-K",
    "KP-K", "KPP-K", "KBP-K",
    "KR-KP", "KR-KB", "KR-KN", "KQ-KP", "KQ-KR", "KNN-KP",
    "KRP-KR", "KRP-KB", "KBP-KB", "KBP-KN", "KP-KP",
]


def parse_family(spec: str) -> tuple[tuple[int, ...], tuple[int, ...]]:
    """"KRP-KR" -> ((ROOK, PAWN), (ROOK,)). Kings are implicit."""
    try:
        strong, weak = spec.split("-")
    except ValueError:
        raise ValueError(f"family must look like KRP-KR, got {spec!r}") from None
    out = []
    for side in (strong, weak):
        if not side.startswith("K"):
            raise ValueError(f"each side of {spec!r} must start with K")
        pieces = []
        for ch in side[1:]:
            if ch not in PIECE_OF:
                raise ValueError(f"unknown piece {ch!r} in {spec!r}")
            pieces.append(PIECE_OF[ch])
        out.append(tuple(pieces))
    if 2 + len(out[0]) + len(out[1]) > 6:
        raise ValueError(f"{spec!r} exceeds 6 men; no table for it here")
    return out[0], out[1]


def random_position(
    rng: random.Random,
    strong: tuple[int, ...],
    weak: tuple[int, ...],
) -> chess.Board:
    """A legal, non-terminal position with White as the strong side to move."""
    n = 2 + len(strong) + len(weak)
    for _ in range(20_000):
        squares = rng.sample(range(64), n)
        board = chess.Board(None)
        board.turn = chess.WHITE
        board.set_piece_at(squares[0], chess.Piece(chess.KING, chess.WHITE))
        board.set_piece_at(squares[1], chess.Piece(chess.KING, chess.BLACK))
        i = 2
        bad = False
        for piece, color in (
            [(p, chess.WHITE) for p in strong] + [(p, chess.BLACK) for p in weak]
        ):
            sq = squares[i]
            i += 1
            # Pawns cannot stand on the back ranks.
            if piece == chess.PAWN and chess.square_rank(sq) in (0, 7):
                bad = True
                break
            board.set_piece_at(sq, chess.Piece(piece, color))
        if bad:
            continue
        # Two same-colour bishops on one side make KBB-K a non-mate.
        for color in (chess.WHITE, chess.BLACK):
            bishops = list(board.pieces(chess.BISHOP, color))
            if len(bishops) == 2:
                a, b = bishops
                if (chess.square_rank(a) + chess.square_file(a)) % 2 == (
                    chess.square_rank(b) + chess.square_file(b)
                ) % 2:
                    bad = True
        if bad:
            continue
        if not board.is_valid() or board.is_check():
            continue
        if not any(board.legal_moves):
            continue
        return board
    raise RuntimeError(f"could not generate a legal position for {strong}/{weak}")


def wdl_for_white(tb: chess.syzygy.Tablebase, board: chess.Board) -> int:
    """WDL from WHITE's point of view, whoever is to move."""
    wdl = tb.probe_wdl(board)
    return wdl if board.turn == chess.WHITE else -wdl


def dtz_abs(tb: chess.syzygy.Tablebase, board: chess.Board) -> int | None:
    try:
        return abs(tb.probe_dtz(board))
    except (chess.syzygy.MissingTableError, KeyError, ValueError):
        return None


def play_and_grade(
    engine: chess.engine.SimpleEngine,
    tb: chess.syzygy.Tablebase,
    board: chess.Board,
    nodes: int,
    max_plies: int,
    game_token: object,
) -> dict:
    """Play the position out; grade every White move against the tablebase."""
    # Count the STRONG side's material only. Whole-board material is right for
    # bare-king families and wrong for every other one: in KR-KP, capturing the
    # enemy pawn is the winning plan, not a material-loss failure.
    def strong_material(b: chess.Board) -> int:
        return chess.popcount(b.occupied_co[chess.WHITE])

    initial_material = strong_material(board)
    graded = 0
    preserved = 0
    dtz_checked = 0
    dtz_progress = 0
    first_discard_ply = None

    for ply in range(max_plies):
        if board.is_checkmate():
            outcome = "mated" if board.turn == chess.BLACK else "wrong_mate"
            break
        if board.is_stalemate():
            outcome = "stalemate"
            break
        if board.is_insufficient_material():
            outcome = "insufficient_material"
            break
        if board.is_fifty_moves() or board.can_claim_fifty_moves():
            outcome = "fifty_move"
            break
        if strong_material(board) < initial_material:
            outcome = "material_lost"
            break

        white_to_move = board.turn == chess.WHITE
        before_wdl = before_dtz = None
        if white_to_move:
            try:
                before_wdl = wdl_for_white(tb, board)
                before_dtz = dtz_abs(tb, board)
            except (chess.syzygy.MissingTableError, KeyError, ValueError):
                before_wdl = None

        result = engine.play(board, chess.engine.Limit(nodes=nodes), game=game_token)
        if result.move is None:
            outcome = "no_move"
            break
        board.push(result.move)

        # Grade only moves made from a position the tablebase calls a CLEAN
        # win. A cursed win (WDL 1) is already unconvertible under the
        # fifty-move rule, so demanding progress from it would score the
        # engine on a position it cannot win.
        if white_to_move and before_wdl == 2:
            graded += 1
            try:
                after_wdl = wdl_for_white(tb, board)
            except (chess.syzygy.MissingTableError, KeyError, ValueError):
                after_wdl = None
            if after_wdl == 2:
                preserved += 1
                after_dtz = dtz_abs(tb, board)
                if before_dtz is not None and after_dtz is not None:
                    dtz_checked += 1
                    if after_dtz < before_dtz:
                        dtz_progress += 1
            elif first_discard_ply is None:
                first_discard_ply = ply
    else:
        outcome = "ply_limit"

    return {
        "outcome": outcome,
        "plies": ply,
        "graded_moves": graded,
        "win_preserving_moves": preserved,
        "dtz_checked_moves": dtz_checked,
        "dtz_progress_moves": dtz_progress,
        "first_discard_ply": first_discard_ply,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--engine", required=True, type=Path)
    parser.add_argument("--syzygy", required=True, type=Path)
    parser.add_argument("--positions", type=int, default=100)
    parser.add_argument("--nodes", type=int, default=60_000)
    parser.add_argument("--max-plies", type=int, default=100)
    parser.add_argument("--seed", type=int, default=0x5E9D18)
    parser.add_argument("--families", default=",".join(DEFAULT_FAMILIES))
    parser.add_argument("--hash", type=int, default=16)
    parser.add_argument(
        "--per-position",
        action="store_true",
        help="write every position's record, so two runs can be paired",
    )
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    if min(args.positions, args.nodes, args.max_plies) <= 0:
        parser.error("positions, nodes and max-plies must be positive")
    engine_path = args.engine.resolve()
    if not engine_path.is_file():
        parser.error(f"engine not found: {engine_path}")
    if not args.syzygy.is_dir():
        parser.error(f"syzygy path is not a directory: {args.syzygy}")

    families = [f for f in args.families.split(",") if f]
    specs = {}
    for name in families:
        try:
            specs[name] = parse_family(name)
        except ValueError as exc:
            parser.error(str(exc))

    report = {
        "schema": "basilisk-endgame-truth-v1",
        "engine": str(engine_path),
        "engine_sha256": hashlib.sha256(engine_path.read_bytes()).hexdigest(),
        "syzygy": str(args.syzygy.resolve()),
        "positions_per_family": args.positions,
        "nodes_per_move": args.nodes,
        "max_plies": args.max_plies,
        "seed": args.seed,
        "hash_mb": args.hash,
        "persistent_tt_per_game": True,
        "families": {},
    }

    tb = chess.syzygy.open_tablebase(str(args.syzygy))
    engine = chess.engine.SimpleEngine.popen_uci(str(engine_path))
    try:
        options = {}
        if "Hash" in engine.options:
            options["Hash"] = args.hash
        if "Threads" in engine.options:
            options["Threads"] = 1
        # The engine must not consult the tablebases itself: this measures the
        # evaluation's own endgame knowledge, and a TB-backed root would
        # measure the tables instead.
        if "SyzygyPath" in engine.options:
            options["SyzygyPath"] = ""
        if options:
            engine.configure(options)

        for name in families:
            strong, weak = specs[name]
            # Seed from the family NAME, never its index in the list. A subset
            # run must produce the same positions as that family in a full run.
            family_seed = args.seed ^ int.from_bytes(
                hashlib.sha256(name.encode()).digest()[:8], "big"
            )
            rng = random.Random(family_seed)
            theory = Counter()
            outcomes = Counter()
            mate_plies = []
            optimal_dtz = []
            graded = preserved = dtz_checked = dtz_progress = 0
            won_positions = 0
            converted = 0
            records = []
            efficiency_pairs = []

            for index in range(args.positions):
                board = random_position(rng, strong, weak)
                fen = board.fen()
                try:
                    verdict = wdl_for_white(tb, board)
                    start_dtz = dtz_abs(tb, board)
                except (chess.syzygy.MissingTableError, KeyError, ValueError) as exc:
                    parser.error(f"no tablebase for {name}: {exc}")
                theory[{2: "win", 1: "cursed_win", 0: "draw",
                        -1: "blessed_loss", -2: "loss"}[verdict]] += 1

                played = play_and_grade(
                    engine, tb, board, args.nodes, args.max_plies, object()
                )
                outcomes[played["outcome"]] += 1
                graded += played["graded_moves"]
                preserved += played["win_preserving_moves"]
                dtz_checked += played["dtz_checked_moves"]
                dtz_progress += played["dtz_progress_moves"]

                # Conversion is only meaningful on a CLEAN theoretical win.
                if verdict == 2:
                    won_positions += 1
                    if played["outcome"] == "mated":
                        converted += 1
                        mate_plies.append(played["plies"])
                        if start_dtz:
                            efficiency_pairs.append((played["plies"], start_dtz))
                    if start_dtz is not None:
                        optimal_dtz.append(start_dtz)

                if args.per_position:
                    records.append({
                        "index": index,
                        "generation_seed": family_seed,
                        "fen": fen,
                        "theory_wdl": verdict,
                        "theory_dtz": start_dtz,
                        **played,
                    })

                if (index + 1) % 25 == 0:
                    print(
                        f"{name}: {index + 1}/{args.positions} "
                        f"converted={converted}/{won_positions} "
                        f"preserved={preserved}/{graded}",
                        flush=True,
                    )

            entry = {
                "spec": name,
                "generation_seed": family_seed,
                "theory": dict(theory),
                "outcomes": dict(outcomes),
                "theoretically_won": won_positions,
                "converted": converted,
                "conversion_rate": (converted / won_positions) if won_positions else None,
                "graded_moves": graded,
                "win_preserving_moves": preserved,
                "win_preserving_rate": (preserved / graded) if graded else None,
                "dtz_checked_moves": dtz_checked,
                "dtz_progress_moves": dtz_progress,
                "dtz_progress_rate": (dtz_progress / dtz_checked) if dtz_checked else None,
                "dtz_progress_is_technique": (not weak) and chess.PAWN not in strong,
                "median_mate_plies": statistics.median(mate_plies) if mate_plies else None,
                "median_optimal_dtz": statistics.median(optimal_dtz) if optimal_dtz else None,
            }
            # Plies taken against the tablebase's own distance, PAIRED per
            # position and reported only where the two are the same quantity.
            #
            # Two traps, both hit before this was written. DTZ is distance to
            # the next ZEROING move, not to mate; in KR-KP the zeroing move is
            # the pawn capture and mate comes much later, which made a naive
            # ratio read 15.5 and mean nothing. And taking median mate plies
            # over converted games while taking median DTZ over all won
            # positions is survivorship bias -- in KBN-K only the 3 easiest
            # positions converted, which made the ratio read 0.811, i.e.
            # "better than optimal".
            #
            # So: pair each converted position with its OWN dtz, and report
            # the ratio only when the weak side is bare and the strong side is
            # pawnless, where no zeroing move exists before mate and DTZ is
            # therefore DTM.
            # Same gate as the efficiency ratio, and for the same reason: DTZ
            # counts to the next ZEROING move, so in any family with a pawn or
            # a capturable enemy piece "progress" is measured toward a pawn
            # push or a capture rather than toward mate. KPP-K reads 0.078 not
            # because the engine is lost but because every pawn move resets
            # the count. Report the rate everywhere -- it is still a valid
            # within-family comparison between two engine versions -- but mark
            # where it may be read as technique against optimal play.
            dtm_comparable = not weak and chess.PAWN not in strong
            if dtm_comparable and efficiency_pairs:
                entry["mate_efficiency"] = round(
                    statistics.median(p / d for p, d in efficiency_pairs), 3
                )
                entry["mate_efficiency_n"] = len(efficiency_pairs)
            else:
                entry["mate_efficiency"] = None
                entry["mate_efficiency_n"] = 0
                entry["mate_efficiency_note"] = (
                    "reported only for pawnless strong side vs bare king, "
                    "where DTZ equals DTM"
                )
            if args.per_position:
                entry["positions"] = records
            report["families"][name] = entry
            print(
                f"{name}: conversion "
                f"{entry['conversion_rate'] if entry['conversion_rate'] is None else round(entry['conversion_rate'], 3)}"
                f" on {won_positions} won; win-preserving "
                f"{entry['win_preserving_rate'] if entry['win_preserving_rate'] is None else round(entry['win_preserving_rate'], 4)}"
                f" over {graded} moves",
                flush=True,
            )
    finally:
        engine.quit()
        tb.close()

    rendered = json.dumps(report, indent=2) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(rendered, encoding="utf-8", newline="\n")
        print(f"Report: {args.output.resolve()}")
    else:
        print(rendered)
    return 0


if __name__ == "__main__":
    sys.exit(main())
