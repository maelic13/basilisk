#include <algorithm>
#include <atomic>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>

#include "wac.h"
#include "wac_epd.h"
#include "search.h"
#include "tt.h"
#include "UciOutput.h"

// ---------------------------------------------------------------------------
// EPD parsing (mirrors Rarog's src/wac.rs parse_epd_line)
// ---------------------------------------------------------------------------

static bool parse_epd_line(const std::string& raw, WacPosition& out) {
    // Trim.
    size_t b = raw.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return false;
    size_t e = raw.find_last_not_of(" \t\r\n");
    const std::string line = raw.substr(b, e - b + 1);

    const size_t bm = line.find(" bm ");
    if (bm == std::string::npos) return false;

    // The board description is exactly 4 fields; some entries carry extra EPD
    // opcodes (e.g. WAC.274's "am Rd6;") between them and `bm` — drop those.
    std::istringstream fields(line.substr(0, bm));
    std::string f1, f2, f3, f4;
    if (!(fields >> f1 >> f2 >> f3 >> f4)) return false;
    out.fen = f1 + " " + f2 + " " + f3 + " " + f4 + " 0 1";

    const std::string rest = line.substr(bm + 4);
    const size_t semi = rest.find(';');
    if (semi == std::string::npos) return false;

    out.best_moves.clear();
    std::istringstream moves(rest.substr(0, semi));
    std::string san;
    while (moves >> san) out.best_moves.push_back(san);

    out.id.clear();
    const size_t idpos = rest.find("id \"");
    if (idpos != std::string::npos) {
        const size_t start = idpos + 4;
        const size_t end   = rest.find('"', start);
        if (end != std::string::npos) out.id = rest.substr(start, end - start);
    }
    return true;
}

std::vector<WacPosition> wac_positions() {
    std::vector<WacPosition> positions;
    positions.reserve(300);
    std::istringstream epd(WAC_EPD);
    std::string line;
    while (std::getline(epd, line)) {
        WacPosition pos;
        if (parse_epd_line(line, pos)) positions.push_back(std::move(pos));
    }
    return positions;
}

// ---------------------------------------------------------------------------
// SAN matching (mirrors Rarog's san_matches)
// ---------------------------------------------------------------------------

bool wac_san_matches(const Board& board, Move mv, const std::string& raw_san) {
    std::string san = raw_san;
    while (!san.empty() && (san.back() == '+' || san.back() == '#'))
        san.pop_back();

    const MoveType mt = move_type(mv);

    // Castling.
    if (san == "O-O" || san == "0-0")
        return mt == CASTLING && file_of(to_sq(mv)) == FILE_G;
    if (san == "O-O-O" || san == "0-0-0")
        return mt == CASTLING && file_of(to_sq(mv)) == FILE_C;
    if (mt == CASTLING) return false;

    // Promotion suffix (=Q etc.).
    char promo = 0;
    const size_t eq = san.find('=');
    if (eq != std::string::npos) {
        if (eq + 1 < san.size()) promo = char(std::toupper(san[eq + 1]));
        san = san.substr(0, eq);
    }
    if (promo) {
        if (mt != PROMOTION) return false;
        static constexpr char PROMO_CHARS[] = {'N', 'B', 'R', 'Q'};
        if (PROMO_CHARS[promo_type(mv) - KNIGHT] != promo) return false;
    } else if (mt == PROMOTION) {
        return false;
    }

    // Leading piece letter (none = pawn).
    PieceType piece = PAWN;
    if (!san.empty()) {
        switch (san[0]) {
            case 'N': piece = KNIGHT; break;
            case 'B': piece = BISHOP; break;
            case 'R': piece = ROOK;   break;
            case 'Q': piece = QUEEN;  break;
            case 'K': piece = KING;   break;
            default:  break;
        }
    }
    if (piece != PAWN) san.erase(0, 1);
    san.erase(std::remove(san.begin(), san.end(), 'x'), san.end());
    if (san.size() < 2) return false;

    if (type_of(board.board_sq[from_sq(mv)]) != piece) return false;

    // Destination square.
    const Square to = to_sq(mv);
    if (san[san.size() - 2] != char('a' + file_of(to))) return false;
    if (san[san.size() - 1] != char('1' + rank_of(to))) return false;

    // Leftover leading chars are disambiguation hints on the origin square
    // (for pawns, the file of a capturing pawn, e.g. "exd5").
    const Square from = from_sq(mv);
    for (size_t i = 0; i + 2 < san.size(); ++i) {
        const char hint = san[i];
        if (std::isdigit(static_cast<unsigned char>(hint))) {
            if (hint != char('1' + rank_of(from))) return false;
        } else {
            if (hint != char('a' + file_of(from))) return false;
        }
    }
    return true;
}

bool wac_move_matches_any(const Board& board, Move mv,
                          const std::vector<std::string>& best_moves) {
    for (const std::string& san : best_moves)
        if (wac_san_matches(board, mv, san)) return true;
    return false;
}

// ---------------------------------------------------------------------------
// wac [depth] — engine command (mirrors Rarog's run_wac; output format kept
// line-compatible so the two engines' reports diff cleanly)
// ---------------------------------------------------------------------------

void run_wac(int depth) {
    std::atomic_bool stop{false};
    TranspositionTable tt(16);
    SearchThreadPool search_pool(tt, stop);
    search_pool.ensure_threads(1);

    const std::vector<WacPosition> positions = wac_positions();
    int solved = 0;
    std::vector<std::string> failed;
    int64_t total_nodes = 0;
    int64_t total_ms    = 0;

    uci_write_line("");
    for (size_t i = 0; i < positions.size(); ++i) {
        const WacPosition& pos = positions[i];
        Board board;
        if (!board.try_set_fen(pos.fen)) {
            uci_write_line("info string wac " + pos.id + " failed to parse");
            return;
        }

        // Clean identical state per position: deterministic, order-independent.
        tt.clear();
        search_pool.clear();

        SearchLimits limits;
        limits.depth = depth;
        stop.store(false, std::memory_order_release);
        SearchResult r = search_pool.search(board, limits, 1);

        total_nodes += r.nodes;
        total_ms    += r.elapsed_ms;

        if (wac_move_matches_any(board, r.bestmove, pos.best_moves)) {
            ++solved;
        } else {
            failed.push_back(pos.id + " (" + move_to_uci(r.bestmove)
                             + " != bm " + [&] {
                                   std::string all;
                                   for (const std::string& s : pos.best_moves) {
                                       if (!all.empty()) all += " ";
                                       all += s;
                                   }
                                   return all;
                               }() + ")");
        }

        std::ostringstream line;
        line << "wac " << (i + 1) << "/" << positions.size()
             << "  " << pos.id
             << "  nodes " << r.nodes
             << "  time " << r.elapsed_ms << "ms"
             << "  (solved " << solved << ")";
        uci_write_line(line.str());
    }

    std::ostringstream summary;
    summary << "\n=========================\n"
            << "WAC solved      : " << solved << "/" << positions.size()
            << " at depth " << depth << "\n"
            << "Nodes searched  : " << total_nodes << "\n"
            << "Total time (ms) : " << total_ms;
    uci_write_line(summary.str());
    if (!failed.empty()) {
        std::string joined;
        for (const std::string& f : failed) {
            if (!joined.empty()) joined += ", ";
            joined += f;
        }
        uci_write_line("Failed: " + joined);
    }
}
