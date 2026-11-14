#!/usr/bin/env python3
"""Independent SEE oracle corpus for Basilisk (python-chess, no engine code).

For every SEE fixture the C++ tests assert (plus the Rarog contract cases,
their colour mirrors, and the move sets the tests sweep), compute:

  tree  legal same-square capture tree: each side in turn may play ANY legal
        capture onto the square (every promotion choice) or stop; replies are
        floored at 0.
  lva   the same exchange restricted to each side's least valuable LEGAL
        attacker (ties: larger promotion, then lower from-square), may stop.
  gain  immediate gain of the initial move.

Castling is outside the exchange model (its king lands on an unattacked
square): tree = lva = gain = 0.

Usage:  see_oracle.py --write   write tests/data/see_v1.tsv
        see_oracle.py           check the file is byte-identical to a rewrite
"""

import argparse
import sys
from pathlib import Path

import chess

ROOT = Path(__file__).resolve().parents[2]
OUTPUT = ROOT / "tests" / "data" / "see_v1.tsv"
PYTHON_CHESS = "1.11.2"
VALUES = {chess.PAWN: 100, chess.KNIGHT: 300, chess.BISHOP: 300,
          chess.ROOK: 500, chess.QUEEN: 900, chess.KING: 20000}
HEADER = "# name|source|FEN|uci|tree|lva|gain|asserted"

# --- fixtures asserted by tests/test_board.cpp -----------------------------
# (name, source, FEN, uci, asserted)
TB = "test_board:"
S_SEE = TB + "test_see"
S_PIN = TB + "test_see_pin_legality"
S_KING = TB + "test_see_king_legality"
S_CREATED = TB + "test_see_created_pins_and_promotions"
ASSERTED = [
    ("pawn captures undefended pawn", S_SEE, "4k3/8/8/4p3/3P4/8/8/4K3 w - - 0 1", "d4e5", 100),
    ("rook captures undefended pawn", S_SEE, "4k3/8/8/4p3/4R3/8/8/4K3 w - - 0 1", "e4e5", 100),
    ("pawn captures undefended knight", S_SEE, "4k3/8/8/3n4/4P3/8/8/4K3 w - - 0 1", "e4d5", 300),
    ("pawn captures undefended queen", S_SEE, "7k/8/8/3q4/4P3/8/8/K7 w - - 0 1", "e4d5", 900),
    ("rook captures undefended rook", S_SEE, "7k/4r3/8/8/4R3/8/8/K7 w - - 0 1", "e4e7", 500),
    ("SEE detects defended losing capture", S_SEE,
     "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", "e5g6", -200),
    ("second vector: bishop for a knight, first vector",
     TB + "test_see_on_the_second_vector",
     "4k3/2p5/3n4/8/1B6/8/8/K7 w - - 0 1", "b4d6", 0),
    ("pinned knight cannot recapture (audit repro)", S_PIN,
     "4k3/4n3/2p5/1B6/8/8/8/K3R3 w - - 0 1", "b5c6", 100),
    ("unpinned twin: knight recaptures", S_PIN,
     "4k3/4n3/2p5/1B6/8/8/8/K2R4 w - - 0 1", "b5c6", -200),
    ("pinned bishop cannot recapture", S_PIN,
     "3k4/3b4/2p5/8/3N4/8/8/K2R4 w - - 0 1", "d4c6", 100),
    ("pinned pawn cannot recapture", S_PIN,
     "3k4/3p4/2n5/8/3N4/8/8/K2R4 w - - 0 1", "d4c6", 300),
    ("pinner departs: pin released mid-exchange", S_PIN,
     "4k3/8/8/4n3/6p1/7P/4Q3/K7 w - - 0 1", "e2g4", -500),
    ("pinner stays: pin holds during exchange", S_PIN,
     "4k3/8/8/4n3/6p1/5B1P/4Q3/K7 w - - 0 1", "f3g4", 100),
    ("king recapture (legal): queen sac refused", S_PIN,
     "4k3/4p3/8/8/8/8/8/4QK2 w - - 0 1", "e1e7", -800),
    ("king recapture (illegal): sentinel folds to legality", S_PIN,
     "4k3/4p3/8/8/7B/8/8/4QK2 w - - 0 1", "e1e7", 100),
    ("king legality: legal king recapture after pawn", S_KING,
     "7k/8/2p5/3pK3/8/8/3R4/8 w - - 0 1", "d2d5", -300),
    ("king legality: legal king recapture of a queen", S_KING,
     "3qk3/8/8/8/8/8/3Q4/3K4 w - - 0 1", "d2d8", 0),
    ("king legality: defended destination refuses king recapture", S_KING,
     "3qk3/8/8/6B1/8/8/3Q4/3K4 w - - 0 1", "d2d8", 900),
    ("king legality: king as the initial capturer", S_KING,
     "k7/7p/8/3n4/4K3/8/7P/8 w - - 0 1", "e4d5", 300),
    ("king legality: mirror legal king recapture after pawn", S_KING,
     "8/3r4/8/8/3Pk3/2P5/8/7K b - - 0 1", "d7d4", -300),
    ("king legality: mirror legal king recapture of a queen", S_KING,
     "3k4/3q4/8/8/8/8/8/3QK3 b - - 0 1", "d7d1", 0),
    ("king legality: mirror defended destination", S_KING,
     "3k4/3q4/8/8/6b1/8/8/3QK3 b - - 0 1", "d7d1", 900),
    ("king legality: pinned defender still forbids Kxf6", S_KING,
     "8/6k1/5p2/8/3B3K/8/5Q2/4b3 w - - 0 1", "d4f6", 100),
    ("king legality: pinned defender, mirror", S_KING,
     "4B3/5q2/8/3b3k/8/5P2/6K1/8 b - - 0 1", "d5f3", 100),
    ("created pin: opened by the mover's departure", S_CREATED,
     "2k5/2n5/2B5/3p4/8/8/8/2R1K3 w - - 0 1", "c6d5", 100),
    ("created pin: mover's departure, mirror", S_CREATED,
     "2r1k3/8/8/8/3P4/2b5/2N5/2K5 b - - 0 1", "c3d4", 100),
    ("created pin: skip the pinned attacker, choose the rook", S_CREATED,
     "2kr4/2n5/2B5/3p4/8/8/8/2R1K3 w - - 0 1", "c6d5", -200),
    ("created pin: opened mid-exchange", S_CREATED,
     "2k5/2n5/2B1p3/3p4/8/8/3R4/2R4K w - - 0 1", "d2d5", -300),
    ("promotion as the initial capture: queen", S_CREATED,
     "1r2k3/P7/8/8/8/8/8/4K3 w - - 0 1", "a7b8q", 1300),
    ("promotion recapture: the queen is recaptured", S_CREATED,
     "7k/8/8/8/8/8/pR6/1rR1K3 w - - 0 1", "b2b1", 100),
    ("promotion recapture: the promoted queen stands", S_CREATED,
     "7k/8/8/8/8/7K/pR6/1r6 w - - 0 1", "b2b1", -800),
    ("promotion recapture: the promoted queen stands, mirror", S_CREATED,
     "1R6/Pr6/7k/8/8/8/8/7K b - - 0 1", "b7b8", -800),
    ("promotion recapture: the pawn is pinned", S_CREATED,
     "R6K/8/8/8/8/8/pR6/kr6 w - - 0 1", "b2b1", 0),
]

