// ordlog.h -- move-ordering data (21/10/2026; only with -DTRIUMV_ORDLOG, never in a release).
// Question: where does the ordering put the moves that refute a node, and what do the late refuters have that the
// ordering score does not see? Three records, written by threads.cpp (13_search.inc) in the working directory:
//  - ordnode_<thread>.bin: a sampled node (depth ORDLOG_MIN_DEPTH..ORDLOG_MAX_DEPTH, not the root, not a singular
//    verification node) that failed high, or failed low (flag 1024: then a refuting move found by the oracle is one
//    the real search missed). After the normal search, EVERY legal move of the node is searched again with a null
//    window at beta and at the node's depth - 1, without reductions (the oracle): the label of a move is "refutes"
//    (value >= beta), whatever the ordering and the reductions did.
//  - ordmove_<thread>.bin: one record per legal move of an oracle node: its place in the queue order (the queue is
//    replayed), its score and stage, the features below, the oracle label.
//  - ordcut_<thread>.bin: a light record of a sampled fail-high at any depth (where the real cutoff came).
// The oracle searches change the tree of the data build only (hash table and histories see them).
#pragma once
#include <cstdint>
#include <cstdio>

#ifndef ORDLOG_EVERY
#define ORDLOG_EVERY 48          // one oracle node every ORDLOG_EVERY eligible fail-high nodes
#endif
#ifndef ORDLOG_LOW_EVERY
#define ORDLOG_LOW_EVERY 96      // one oracle node every ORDLOG_LOW_EVERY eligible fail-low nodes
#endif
#ifndef ORDLOG_CUT_EVERY
#define ORDLOG_CUT_EVERY 16      // one light cutoff record every ORDLOG_CUT_EVERY fail-high nodes
#endif
#ifndef ORDLOG_MIN_DEPTH
#define ORDLOG_MIN_DEPTH 3
#endif
#ifndef ORDLOG_MAX_DEPTH
#define ORDLOG_MAX_DEPTH 8
#endif

static thread_local int ol_in_probe = 0;   // no sampling and no killer/counter updates inside the oracle

#pragma pack(push, 1)
struct OrdNodeRec {
  uint32_t node_id;              // per thread, joins ordmove records
  int16_t depth, ply, expert, pieces, n_legal, cut_index, cut_order;
  uint16_t flags;                // 1 pv, 2 cut, 4 all, 8 improving, 16 tt_pv, 32 in check, 64 hash move, 128 tactical
                                 // hash, 256 opp_worse, 512 hash move is the real cutter, 1024 failed low
  int32_t beta, eval_beta, static_beta;   // eval and static eval relative to beta
  int16_t c2ex;                  // Consilium Plebis experts (white | black << 4), -1 without TRIUMV_C2
  int8_t cut_step;               // queue step of the real cutter (QueueStep), -1 hash move, -3 failed low
  int8_t last_kind, last_victim; // opponent's last move: piece kind (P..K, -1 none/null), victim kind (-1 none)
  int8_t hanging_n, hanging_max; // our pieces en prise (attacked by a lower piece, or attacked and undefended), max kind
  uint32_t probe_nodes;
};
struct OrdMoveRec {
  uint32_t node_id;
  int32_t move;
  int16_t order;                 // 1-based place in the queue order (0 = the queue does not hand it out)
  int8_t step;                   // QueueStep when handed out, -1 hash move
  int32_t score;                 // queue score (tactical or quiet scale; 0 for the hash move)
  int8_t refutes;                // oracle label: value >= beta at depth - 1, null window, no reduction
  int16_t margin;                // oracle value - beta, clipped to +-3000
  uint32_t nodes;                // nodes of the oracle search of this move
  // features, all before the move
  int8_t pt, victim, promo, castle, ep, gives, see0, see_win;
  int8_t from_att, from_lesser, from_hang;   // origin attacked; by a lower piece; our piece en prise there
  int8_t to_att_min, to_def;                 // lowest enemy attacker of the target after the move (kind + 1, 0 none);
                                             // target defended by another of our pieces
  int8_t n_targets, max_target, n_good_targets, fork;   // enemy non-king pieces attacked from the target after the move
  int8_t kdist, katt;            // Chebyshev distance target - enemy king; squares of the enemy king ring attacked
  int8_t recapture, hits_last, escapes_last, same_piece;   // relations to the last two moves
  int8_t passed, rank_to, advance;   // pawn passed after the move; relative rank of the target; rank gain
  int8_t killer, counter;        // the move cut at this ply before / cut after this opponent move before
  int8_t c2_switch;              // the capture changes a Consilium Plebis expert (-1 without TRIUMV_C2)
  int8_t fate, red;              // what the real search did: 0 never handed out (after the cutoff, or move-count
                                 // pruning), 1 skipped by a pruning rule, 2 searched reduced, 3 searched without
                                 // reduction; red = plies of reduction of its first search (negative = extension)
  int32_t h_main, h_pawn, h_c1, h_c2, h_far, h_low, h_capt;   // statistics read by the ordering
};
struct OrdCutRec {
  int16_t depth, ply, expert, cut_index;
  int8_t step, flags;            // flags: 1 pv, 2 cut node, 4 in check, 8 tactical cutter, 16 hash cutter
};
#pragma pack(pop)

static unsigned ordlog_counters[1024 * 16];
static inline bool ordlog_pick(int tid) {
  return tid >= 0 && tid < 1024 && !ol_in_probe && ++ordlog_counters[tid * 16] % ORDLOG_EVERY == 0;
}
static inline bool ordlog_low_pick(int tid) {
  return tid >= 0 && tid < 1024 && !ol_in_probe && ++ordlog_counters[tid * 16 + 2] % ORDLOG_LOW_EVERY == 0;
}
static inline bool ordlog_cut_pick(int tid) {
  return tid >= 0 && tid < 1024 && !ol_in_probe && ++ordlog_counters[tid * 16 + 1] % ORDLOG_CUT_EVERY == 0;
}

template <class R>
static inline void ordlog_write(int tid, const char *stem, int slot, const R *r, size_t n) {
  static FILE *files[3][1024];
  if (tid < 0 || tid >= 1024 || !n)
    return;
  FILE *&fp = files[slot][tid];
  if (!fp) {
    char name[64];
    std::snprintf(name, sizeof name, "%s_%d.bin", stem, tid);
    fp = std::fopen(name, "ab");
    if (!fp)
      return;
  }
  std::fwrite(r, sizeof(R), n, fp);
  std::fflush(fp);
}

// Killer and counter memories of the data build (classic ideas the search does not use; here only as features).
static thread_local int ol_killer[256];
static thread_local int ol_counter[12][64];
