#include <algorithm>
#include <atomic>
#include <cctype>
#include <format>
#include <sstream>
#include <string>
#include <vector>

#include "wac.h"
#include "wac_epd.h"
#include "search.h"
#include "tt.h"
#include "uci_output.h"

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
    for (const char* chunk : WAC_EPD) {
        std::istringstream epd(chunk);
        std::string line;
        while (std::getline(epd, line)) {
            WacPosition pos;
            if (parse_epd_line(line, pos)) positions.push_back(std::move(pos));
        }
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

    if (type_of(board.piece_on(from_sq(mv))) != piece) return false;

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

bool parse_wac_request(const std::string& args, WacRequest& request,
                       std::string& error) {
    request = WacRequest{};
    error.clear();

    std::istringstream input(args);
    std::string first;
    if (!(input >> first))
        return true;

    auto parse_positive = [&](const std::string& token, int64_t& value) {
        try {
            size_t used = 0;
            value = std::stoll(token, &used);
            return used == token.size() && value > 0;
        } catch (const std::exception&) {
            return false;
        }
    };

    std::string budget_token;
    if (first == "nodes" || first == "depthpv") {
        request.mode = first == "nodes" ? WacMode::Nodes : WacMode::DepthPv;
        if (!(input >> budget_token)) {
            error = "wac " + first + " requires a positive budget";
            return false;
        }
    } else {
        request.mode = WacMode::Depth;
        budget_token = first;
    }

    if (!parse_positive(budget_token, request.budget)) {
        error = "invalid wac budget: " + budget_token;
        return false;
    }
    std::string extra;
    if (input >> extra) {
        error = "unexpected wac argument: " + extra;
        return false;
    }
    if (request.mode != WacMode::Nodes && request.budget > MAX_SEARCH_DEPTH) {
        error = "wac depth exceeds " + std::to_string(MAX_SEARCH_DEPTH);
        return false;
    }
    return true;
}

namespace {

struct WacIteration {
    int depth = 0;
    int seldepth = 0;
    int64_t nodes = 0;
    int64_t time_ms = 0;
    std::string pv1;
};

bool parse_iteration(const std::string& line, WacIteration& out) {
    if (!line.starts_with("info depth ")
        || line.find(" lowerbound ") != std::string::npos
        || line.find(" upperbound ") != std::string::npos)
        return false;

    std::istringstream input(line);
    std::string token;
    input >> token; // info
    while (input >> token) {
        if (token == "depth") input >> out.depth;
        else if (token == "seldepth") input >> out.seldepth;
        else if (token == "nodes") input >> out.nodes;
        else if (token == "time") input >> out.time_ms;
        else if (token == "pv") {
            std::string move;
            if (input >> move)
                out.pv1 = move;
            break;
        }
    }
    return out.depth > 0 && !out.pv1.empty();
}

Move legal_uci_move(const Board& board, const std::string& uci) {
    MoveList legal;
    board.gen_legal(legal);
    for (int i = 0; i < legal.size(); ++i)
        if (move_to_uci(legal[i]) == uci)
            return legal[i];
    return MOVE_NONE;
}

const char* mode_name(WacMode mode) {
    switch (mode) {
        case WacMode::Depth: return "depth";
        case WacMode::Nodes: return "nodes";
        case WacMode::DepthPv: return "depthpv";
    }
    return "unknown";
}

std::string joined_best_moves(const std::vector<std::string>& moves) {
    std::string joined;
    for (const std::string& move : moves) {
        if (!joined.empty()) joined += ',';
        joined += move;
    }
    return joined;
}

} // namespace

// ---------------------------------------------------------------------------
// wac [depth] | wac nodes N | wac depthpv N
// ---------------------------------------------------------------------------

void run_wac(const WacRequest& request, const SearchParams& params) {
    std::atomic_bool stop{false};
    TranspositionTable tt(16);
    std::vector<WacIteration>* active_iterations = nullptr;
    SearchThreadPool search_pool(tt, stop, [&](const std::string& line) {
        WacIteration iteration;
        if (active_iterations && parse_iteration(line, iteration))
            active_iterations->push_back(iteration);
    });
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
        if (request.mode == WacMode::Nodes)
            limits.nodes = request.budget;
        else
            limits.depth = static_cast<int>(request.budget);
        limits.params = params;   // 5.4.4: honour UCI-set search parameters
        std::vector<WacIteration> iterations;
        active_iterations = &iterations;
        stop.store(false, std::memory_order_release);
        SearchResult r = search_pool.search(board, limits, 1);
        active_iterations = nullptr;

        total_nodes += r.nodes;
        total_ms    += r.elapsed_ms;

        const bool position_solved =
            wac_move_matches_any(board, r.bestmove, pos.best_moves);
        if (position_solved) {
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

        int first = 0;
        int stable = 0;
        if (request.mode == WacMode::DepthPv) {
            for (const WacIteration& iteration : iterations) {
                const Move iteration_move = legal_uci_move(board, iteration.pv1);
                const bool iteration_solved =
                    wac_move_matches_any(board, iteration_move, pos.best_moves);
                if (iteration_solved && first == 0)
                    first = iteration.depth;

                uci_write_line(std::format("wac pv id {} depth {} bestmove {} solved {}",
                                           pos.id, iteration.depth, iteration.pv1,
                                           iteration_solved ? 1 : 0));
            }
            for (size_t j = 0; j < iterations.size(); ++j) {
                bool remains_solved = true;
                for (size_t k = j; k < iterations.size(); ++k)
                    remains_solved &= wac_move_matches_any(
                        board, legal_uci_move(board, iterations[k].pv1),
                        pos.best_moves);
                if (remains_solved) {
                    stable = iterations[j].depth;
                    break;
                }
            }
        }

        const WacIteration last = iterations.empty() ? WacIteration{} : iterations.back();
        std::string line = std::format(
            "wac record id {} mode {} budget {} depth {} seldepth {} nodes {} time_ms {} "
            "search_nodes {} search_time_ms {} bestmove {} bm {} solved {}",
            pos.id, mode_name(request.mode), request.budget, last.depth, last.seldepth,
            last.nodes, last.time_ms, r.nodes, r.elapsed_ms, move_to_uci(r.bestmove),
            joined_best_moves(pos.best_moves), position_solved ? 1 : 0);
        if (request.mode == WacMode::DepthPv)
            line += std::format(" first {} stable {}", first, stable);
        uci_write_line(line);
    }

    uci_write_line(std::format(
        "\n=========================\n"
        "WAC solved      : {}/{} in {} {}\n"
        "Nodes searched  : {}\n"
        "Total time (ms) : {}",
        solved, positions.size(), mode_name(request.mode), request.budget,
        total_nodes, total_ms));
    if (!failed.empty()) {
        std::string joined;
        for (const std::string& f : failed) {
            if (!joined.empty()) joined += ", ";
            joined += f;
        }
        uci_write_line("Failed: " + joined);
    }
}
