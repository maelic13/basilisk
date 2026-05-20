#include <iostream>
#include <string>

#include "Constants.h"
#include "Engine.h"

Engine::Engine(std::atomic_bool& go_, std::atomic_bool& quit_,
               Parameters& params_, std::mutex& mutex_, std::condition_variable& cv)
    : go(go_), quit(quit_), parameters(params_), mutex(mutex_), conditionVariable(cv)
    , tt(64)
    , searcher_(std::make_unique<Searcher>(tt, go,
          [](const std::string& info) {
              std::cout << info << '\n';
              std::cout.flush();
          }))
{}

void Engine::start() {
    int current_hash_mb = 64;  // matches initial tt(64) in constructor

    while (true) {
        std::unique_lock lock(mutex);
        conditionVariable.wait(lock, [&] { return go.load() || quit.load(); });

        if (quit.load()) break;
        if (!go.load())  continue;

        // Build search limits from parameters
        SearchLimits limits;
        limits.depth     = parameters.depth;
        limits.movetime  = parameters.moveTime;
        limits.wtime     = parameters.whiteTime;
        limits.btime     = parameters.blackTime;
        limits.winc      = parameters.whiteIncrement;
        limits.binc      = parameters.blackIncrement;
        limits.movestogo = parameters.movestogo;
        limits.nodes     = parameters.nodes;
        limits.overhead  = parameters.moveOverhead;
        limits.ponder    = parameters.ponder;
        limits.infinite  = (parameters.depth == infiniteDepth && parameters.moveTime == 0
                            && parameters.whiteTime == 0 && parameters.blackTime == 0
                            && !parameters.ponder);

        int desired_hash_mb = parameters.hash_mb;
        bool  do_clear_hash = parameters.clear_hash;
        Board board_copy    = parameters.board;
        bool  is_new_game   = parameters.new_game;
        parameters.new_game   = false;
        parameters.clear_hash = false;
        lock.unlock();

        // Resize TT if hash size changed
        if (desired_hash_mb != current_hash_mb) {
            tt.resize(desired_hash_mb);
            current_hash_mb = desired_hash_mb;
        }

        if (is_new_game || do_clear_hash) {
            tt.clear();
            searcher_->clear();
        }

        SearchResult result = searcher_->search(board_copy, limits);

        go = false;

        if (result.bestmove != MOVE_NONE) {
            std::string out = "bestmove " + move_to_uci(result.bestmove);
            if (result.pondermove != MOVE_NONE) {
                Board ponder_board = board_copy;
                ponder_board.make_move(result.bestmove);
                // Validate ponder: check piece exists at from_sq and belongs to side to move
                Square pfrom = from_sq(result.pondermove);
                Piece  pp    = ponder_board.board_sq[pfrom];
                if (pp != NO_PIECE
                    && color_of(pp) == ponder_board.side_to_move
                    && ponder_board.is_legal(result.pondermove))
                    out += " ponder " + move_to_uci(result.pondermove);
            }
            std::cout << out << '\n';
        } else {
            std::cout << "bestmove 0000\n";
        }
        std::cout.flush();
    }
}


