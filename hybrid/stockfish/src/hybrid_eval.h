/*
  Basilisk/Stockfish HCE oracle adapter (Basilisk PLAN 5.1).

  This file is distributed under the GNU General Public License version 3
  or (at your option) any later version.
*/

#ifndef HYBRID_EVAL_H_INCLUDED
#define HYBRID_EVAL_H_INCLUDED

#include <string>

#include "types.h"

class Position;

namespace HybridEval {

bool initialize(std::string& error);
bool enabled();
void set_enabled(bool value);
Value evaluate(const Position& pos);

} // namespace HybridEval

#endif // HYBRID_EVAL_H_INCLUDED
