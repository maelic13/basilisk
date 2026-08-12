/*
  Basilisk/Stockfish HCE oracle — bridge implementation (PLAN 5.1).

  The single translation unit that sees Basilisk headers. It links against
  Basilisk's unmodified source; nothing in src/ is changed for this
  experiment, which is what makes the measured evaluator the released one.

  This file is distributed under the GNU General Public License version 3
  or (at your option) any later version.
*/

#include "basilisk_bridge.h"

#include <mutex>

#include "attacks.h"
#include "bitboard.h"
#include "board.h"
#include "eval.h"
#include "eval_params.h"
#include "types.h"
#include "zobrist.h"

namespace BasiliskBridge {
namespace {

// One evaluator and one scratch board per Stockfish worker. The evaluator owns
// a 16384-entry pawn cache; sharing it across threads would either need a lock
// on the hot path or race. Per-thread state keeps the cache behaviour close to
// how Basilisk itself runs, which matters because the oracle's throughput is
// part of what the experiment reports.
thread_local Evaluator tls_evaluator;
thread_local Board     tls_board;

// The conformance reference gets its own evaluator so that the fast path
// cannot poison the cache the reference reads from. Sharing one would let a
// wrong pawn_key store under a wrong slot and then be confirmed by a probe
// that collided with it — the test would agree with the bug.
thread_local Evaluator tls_reference_evaluator;

} // namespace

bool initialize(std::string& error) {
    static std::once_flag once;
    static bool           ok = false;

    std::call_once(once, [&] {
        // Exactly the sequence Basilisk's own main() runs before searching.
        init_bitboards();
        init_attacks();
        Zobrist::init();
        init_eval_tables(g_eval_params);
        ok = true;
    });

    if (!ok)
        error = "Basilisk global table initialization failed";
    return ok;
}

std::int32_t evaluate(const std::uint64_t* pieces,
                      std::uint8_t stm,
                      std::uint8_t castling,
                      std::uint8_t rule50) {
    Board& b = tls_board;

    // Every field below is public state that Basilisk's own evaluator reads.
    // Board::put_piece() is private and is deliberately not used: making it
    // reachable would mean editing src/ for an experiment, and the value of
    // this oracle depends on the evaluator under test being the released one,
    // byte for byte.
    //
    // The set of fields written here is not a guess. evaluate() and everything
    // it calls read exactly: pieces, occupancy, all_occ, board_sq, king_sq,
    // side_to_move, halfmove_clock, castling_rights and pawn_key. ep_sq,
    // checkers, hash, minor_key, nonpawn_key, ply and the move history are
    // never consulted, so they are left alone rather than filled with
    // plausible-looking values that could mask a mistake. evaluate_fen() below
    // is the reference that holds this claim to account.

    for (int sq = 0; sq < SQUARE_NB; ++sq)
        b.board_sq[sq] = NO_PIECE;

    Key pawn_key = 0ULL;

    for (int c = 0; c < NCOLORS; ++c) {
        Bitboard side_occ = 0ULL;

        b.pieces[c][NO_PIECE_TYPE] = 0ULL;

        for (int pt = PAWN; pt <= KING; ++pt) {
            const Bitboard bb = pieces[c * 6 + pt - 1];
            b.pieces[c][pt] = bb;
            side_occ |= bb;

            for (Bitboard scan = bb; scan;) {
                const int sq = pop_lsb(scan);
                b.board_sq[sq] = make_piece(static_cast<Color>(c),
                                            static_cast<PieceType>(pt));
                if (pt == PAWN)
                    pawn_key ^= Zobrist::PieceKeys[c][PAWN][sq];
            }
        }

        b.occupancy[c] = side_occ;

        // Exactly one king per side is required: several evaluation terms
        // index king_sq unconditionally, and the pawn-cache key would collide
        // across genuinely different positions if the board were malformed.
        const Bitboard kings = b.pieces[c][KING];
        if (!kings || more_than_one(kings))
            return EvalError;
        b.king_sq[c] = static_cast<Square>(lsb(kings));
    }

    b.all_occ         = b.occupancy[WHITE] | b.occupancy[BLACK];
    b.pawn_key        = pawn_key;
    b.side_to_move    = static_cast<Color>(stm);
    b.castling_rights = static_cast<int>(castling);
    b.halfmove_clock  = static_cast<int>(rule50);

    return tls_evaluator.evaluate(b);
}

std::int32_t evaluate_fen(const std::string& fen) {
    Board b;
    if (auto r = b.try_set_fen(fen); !r)
        return EvalError;
    return tls_reference_evaluator.evaluate(b);
}

} // namespace BasiliskBridge
