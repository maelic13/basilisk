#pragma once

#include "types.h"
#include "bitboard.h"
#include "move.h"
#include "attacks.h"
#include "zobrist.h"
#include <memory>
#include <vector>
#include <expected>
#include <string>
#include <cstdint>

// Per-move undo information
struct UndoInfo {
    Key      hash;
    Key      pawn_key;
    Key      minor_key;
    Key      nonpawn_key[NCOLORS];
    Bitboard checkers;
    Square   ep_sq;
    int      castling;
    int      halfmove;
    int      plies_from_null;
    Piece    captured;
};

// Stack-allocated move list (256 slots — more than any legal position needs)
struct MoveList {
    static constexpr int CAPACITY = 256;
    Move moves[CAPACITY];
    int  count = 0;
    void push(Move m) noexcept { moves[count++] = m; }
    void reset()  noexcept { count = 0; }
    int  size()  const noexcept { return count; }
    bool empty() const noexcept { return count == 0; }
    Move operator[](int i) const noexcept { return moves[i]; }
    Move*       begin() noexcept       { return moves; }
    Move*       end()   noexcept       { return moves + count; }
    const Move* begin() const noexcept { return moves; }
    const Move* end()   const noexcept { return moves + count; }
};

class Board {
public:
    // ---- Core state (public for eval/search access) ----
    Bitboard pieces[NCOLORS][PIECE_TYPE_NB]; // [color][piece_type]
    Bitboard occupancy[NCOLORS];
    Bitboard all_occ;
    Piece    board_sq[SQUARE_NB];  // mailbox

    Color    side_to_move;
    int      fullmove_number;
    int      ply;            // half-moves from root (for repetition check)

    Key    hash;
    Key    pawn_key;
    Key    minor_key;
    Key    nonpawn_key[NCOLORS];
    Square ep_sq;
    int    castling_rights;
    int    halfmove_clock;
    int    plies_from_null;

    Square king_sq[NCOLORS];
    Bitboard checkers;  // pieces giving check to side_to_move (updated by make_move/set_fen)

    // Growable undo history (8.6.10a, replacing the fixed 2048-entry array +
    // release clamp): a `position ... moves` list of any length is now simply
    // correct — no clamp, no bounded-state-damage compromise, no assert/clamp
    // contradiction. HISTORY_RESERVE keeps the common case allocation-free;
    // push_back beyond it amortizes. Capacity is preserved across copies and
    // set_fen, so per-search Board copies never reallocate mid-search.
    static constexpr size_t HISTORY_RESERVE = 2048;
    std::vector<UndoInfo> history;

    // ---- Interface ----
    Board();
    Board(const Board& other);
    Board& operator=(const Board& other);
    Board(Board&& other) noexcept = default;
    Board& operator=(Board&& other) noexcept = default;

    void set_fen(const std::string& fen);
    // C++23 std::expected error channel (8.6.10 / 8.6.2c): on failure the
    // board is untouched and the error() explains why. Replaces the previous
    // bool + out-param pair — the error can no longer be silently ignored,
    // and there is no half-initialized "success bool true, error set" state
    // to misuse.
    [[nodiscard]] std::expected<void, std::string>
    try_set_fen(const std::string& fen, bool validate_legal_position = false);
    [[nodiscard]] std::string get_fen() const;

    // Full internal-consistency check (Phase 8.8, infra audit 4.5). Verifies
    // mailbox <-> bitboards, occupancy unions, king counts/squares, castling-
    // rights plausibility, EP-square plausibility, incremental Zobrist/pawn/
    // minor/non-pawn keys against a from-scratch recompute, cached `checkers`,
    // and the clock. Returns true when consistent; on failure writes the first
    // offending invariant to stderr. Cheap enough that test_invariants calls it
    // after every move across millions of random-walk states; not on any
    // release hot path.
    bool assert_ok() const;

    void make_move(Move m);
    void unmake_move(Move m);
    void make_null_move();
    void unmake_null_move();

