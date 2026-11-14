#pragma once
// The search-replay trace recorder's hooks (board_trace.cpp); empty unless
// BASILISK_BOARD_TRACE is defined.
#ifdef BASILISK_BOARD_TRACE
#include "board_trace_format.h"

class Board;

namespace board_trace {

extern const Board* g_board;  // the board the running search mutates, or nullptr
extern int          g_depth;  // nesting depth of hooked Board methods
extern bool         g_in_eval;

void begin_search(const Board& b);
void end_search();
void put(Op op, uint8_t aux, uint16_t move, int32_t arg);

struct EvalScope {
    bool prev;
    EvalScope() : prev(g_in_eval) { g_in_eval = true; }
    ~EvalScope() { g_in_eval = prev; }
};

}  // namespace board_trace
#endif
