#pragma once

#include "types.h"
#include "bitboard.h"
#include "move.h"
#include "attacks.h"
#include "zobrist.h"
#include <vector>
#include <string>

// Per-move undo information
struct UndoInfo {
    Key    hash;
    Square ep_sq;
    int    castling;
    int    halfmove;
    Piece  captured;
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
    Square ep_sq;
    int    castling_rights;
    int    halfmove_clock;

    Square king_sq[NCOLORS];

    std::vector<UndoInfo> history;

    // ---- Interface ----
    Board();
    void set_fen(const std::string& fen);
    std::string get_fen() const;

    void make_move(Move m);
    void unmake_move(Move m);
    void make_null_move();
    void unmake_null_move();

    bool is_in_check() const;
    bool is_square_attacked(Square sq, Color by) const;
    Bitboard attackers_to(Square sq, Bitboard occ) const;
    Bitboard attackers_to(Square sq, Bitboard occ, Color by) const;

    // Pseudo-legal move generation
    void gen_pseudo_legal(std::vector<Move>& moves) const;
    void gen_pseudo_legal_captures(std::vector<Move>& moves) const;

    bool is_legal(Move m) const;

    bool is_draw() const;
    bool is_insufficient_material() const;

    bool has_non_pawn_material(Color c) const;
    int  see(Move m) const;

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
