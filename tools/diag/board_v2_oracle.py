#!/usr/bin/env python3
"""Generate or check the board-v2 oracle with python-chess.

The oracle shares no code with the engine. For every position of
tests/data/board_v2.tsv it records the canonical FEN (en-passant square only
when a legal capture exists), the checkers, the sorted legal moves, the
captures among them, the moves that give check, and perft with its divide.
test_board_v2 compares the engine with it. Every profile tag is checked
against python-chess first, so a FEN that stops showing what its tag claims
fails here rather than silently thinning the corpus.

    python tools/diag/board_v2_oracle.py          # check the file is current
    python tools/diag/board_v2_oracle.py --write  # regenerate it
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

import chess

ROOT = Path(__file__).resolve().parents[2]
PROFILE = ROOT / "tests" / "data" / "board_v2.tsv"
ORACLE = ROOT / "tests" / "data" / "board_v2_oracle.tsv"
PYTHON_CHESS = "1.11.2"

REQUIRED_TAGS = {
    "single-check", "double-check", "evasion",
    "ep-legal", "ep-illegal", "ep-evasion", "ep-check",
    "quiet-promotion", "capture-promotion", "underpromotion",
    "underpromotion-check",
    "castle-white-king", "castle-white-queen",
    "castle-black-king", "castle-black-queen",
    "castle-blocked", "castle-check",
    "discovered-check", "pin", "stalemate", "checkmate",
    "sparse-endgame", "long-history",
}
KNOWN_TAGS = REQUIRED_TAGS | {"baseline"}


def load_profile(path: Path) -> list[tuple[str, list[str], str, int]]:
    rows = []
    for number, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        if not raw.strip() or raw.startswith("#"):
            continue
        fields = [field.strip() for field in raw.split("|")]
        if len(fields) != 4:
            raise SystemExit(f"{path}:{number}: expected four '|' fields")
        name, tags_text, fen, depth_text = fields
        tags = sorted(tags_text.split(","))
        unknown = set(tags) - KNOWN_TAGS
        if unknown:
            raise SystemExit(f"{path}:{number}: unknown tags {sorted(unknown)}")
        rows.append((name, tags, fen, int(depth_text)))
    names = [row[0] for row in rows]
    if len(set(names)) != len(names):
        raise SystemExit(f"{path}: names must be unique")
    present = set().union(*(set(row[1]) for row in rows))
    missing = REQUIRED_TAGS - present
    if missing:
        raise SystemExit(f"{path}: no position carries {sorted(missing)}")
    return rows


def is_discovered_check(board: chess.Board, move: chess.Move) -> bool:
    """The move gives check and the piece that moved is not a checker."""
    if not board.gives_check(move):
        return False
    board.push(move)
    try:
        king = board.king(board.turn)
        return move.to_square not in board.attackers(not board.turn, king) \
            or len(board.checkers()) > 1
    finally:
        board.pop()


def require_tags(name: str, tags: list[str], board: chess.Board) -> None:
    def need(condition: bool, what: str) -> None:
        if not condition:
            raise SystemExit(f"{name}: expected {what}")

    legal = list(board.legal_moves)
    checkers = len(board.checkers())
    ep_moves = [m for m in legal if board.is_en_passant(m)]
    promotions = [m for m in legal if m.promotion]
    castles = {m.uci() for m in legal if board.is_castling(m)}
    for tag in tags:
        if tag == "single-check":
            need(checkers == 1, "one checker")
        elif tag == "double-check":
            need(checkers == 2, "two checkers")
        elif tag == "evasion":
            need(checkers > 0 and bool(legal), "a nonempty evasion set")
        elif tag == "ep-legal":
            need(bool(ep_moves), "a legal en-passant capture")
        elif tag == "ep-illegal":
            need(board.ep_square is not None and not ep_moves,
                 "an en-passant token with no legal capture")
        elif tag == "ep-evasion":
            need(checkers > 0 and bool(ep_moves), "an en-passant evasion")
        elif tag == "ep-check":
            need(any(board.gives_check(m) for m in ep_moves),
                 "an en-passant capture that gives check")
        elif tag == "quiet-promotion":
            need(any(not board.is_capture(m) for m in promotions), "a quiet promotion")
        elif tag == "capture-promotion":
            need(any(board.is_capture(m) for m in promotions), "a capture promotion")
        elif tag == "underpromotion":
            need({m.promotion for m in promotions} >= {chess.KNIGHT, chess.BISHOP, chess.ROOK},
                 "knight, bishop and rook promotions")
        elif tag == "underpromotion-check":
            need(any(m.promotion != chess.QUEEN and board.gives_check(m)
                     and not board.gives_check(chess.Move(m.from_square, m.to_square, chess.QUEEN))
                     for m in promotions),
                 "an underpromotion that checks where the queen does not")
        elif tag.startswith("castle-") and tag[7:] in ("white-king", "white-queen",
                                                        "black-king", "black-queen"):
            uci = {"white-king": "e1g1", "white-queen": "e1c1",
                   "black-king": "e8g8", "black-queen": "e8c8"}[tag[7:]]
            need(uci in castles, f"castling move {uci}")
        elif tag == "castle-blocked":
            rank = "1" if board.turn == chess.WHITE else "8"
            blocked = [uci for right, uci in (
                (board.has_kingside_castling_rights(board.turn), f"e{rank}g{rank}"),
                (board.has_queenside_castling_rights(board.turn), f"e{rank}c{rank}"))
                if right and uci not in castles]
            need(not board.is_check() and bool(blocked),
                 "a castling right whose castle is illegal, not in check")
        elif tag == "castle-check":
            need(any(board.gives_check(chess.Move.from_uci(u)) for u in castles),
                 "a castling move that gives check")
        elif tag == "discovered-check":
            need(any(is_discovered_check(board, m) for m in legal), "a discovered check")
        elif tag == "pin":
            need(any(board.is_pinned(board.turn, sq)
                     for sq in chess.SquareSet(board.occupied_co[board.turn])
                     if sq != board.king(board.turn)), "a pinned piece")
        elif tag == "stalemate":
            need(board.is_stalemate(), "stalemate")
        elif tag == "checkmate":
            need(board.is_checkmate(), "checkmate")
        elif tag == "sparse-endgame":
            need(len(board.piece_map()) <= 10, "at most ten pieces")
        elif tag == "long-history":
            need(board.halfmove_clock >= 90 and board.fullmove_number >= 100,
                 "a long clock")


def perft(board: chess.Board, depth: int) -> int:
    if depth == 0:
        return 1
    if depth == 1:
        return board.legal_moves.count()
    total = 0
    for move in board.legal_moves:
        board.push(move)
        total += perft(board, depth - 1)
        board.pop()
    return total


def render(rows: list[tuple[str, list[str], str, int]]) -> str:
    lines = [
        f"# board-v2 oracle; python-chess {PYTHON_CHESS}; "
        "generated by tools/diag/board_v2_oracle.py, do not edit",
        "# name|tags|input FEN|canonical FEN|depth|checkers|legal|captures|checks|perft|divide",
    ]
    for name, tags, fen, depth in rows:
        board = chess.Board(fen)
        if not board.is_valid():
            raise SystemExit(f"{name}: python-chess rejects {fen}: {board.status()!r}")
        require_tags(name, tags, board)
        legal = sorted(board.legal_moves, key=chess.Move.uci)
        divide = []
        for move in legal:
            board.push(move)
            divide.append((move.uci(), perft(board, depth - 1)))
            board.pop()
        checkers = sorted(chess.square_name(sq) for sq in board.checkers())
        lines.append("|".join((
            name,
            ",".join(tags),
            fen,
            board.fen(en_passant="legal"),
            str(depth),
            " ".join(checkers) or "-",
            " ".join(m.uci() for m in legal) or "-",
            " ".join(m.uci() for m in legal if board.is_capture(m)) or "-",
            " ".join(m.uci() for m in legal if board.gives_check(m)) or "-",
            str(sum(nodes for _, nodes in divide)),
            " ".join(f"{uci}:{nodes}" for uci, nodes in divide) or "-",
        )))
    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--write", action="store_true", help="regenerate the oracle file")
    args = parser.parse_args()
    if chess.__version__ != PYTHON_CHESS:
        print(f"board-v2 oracle: python-chess {chess.__version__}, expected {PYTHON_CHESS}",
              file=sys.stderr)
        return 2
    expected = render(load_profile(PROFILE))
    cases = len(expected.splitlines()) - 2
    if args.write:
        ORACLE.write_bytes(expected.encode("utf-8"))
        print(f"board-v2 oracle: wrote {cases} cases to {ORACLE.relative_to(ROOT)}")
        return 0
    actual = ORACLE.read_text(encoding="utf-8").replace("\r\n", "\n")
    if actual != expected:
        print(f"board-v2 oracle: {ORACLE.relative_to(ROOT)} is stale; rerun with --write",
              file=sys.stderr)
        return 1
    print(f"board-v2 oracle: PASS ({cases} cases)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
