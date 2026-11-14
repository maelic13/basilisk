#pragma once
// Record format of the search-replay board trace, shared by the recorder
// (board_trace.cpp, BASILISK_BOARD_TRACE builds) and tests/board_replay.cpp.
#include <cstdint>

namespace board_trace {

enum Op : uint8_t {
    ROOT = 1,            // arg = index of the FEN line in the .fens side file
    MAKE,                // move; arg = low 32 bits of position_key() after
    UNMAKE,              // move
    NULL_MAKE,
    NULL_UNMAKE,
    GEN_LEGAL,           // arg = moves generated
    GEN_CAPS,
    GEN_QUIETS,
    GEN_QUIET_CHECKS,
    SEE,                 // move; arg = threshold; aux = result
    GIVES_CHECK,         // move; aux = result
    CHECK_SQUARES,       // aux: bits 0-2 piece type, bit 6 = called inside Evaluator, bit 7 = us; arg = lo ^ hi of result
    IS_LEGAL,            // move; aux = result
    IS_DRAW_PLY,         // arg = search_ply; aux = result
    IS_REPETITION,       // arg = search_ply; aux = result
    UPCOMING_REPETITION, // arg = search_ply; aux = result
    IN_CHECK,            // aux = result
    HAS_NPM,             // aux: bit 0 = result, bit 7 = colour argument
    OP_COUNT
};

struct Rec {
    uint8_t  op;
    uint8_t  aux;
    uint16_t move;
    int32_t  arg;
};
static_assert(sizeof(Rec) == 8, "trace record must be 8 bytes");

inline const char* op_name(unsigned op) {
    static const char* const names[] = {
        "?", "ROOT", "MAKE", "UNMAKE", "NULL_MAKE", "NULL_UNMAKE", "GEN_LEGAL", "GEN_CAPS",
        "GEN_QUIETS", "GEN_QUIET_CHECKS", "SEE", "GIVES_CHECK", "CHECK_SQUARES", "IS_LEGAL",
        "IS_DRAW_PLY", "IS_REPETITION", "UPCOMING_REPETITION", "IN_CHECK", "HAS_NPM"};
    return op < OP_COUNT ? names[op] : "?";
}

}  // namespace board_trace
