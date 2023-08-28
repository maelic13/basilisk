#ifndef BASILISK_PIECE_TYPE_H
#define BASILISK_PIECE_TYPE_H

enum class PieceType {
    NONE,
    PAWN,
    KNIGHT,
    BISHOP,
    ROOK,
    QUEEN,
    KING,
};

enum class Color {
    NONE,
    WHITE,
    BLACK,
};

class Piece {
public:
    PieceType type;
    Color color;

    Piece() {
        type = PieceType::NONE;
        color = Color::NONE;
    }

    Piece(PieceType type, Color color) : type(type), color(color) {};
};

#endif //BASILISK_PIECE_TYPE_H
