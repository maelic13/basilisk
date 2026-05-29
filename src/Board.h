#pragma once

#include "types.h"
#include "bitboard.h"
#include "move.h"
#include "attacks.h"
#include "zobrist.h"
#include <memory>
#include <vector>
#include <string>

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

    static constexpr int MAX_HISTORY = 1024;
    std::unique_ptr<UndoInfo[]> history;
    int history_size;

    // ---- Interface ----
    Board();
    Board(const Board& other);
    Board& operator=(const Board& other);
    Board(Board&& other) noexcept = default;
    Board& operator=(Board&& other) noexcept = default;

    void set_fen(const std::string& fen);
    bool try_set_fen(const std::string& fen, std::string* error = nullptr,
                     bool validate_legal_position = false);
    std::string get_fen() const;

    void make_move(Move m);
    void unmake_move(Move m);
    void make_null_move();
    void unmake_null_move();

    bool is_in_check() const;
    bool gives_check(Move m) const;
    Bitboard check_squares(PieceType pt, Color us) const;
    bool is_square_attacked(Square sq, Color by) const;
    bool is_attacked_by(Square sq, Bitboard occ, Color by) const;
    Bitboard attackers_to(Square sq, Bitboard occ) const;
    Bitboard attackers_to(Square sq, Bitboard occ, Color by) const;

    // Pseudo-legal move generation
    void gen_pseudo_legal(std::vector<Move>& moves) const;
    void gen_pseudo_legal_captures(std::vector<Move>& moves) const;

    // Legal move generation (no is_legal() filter needed)
    void gen_legal(MoveList& moves) const;
    void gen_legal_captures(MoveList& moves) const;
    void gen_legal_quiets(MoveList& moves) const;
    void gen_quiet_checks(MoveList& moves) const;  // quiet moves that give check

    bool is_legal(Move m) const;

    bool is_draw() const;
    bool is_draw(int search_ply) const;
    bool is_repetition(int search_ply) const;
    bool is_insufficient_material() const;

    bool has_non_pawn_material(Color c) const;
    int  see(Move m) const;
    bool see_ge(Move m, int threshold) const;

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