# Move sets the tests sweep without asserting a value.
SWEEP_THRESHOLD = [  # test_see "threshold SEE matches full SEE": tactical moves
    "4k3/8/8/3n4/4P3/8/8/4K3 w - - 0 1",
    "4k2r/6P1/8/8/8/8/8/4K3 w - - 0 1",
    "4k3/P7/8/8/8/8/8/4K3 w - - 0 1",
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
]
SWEEP_SECOND = [  # second-vector sweep: captures
    "r1bq1rk1/ppp2ppp/2np1n2/2b1p3/2B1P3/2NP1N2/PPP2PPP/R1BQ1RK1 w - - 0 1",
]
SWEEP_QUIET = [  # test_see_quiet_moves: every legal quiet
    "4k3/8/8/3n4/4P3/8/8/4K3 w - - 0 1",
    "4k2r/6P1/8/8/8/8/8/4K3 w - - 0 1",
    "4k3/P7/8/8/8/8/8/4K3 w - - 0 1",
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
    "4k3/8/4p3/8/8/8/8/3QK3 w - - 0 1",
    "4k3/4r3/8/8/8/8/4R3/4K3 w - - 0 1",
    "4k3/8/2n5/8/2B5/8/8/2R1K3 b - - 0 1",
]

# --- Rarog reference cases (name, FEN, uci); data only ----------------------
CONTRACT = [
    ("free-pawn", "4k3/8/8/3p4/8/8/3R4/4K3 w - - 0 1", "d2d5"),
    ("defended-pawn", "4k3/8/2p5/3p4/8/8/3R4/4K3 w - - 0 1", "d2d5"),
    ("king-after-pawn", "7k/8/2p5/3pK3/8/8/3R4/8 w - - 0 1", "d2d5"),
    ("legal-king-recapture", "3qk3/8/8/8/8/8/3Q4/3K4 w - - 0 1", "d2d8"),
    ("defended-king-destination", "3qk3/8/8/6B1/8/8/3Q4/3K4 w - - 0 1", "d2d8"),
    ("pinned-pawn", "3k4/3p4/2p5/8/4B3/8/8/3RK3 w - - 0 1", "e4c6"),
    ("pin-created", "2k5/2n5/2B5/3p4/8/8/8/2R1K3 w - - 0 1", "c6d5"),
    ("pin-released", "k7/1r1p4/2B5/8/8/8/8/4K3 w - - 0 1", "c6d7"),
    ("xray", "3rk3/8/8/3r4/8/8/3R4/3RK3 w - - 0 1", "d2d5"),
    ("ep-opens-rook", "7k/8/8/3pP3/8/8/K7/3r4 w - d6 0 1", "e5d6"),
    ("quiet-hanging", "7k/8/8/2p5/8/8/3R4/7K w - - 0 1", "d2d4"),
    ("quiet-promotion-hanging", "7r/P7/7k/8/8/8/8/K7 w - - 0 1", "a7a8q"),
    ("quiet-underpromotion", "4k3/P7/8/8/8/8/8/4K3 w - - 0 1", "a7a8n"),
    ("capture-promotion", "1r2k3/P7/8/8/8/8/8/4K3 w - - 0 1", "a7b8q"),
    ("capture-underpromotion", "1r2k3/P7/8/8/8/8/8/4K3 w - - 0 1", "a7b8n"),
    ("promotion-recapture", "7k/8/8/8/8/7K/pR6/1r6 w - - 0 1", "b2b1"),
    ("castle-king", "4k3/8/8/8/8/8/8/R3K2R w KQ - 0 1", "e1g1"),
    ("castle-queen", "4k3/8/8/8/8/8/8/R3K2R w KQ - 0 1", "e1c1"),
]
REPAIR = [
    ("promoted-piece-recaptured", "7k/8/8/8/8/8/pR6/1rR1K3 w - - 0 1", "b2b1"),
    ("pin-created-later", "2k5/2n5/2B1p3/3p4/8/8/3R4/2R4K w - - 0 1", "d2d5"),
    ("skip-pinned-choose-rook", "2kr4/2n5/2B5/3p4/8/8/8/2R1K3 w - - 0 1", "c6d5"),
    ("quiet-allows-promotion", "7k/8/8/8/8/7K/pR6/8 w - - 0 1", "b2b1"),
    ("initial-king-capture", "k7/7p/8/3n4/4K3/8/7P/8 w - - 0 1", "e4d5"),
]


