#pragma once

// NNUE evaluation (Phase 9): loader + evaluator for the .mnn network format
// defined in the shared net_trainer repo (docs/mnn_format.md is the
// cross-language contract; net_trainer/nnue/inference.py is the reference).
//
// Bring-up implementation: full-board recompute per call. Correct and
// test-vector-exact, but ~an order of magnitude too slow for search — the
// incremental-accumulator + SIMD version replaces the evaluate() internals
// in Step 9.3 without changing this interface.
//
// The network is OFF by default (UCI: UseNNUE) and absent unless a net was
// baked in at configure time (-DBASILISK_NNUE_FILE=...) or loaded at runtime
// (UCI: EvalFile). With no net / UseNNUE=false the engine is byte-identical
// to the HCE build.

#include <cstdint>
#include <string>

class Board;

namespace nnue {

// Load a .mnn from a file path. Returns false (and keeps any previous net)
// on failure.
bool load_file(const std::string& path);

// Load the compile-time embedded net, if the build carries one.
bool load_embedded();

// A net is loaded (embedded or file).
bool available();

// UseNNUE toggle. Enabling without an available net is a no-op (returns false).
bool set_enabled(bool on);
bool enabled();

// Centipawns from the side to move's point of view. Requires enabled().
int evaluate(const Board& b);

// One-line human-readable state for `info string` replies.
std::string status();

}  // namespace nnue
