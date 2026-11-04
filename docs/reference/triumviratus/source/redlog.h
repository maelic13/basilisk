// redlog.h -- data collection for block 2 of 8.1 (only with -DTRIUMV_REDLOG; never in a release).
// Every REDLOG_EVERY-th reduced move writes one record: the features of the node and the move, the reduction the
// current formula gives it and the outcome of its searches. Files: redlog3_<thread>.bin in the working directory.
#pragma once
#include <cstdint>
#include <cstdio>

#ifndef REDLOG_EVERY
#define REDLOG_EVERY 8
#endif

#ifndef REDLOG_PROBE
#define REDLOG_PROBE 4   // one in REDLOG_PROBE recorded moves that fail low reduced is also searched at full depth
#endif

#pragma pack(push, 1)
struct RedlogRec {   // version 2 (18/10/2026): outcome of the searches appended
  int16_t depth, moves, move_no, expert;
  int32_t stat_score;
  int16_t gap, cutoff_cnt;
  int32_t r_entry, r_final, signal;
  int16_t window_term, ply;
  uint16_t flags;
  uint8_t outcome;     // 1 reduced > alpha, 2 re-searched, 4 final > alpha, 8 probed, 16 probe > alpha
  int8_t d_red, d_full;
  int16_t hash_gap;    // hash value - static eval (0 when unknown)
  uint8_t hash_known;
  // version 3 (22/10/2026): the own score of a sorted quiet move of the own ordering form (removed the same day, now
  // always 0 and not valid; kept for the file format) and the raw move number.
  int32_t own_score;
  int16_t raw_moves;
  uint8_t own_ok;
};
#pragma pack(pop)

static unsigned redlog_counters[1024 * 16];   // one cache line per thread
static inline bool redlog_pick(int tid) {
  return tid >= 0 && tid < 1024 && ++redlog_counters[tid * 16] % REDLOG_EVERY == 0;
}
static inline bool redlog_probe(int tid) {
  return tid >= 0 && tid < 1024 && ++redlog_counters[tid * 16 + 1] % REDLOG_PROBE == 0;
}

static inline void redlog_write(int tid, int depth, int moves, int move_no, int expert, int stat_score, int gap,
                                int cutoff_cnt, int r_entry, int r_final, int signal, int window_term, int ply,
                                int flags, int outcome, int d_red, int d_full, int hash_gap, bool hash_known,
                                int own_score, int raw_moves, bool own_ok) {
  static FILE *files[1024];
  if (tid < 0 || tid >= 1024)
    return;
  FILE *&fp = files[tid];
  if (!fp) {
    char name[64];
    std::snprintf(name, sizeof name, "redlog3_%d.bin", tid);
    fp = std::fopen(name, "ab");
    if (!fp)
      return;
  }
  RedlogRec r{(int16_t)depth, (int16_t)moves, (int16_t)move_no, (int16_t)expert, stat_score, (int16_t)gap,
              (int16_t)cutoff_cnt, r_entry, r_final, signal, (int16_t)window_term, (int16_t)ply, (uint16_t)flags,
              (uint8_t)outcome, (int8_t)d_red, (int8_t)d_full, (int16_t)hash_gap, (uint8_t)hash_known,
              own_score, (int16_t)raw_moves, (uint8_t)own_ok};
  std::fwrite(&r, sizeof r, 1, fp);
}

// Re-search data (18/10/2026). Type 1: a reduced move that failed high, searched again with a null window at the full
// depth minus one, the full depth and one ply more. Type 2: the first move of a non-PV node, searched at the full depth
// minus two, minus one and the full depth. Values are relative to alpha; nodes are the cost of each probe. Files:
// redres_<thread>.bin. The probes change the tree of the data build only.
#ifndef REDRES_EVERY
#define REDRES_EVERY 128   // type 2 only; type 1 takes every move redlog_pick chose
#endif

#pragma pack(push, 1)
struct RedresRec {
  uint8_t type, done;   // done: bit k set when probe k ran to the end, bits 3-5 the order of the probes (0-5)
  int8_t d, nd;         // depth of the reduced search, full depth of the move
  int16_t depth, moves, move_no, expert, ply;
  int32_t stat_score, r, signal;
  int16_t hash_gap;
  uint8_t hash_known;
  uint16_t flags;
  int16_t m_best, m_alpha;   // reduced value minus best and minus alpha (type 1)
  int16_t v[3];              // probe values minus alpha
  uint32_t nodes[3];
};
#pragma pack(pop)

static inline bool redres_pick(int tid) {
  return tid >= 0 && tid < 1024 && ++redlog_counters[tid * 16 + 2] % REDRES_EVERY == 0;
}

static inline int16_t redres_clip(int v) { return (int16_t)(v < -32000 ? -32000 : v > 32000 ? 32000 : v); }

static inline void redres_write(int tid, const RedresRec &r) {
  static FILE *files[1024];
  if (tid < 0 || tid >= 1024)
    return;
  FILE *&fp = files[tid];
  if (!fp) {
    char name[64];
    std::snprintf(name, sizeof name, "redres_%d.bin", tid);
    fp = std::fopen(name, "ab");
    if (!fp)
      return;
  }
  std::fwrite(&r, sizeof r, 1, fp);
}