def mirror_case(name, fen, uci):
    """Colour mirror, written here: flip ranks, swap colours."""
    b = chess.Board(fen).mirror()
    mv = chess.Move.from_uci(uci)
    mm = chess.Move(chess.square_mirror(mv.from_square),
                    chess.square_mirror(mv.to_square), promotion=mv.promotion)
    return ("mirror-" + name, b.fen(en_passant="fen"), mm.uci())


# --- the oracle -------------------------------------------------------------
def is_exchange_capture(board, move):
    return (not board.is_castling(move) and not board.is_en_passant(move)
            and board.piece_at(move.to_square) is not None)


def gain_of(board, move):
    """Immediate material gain of `move` on `board` (before it is played)."""
    if board.is_castling(move):
        return 0
    if board.is_en_passant(move):
        g = VALUES[chess.PAWN]
    else:
        victim = board.piece_at(move.to_square)
        g = VALUES[victim.piece_type] if victim else 0
    if move.promotion:
        g += VALUES[move.promotion] - VALUES[chess.PAWN]
    return g


def tree_reply(board, sq):
    """Value for the side to move of continuing on `sq`, any legal capture."""
    best = 0
    for m in list(board.legal_moves):
        if m.to_square != sq or not is_exchange_capture(board, m):
            continue
        g = gain_of(board, m)
        board.push(m)
        v = g - tree_reply(board, sq)
        board.pop()
        best = max(best, v)
    return best


