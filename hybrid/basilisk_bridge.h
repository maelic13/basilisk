/*
  Basilisk/Stockfish HCE oracle — bridge interface (PLAN 5.1).

  This header is included by Stockfish translation units and therefore must
  NOT include any Basilisk header. Basilisk and Stockfish both define Color,
  PieceType, Square, Bitboard, Value, WHITE, PAWN and many more at global
  scope; a single translation unit that saw both would not compile, and any
  workaround that made it compile (macro renaming, `#define private public`,
  include ordering tricks) would be exactly the kind of entanglement this
  experiment must not have.

  So the bridge is a deliberate firewall: this header is plain C++ over
  primitive types only, and basilisk_bridge.cpp is the single translation
  unit that sees Basilisk headers. It compiles against Basilisk's real source
  with no modifications to that source.

  This file is distributed under the GNU General Public License version 3
  or (at your option) any later version.
*/

#ifndef BASILISK_BRIDGE_H_INCLUDED
#define BASILISK_BRIDGE_H_INCLUDED

#include <cstdint>
#include <string>

namespace BasiliskBridge {

// Initializes Basilisk's global tables (bitboards, attacks, Zobrist keys,
// evaluation tables). Must be called once before evaluate(). Returns false and
// fills `error` on failure.
bool initialize(std::string& error);

// Sentinel returned by evaluate() when the incoming position is malformed
// (missing or duplicated king). The caller must treat it as fatal: a corrupt
// position silently entering the search would invalidate the whole experiment.
constexpr std::int32_t EvalError = INT32_MIN;

// Evaluates a position with Basilisk's unmodified 1.9.3 HCE.
//
//   pieces   12 bitboards, indexed color * 6 + (pieceType - 1), with
//            pieceType following the PAWN=1..KING=6 numbering that Basilisk
//            and Stockfish 9587eeeb share, and color WHITE=0, BLACK=1.
//   stm      side to move, WHITE=0, BLACK=1.
//   castling bit 0 = white kingside, 1 = white queenside, 2 = black kingside,
//            3 = black queenside. Basilisk's WK/WQ/BK/BQ_CASTLE and
//            Stockfish's WHITE_OO/WHITE_OOO/BLACK_OO/BLACK_OOO agree on these
//            values, so the field transfers unchanged.
//   rule50   halfmove clock, already clamped to [0, 100] by the caller.
//
// Returns centipawns from the side-to-move point of view — Basilisk's own
// convention — or EvalError.
//
// En passant is deliberately not transferred: Basilisk's evaluator reads
// pieces, occupancy, all_occ, board_sq, king_sq, side_to_move, halfmove_clock,
// castling_rights and pawn_key, and no evaluation term consults ep_sq. The
// conformance test enforces this claim rather than trusting it.
std::int32_t evaluate(const std::uint64_t* pieces,
                      std::uint8_t stm,
                      std::uint8_t castling,
                      std::uint8_t rule50);

// Evaluates a position given as a FEN string, using Basilisk's own validated
// parser and a fully derived board. Used only by the conformance test as the
// reference the fast path must reproduce exactly. Returns EvalError if the FEN
// does not parse.
std::int32_t evaluate_fen(const std::string& fen);

} // namespace BasiliskBridge

#endif // BASILISK_BRIDGE_H_INCLUDED