    // ---- 8.7.1(c) speed telemetry -----------------------------------------
    // Counted unconditionally (8.6.6 DiagCounters class: NIL cost, no Diag
    // branch), one increment at the top of each query. They live HERE rather
    // than at the ~11 search call sites so every caller is captured, including
    // movegen's own gives_check use. `mutable` because both queries are const;
    // race-free because a Board is per-searcher (Searcher::search takes it BY
    // VALUE). Snapshotted into DiagCounters at search teardown, since
    // board_ptr_ is nulled before print_diag() runs.
    //
    // What they answer: 8.7.5 needs SEE calls-per-node (how much SEE work is
    // recomputed), 8.7.3 needs the full-gives_check rate (the check-hint
    // candidate). Both are currently unmeasurable.
    mutable int64_t diag_see_ge_calls = 0;
    mutable int64_t diag_gives_check_calls = 0;

    bool is_in_check() const;
    [[nodiscard]] bool gives_check(Move m) const;
    Bitboard check_squares(PieceType pt, Color us) const;
    bool is_square_attacked(Square sq, Color by) const;
    bool is_attacked_by(Square sq, Bitboard occ, Color by) const;
    [[nodiscard]] Bitboard attackers_to(Square sq, Bitboard occ) const;
    [[nodiscard]] Bitboard attackers_to(Square sq, Bitboard occ, Color by) const;

    // Pseudo-legal move generation

    // Legal move generation (no is_legal() filter needed)
    void gen_legal(MoveList& moves) const;
    void gen_legal_captures(MoveList& moves) const;
    void gen_legal_quiets(MoveList& moves) const;
    void gen_quiet_checks(MoveList& moves) const;  // quiet moves that give check

    [[nodiscard]] bool is_legal(Move m) const;

    [[nodiscard]] bool is_draw() const;
    [[nodiscard]] bool is_draw(int search_ply) const;
    // Rule-50 with correct mate precedence: at clock >= 100 the game is a
    // draw only if the side to move is not in check, or is in check but has
    // a legal evasion. In check with no legal move = CHECKMATE, not a draw
    // (SF semantics; infra audit 4.1). Generates legal moves only in the
    // rare in-check-at-the-boundary case.
    [[nodiscard]] bool rule50_draw() const;
    // True if at least one LEGAL en-passant capture of the pawn behind `ep`
    // exists for `capturer` (full king-safety simulation per candidate).
    // Position identity for repetition depends on legal moves, so ep_sq is
    // set/hashed only when this holds (infra audit 4.5; SF semantics).
    bool ep_capture_legal(Square ep, Color capturer) const;
    [[nodiscard]] bool ep_capture_legal_from(Square from, Square ep, Color capturer) const;
    // SEE pin support (8.2): absolute pins for both colors against the given
    // occupancy. pinner_of[sq] is valid only where pinned[] has the bit.
    void see_pins(Bitboard occ, Bitboard pinned[NCOLORS], Square pinner_of[SQUARE_NB]) const;
    [[nodiscard]] bool is_repetition(int search_ply) const;
    bool is_insufficient_material() const;

    [[nodiscard]] bool has_non_pawn_material(Color c) const;
    int  see(Move m) const;
    [[nodiscard]] bool see_ge(Move m, int threshold) const;

    // Castling permission mask per square (AND onto castling_rights when piece moves from/to sq)
    static constexpr int CASTLING_MASK[SQUARE_NB] = {
        13, 15, 15, 15, 12, 15, 15, 14, // rank 1
        15,15,15,15,15,15,15,15,
        15,15,15,15,15,15,15,15,
        15,15,15,15,15,15,15,15,
        15,15,15,15,15,15,15,15,
        15,15,15,15,15,15,15,15,
        15,15,15,15,15,15,15,15,
         7, 15, 15, 15,  3, 15, 15, 11  // rank 8
    };

private:
    void put_piece(Color c, PieceType pt, Square sq);
    void remove_piece(Square sq);
    void move_piece(Square from, Square to);
    Key  compute_hash() const;
};
