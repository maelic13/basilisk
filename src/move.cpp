#include "move.h"

std::string move_to_uci(Move m) {
    if (m == MOVE_NONE) return "0000";

    const char files[] = "abcdefgh";
    const char ranks[] = "12345678";

    Square from = from_sq(m);
    Square to   = to_sq(m);

    std::string s;
    s += files[file_of(from)];
    s += ranks[rank_of(from)];
    s += files[file_of(to)];
    s += ranks[rank_of(to)];

    if (move_type(m) == PROMOTION) {
        const char promos[] = "nbrq";
        s += promos[int(promo_type(m)) - int(KNIGHT)];
    }
    return s;
}