def lva_reply(board, sq):
    """Same, but only the least valuable legal attacker (larger promotion on a tie)."""
    pick = None
    pick_key = None
    for m in board.legal_moves:
        if m.to_square != sq or not is_exchange_capture(board, m):
            continue
        pg = (VALUES[m.promotion] - VALUES[chess.PAWN]) if m.promotion else 0
        key = (VALUES[board.piece_at(m.from_square).piece_type], -pg, m.from_square)
        if pick_key is None or key < pick_key:
            pick, pick_key = m, key
    if pick is None:
        return 0
    g = gain_of(board, pick)
    board.push(pick)
    v = g - lva_reply(board, sq)
    board.pop()
    return max(0, v)


def evaluate(fen, uci):
    board = chess.Board(fen)
    if not board.is_valid():
        raise SystemExit("invalid FEN: %s (status %s)" % (fen, board.status()))
    move = chess.Move.from_uci(uci)
    if move not in board.legal_moves:
        raise SystemExit("illegal move %s in %s" % (uci, fen))
    if board.is_castling(move):
        return 0, 0, 0
    g = gain_of(board, move)
    sq = move.to_square
    board.push(move)
    t = g - tree_reply(board, sq)
    l = g - lva_reply(board, sq)
    board.pop()
    return t, l, g


def sweep(fens, source, select):
    rows = []
    for idx, fen in enumerate(fens):
        board = chess.Board(fen)
        if not board.is_valid():
            raise SystemExit("invalid FEN: " + fen)
        for m in sorted(board.legal_moves, key=lambda x: x.uci()):
            has_victim = board.piece_at(m.to_square) is not None
            tactical = bool(m.promotion) or board.is_en_passant(m) or has_victim
            if select == "tactical" and not tactical:
                continue
            if select == "capture" and not has_victim:
                continue
            if select == "quiet" and (tactical or board.is_castling(m)):
                continue
            tag = source.split(":")[1]
            rows.append(("%s:%d:%s" % (tag, idx, m.uci()), source, fen, m.uci(), "-"))
    return rows


def build_cases():
    cases = [(n, s, f, u, str(a)) for n, s, f, u, a in ASSERTED]
    cases += sweep(SWEEP_THRESHOLD, TB + "test_see:threshold-sweep", "tactical")
    cases += sweep(SWEEP_SECOND, TB + "test_see_on_the_second_vector:sweep", "capture")
    cases += sweep(SWEEP_QUIET, TB + "test_see_quiet_moves:fen-list", "quiet")
    for n, f, u in CONTRACT:
        cases.append((n, "rarog-contract", f, u, "-"))
    for n, f, u in REPAIR:
        cases.append((n, "rarog-repair", f, u, "-"))
    for n, f, u in CONTRACT:
        mn, mf, mu = mirror_case(n, f, u)
        cases.append((mn, "mirror", mf, mu, "-"))
    seen = set()
    for c in cases:
        if (c[0], c[1]) in seen:
            raise SystemExit("duplicate case name: %r" % (c[:2],))
        seen.add((c[0], c[1]))
    return cases


def render():
    lines = [HEADER]
    for name, source, fen, uci, asserted in build_cases():
        t, l, g = evaluate(fen, uci)
        if "|" in name:
            raise SystemExit("name contains |: " + name)
        lines.append("|".join([name, source, fen, uci, str(t), str(l), str(g), asserted]))
    return ("\n".join(lines) + "\n").encode("utf-8")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--write", action="store_true")
    args = ap.parse_args()
    if chess.__version__ != PYTHON_CHESS:
        print("see oracle: python-chess %s, expected %s" % (chess.__version__, PYTHON_CHESS),
              file=sys.stderr)
        return 2
    data = render()
    if args.write:
        OUTPUT.parent.mkdir(parents=True, exist_ok=True)
        OUTPUT.write_bytes(data)
        print("wrote %s (%d cases)" % (OUTPUT, data.count(b"\n") - 1))
        return 0
    # The checkout may hold a CRLF copy of the LF file.
    if not OUTPUT.exists() or OUTPUT.read_bytes().replace(b"\r\n", b"\n") != data:
        print("see_v1.tsv is missing or stale; run: python tools/diag/see_oracle.py --write",
              file=sys.stderr)
        return 1
    print("see_v1.tsv is current (%d cases)" % (data.count(b"\n") - 1))
    return 0


if __name__ == "__main__":
    sys.exit(main())
