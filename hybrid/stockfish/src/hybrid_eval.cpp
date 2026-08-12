/*
  Basilisk/Stockfish HCE oracle — Stockfish-side adapter (Basilisk PLAN 5.1).

  Adapted from the Rarog Stage-1 hybrid, with the dynamic-library boundary
  removed: Basilisk is C++, so its evaluator links directly and there is no
  DLL, no exported C ABI and no ABI-version handshake to get wrong. What
  remains is the type firewall in basilisk_bridge.h, which exists because
  Basilisk and Stockfish collide on Color, PieceType, Square, Value and more.

  This file is distributed under the GNU General Public License version 3
  or (at your option) any later version.
*/

#include "hybrid_eval.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <limits>

#include "position.h"

#include "../../basilisk_bridge.h"

namespace HybridEval {
namespace {

bool useBasiliskHce = true;

} // namespace

bool initialize(std::string& error) {
  return BasiliskBridge::initialize(error);
}

bool enabled() {
  return useBasiliskHce;
}

void set_enabled(bool value) {
  useBasiliskHce = value;
}

Value evaluate(const Position& pos) {
  std::uint64_t pieces[12];
  for (Color color : {WHITE, BLACK})
      for (PieceType piece = PAWN; piece <= KING; ++piece)
          pieces[color * 6 + piece - 1] = pos.pieces(color, piece);

  // Basilisk's rule-50 damping curve is defined over a clock of 0..100; a
  // larger value would push the damping numerator negative and flip the sign
  // of the evaluation.
  const int rule50 = std::clamp(pos.rule50_count(), 0, 100);

  // Both engines number castling rights identically (white kingside 1,
  // white queenside 2, black kingside 4, black queenside 8), so the field
  // crosses unchanged.
  const auto raw = BasiliskBridge::evaluate(
      pieces,
      static_cast<std::uint8_t>(pos.side_to_move()),
      static_cast<std::uint8_t>(pos.castling_rights(WHITE) | pos.castling_rights(BLACK)),
      static_cast<std::uint8_t>(rule50));

  if (raw == BasiliskBridge::EvalError)
      std::abort(); // A corrupt position must never silently enter the search.

  // Basilisk scores in centipawns from the side-to-move point of view, which
  // is also Stockfish's convention for Eval::evaluate. Only the unit differs:
  // Stockfish's internal pawn is PawnValueEg, not 100.
  const std::int64_t numerator = static_cast<std::int64_t>(raw) * int(PawnValueEg);
  const std::int64_t scaled = numerator >= 0 ? (numerator + 50) / 100
                                             : (numerator - 50) / 100;

  // Clamped strictly inside VALUE_KNOWN_WIN so that a static evaluation can
  // never be mistaken for a proven win or a mate score by the search.
  return Value(std::max<std::int64_t>(-VALUE_KNOWN_WIN + 1,
               std::min<std::int64_t>(VALUE_KNOWN_WIN - 1, scaled)));
}

} // namespace HybridEval
