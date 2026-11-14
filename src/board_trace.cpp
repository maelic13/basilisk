// Search-replay trace recorder, compiled only into a BASILISK_BOARD_TRACE build.
//
// Every public Board operation the search makes on its own board is recorded
// in order with its result, eight bytes a record (board_trace_format.h), to
// the file BASILISK_BOARD_TRACE_FILE names; each search's root goes to the
// side file <file>.fens. With the variable unset nothing is recorded. One
// search at a time: a trace of a multi-threaded search would interleave.
#ifdef BASILISK_BOARD_TRACE
#include "board_trace.h"
#include "board.h"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace board_trace {

const Board* g_board = nullptr;
int          g_depth = 0;
bool         g_in_eval = false;

namespace {

constexpr size_t CHUNK = size_t(1) << 20;  // records per write

struct Sink {
    std::FILE* trace = nullptr;
    std::FILE* fens  = nullptr;
    std::vector<Rec> buf;
    int64_t fen_lines = 0;
    int64_t total = 0;
    bool tried = false;
    bool open() {
        if (tried) return trace != nullptr;
        tried = true;
        const char* path = std::getenv("BASILISK_BOARD_TRACE_FILE");
        if (!path) return false;
        trace = std::fopen(path, "wb");
        fens  = std::fopen((std::string(path) + ".fens").c_str(), "wb");
        if (!trace || !fens) {
            std::fprintf(stderr, "board trace: cannot write %s\n", path);
            std::abort();
        }
        buf.reserve(CHUNK);
        return true;
    }
    void flush() {
        if (!trace) return;
        if (!buf.empty()) std::fwrite(buf.data(), sizeof(Rec), buf.size(), trace);
        buf.clear();
        std::fflush(trace);
        if (fens) std::fflush(fens);
    }
    ~Sink() {
        flush();
        if (trace) std::fclose(trace);
        if (fens) std::fclose(fens);
    }
};

Sink g_sink;

}  // namespace

void put(Op op, uint8_t aux, uint16_t move, int32_t arg) {
    if (!g_sink.trace) return;
    g_sink.buf.push_back(Rec{uint8_t(op), aux, move, arg});
    ++g_sink.total;
    if (g_sink.buf.size() >= CHUNK) {
        std::fwrite(g_sink.buf.data(), sizeof(Rec), g_sink.buf.size(), g_sink.trace);
        g_sink.buf.clear();
    }
}

void begin_search(const Board& b) {
    if (!g_sink.open()) return;
    if (g_board) {
        std::fprintf(stderr, "board trace: a second search started while one runs; "
                             "record with Threads 1\n");
        std::abort();
    }
    g_depth = 0;
    const int64_t line = g_sink.fen_lines++;
    std::fprintf(g_sink.fens, "%s\t%zu\n", b.get_fen().c_str(), b.history_size());
    g_board = &b;
    put(ROOT, 0, 0, int32_t(line));
}

void end_search() {
    g_board = nullptr;
    g_depth = 0;
    g_sink.flush();
}

}  // namespace board_trace

// ---- Hooked Board methods: record, then forward to the *_untraced original ----
using namespace board_trace;

namespace {
// Depth bookkeeping for one hooked call. `rec` is true only for a depth-0 call on
// the search's board; every hooked call on that board raises the depth, so a
// Board method called from inside another hooked one is not recorded.
struct Scope {
    bool on, rec;
    explicit Scope(const Board* self) : on(self == g_board), rec(on && g_depth == 0) {
        if (on) ++g_depth;
    }
    ~Scope() { if (on) --g_depth; }
};
inline uint8_t u8(bool b) { return b ? 1 : 0; }
inline int32_t fold(Bitboard b) { return int32_t(uint32_t(b) ^ uint32_t(b >> 32)); }
}  // namespace

void Board::make_move(Move m) {
    Scope s(this);
    make_move_untraced(m);
    if (s.rec) put(MAKE, 0, m, int32_t(uint32_t(hash)));
}
void Board::unmake_move(Move m) {
    Scope s(this);
    unmake_move_untraced(m);
    if (s.rec) put(UNMAKE, 0, m, 0);
}
void Board::make_null_move() {
    Scope s(this);
    make_null_move_untraced();
    if (s.rec) put(NULL_MAKE, 0, 0, 0);
}
void Board::unmake_null_move() {
    Scope s(this);
    unmake_null_move_untraced();
    if (s.rec) put(NULL_UNMAKE, 0, 0, 0);
}
void Board::gen_legal(MoveList& ml) const {
    Scope s(this);
    gen_legal_untraced(ml);
    if (s.rec) put(GEN_LEGAL, 0, 0, ml.size());
}
void Board::gen_legal_captures(MoveList& ml) const {
    Scope s(this);
    gen_legal_captures_untraced(ml);
    if (s.rec) put(GEN_CAPS, 0, 0, ml.size());
}
void Board::gen_legal_quiets(MoveList& ml) const {
    Scope s(this);
    gen_legal_quiets_untraced(ml);
    if (s.rec) put(GEN_QUIETS, 0, 0, ml.size());
}
void Board::gen_quiet_checks(MoveList& ml) const {
    Scope s(this);
    gen_quiet_checks_untraced(ml);
    if (s.rec) put(GEN_QUIET_CHECKS, 0, 0, ml.size());
}
bool Board::see_ge(Move m, int threshold) const {
    Scope s(this);
    const bool r = see_ge_untraced(m, threshold);
    if (s.rec) put(SEE, u8(r), m, threshold);
    return r;
}
bool Board::gives_check(Move m) const {
    Scope s(this);
    const bool r = gives_check_untraced(m);
    if (s.rec) put(GIVES_CHECK, u8(r), m, 0);
    return r;
}
Bitboard Board::check_squares(PieceType pt, Color us) const {
    Scope s(this);
    const Bitboard r = check_squares_untraced(pt, us);
    if (s.rec)
        put(CHECK_SQUARES, uint8_t(int(pt) | (g_in_eval ? 0x40 : 0) | (int(us) << 7)), 0, fold(r));
    return r;
}
bool Board::is_legal(Move m) const {
    Scope s(this);
    const bool r = is_legal_untraced(m);
    if (s.rec) put(IS_LEGAL, u8(r), m, 0);
    return r;
}
bool Board::is_draw(int search_ply) const {
    Scope s(this);
    const bool r = is_draw_untraced(search_ply);
    if (s.rec) put(IS_DRAW_PLY, u8(r), 0, search_ply);
    return r;
}
bool Board::is_repetition(int search_ply) const {
    Scope s(this);
    const bool r = is_repetition_untraced(search_ply);
    if (s.rec) put(IS_REPETITION, u8(r), 0, search_ply);
    return r;
}
bool Board::upcoming_repetition(int search_ply) const {
    Scope s(this);
    const bool r = upcoming_repetition_untraced(search_ply);
    if (s.rec) put(UPCOMING_REPETITION, u8(r), 0, search_ply);
    return r;
}
bool Board::is_in_check() const {
    Scope s(this);
    const bool r = is_in_check_untraced();
    if (s.rec) put(IN_CHECK, u8(r), 0, 0);
    return r;
}
bool Board::has_non_pawn_material(Color c) const {
    Scope s(this);
    const bool r = has_non_pawn_material_untraced(c);
    if (s.rec) put(HAS_NPM, uint8_t(u8(r) | (int(c) << 7)), 0, 0);
    return r;
}
#endif
