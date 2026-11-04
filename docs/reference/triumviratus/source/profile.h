#ifndef TRIUMV_PROFILE_H
#define TRIUMV_PROFILE_H

// Compile-time profiling instrumentation. Define TRIUMV_PROFILE (e.g. /D TRIUMV_PROFILE
// in the build, or `make profile`) to enable per-phase cycle counters
// (eval / movegen / make / tt / score). Default builds (no define) compile every
// PROF_GUARD to a no-op => zero overhead, byte-identical to the release engine.
#ifdef TRIUMV_PROFILE
  // __rdtsc: <intrin.h> is MSVC; on GCC/clang it lives in <x86intrin.h>. Without this
  // branch `make profile` did not compile on Linux and the per-stage breakdown had
  // never been measured on the rig.
  #ifdef _MSC_VER
    #include <intrin.h>
  #else
    #include <x86intrin.h>
  #endif
  extern unsigned long long prof_eval, prof_mg, prof_make, prof_tt, prof_score;
  // Breakdown of the NNUE forward (prof_eval). It serves to answer ONE question that
  // blocks the work on the sparsity of L1: how much does fc_0 really weigh? The audit archived
  // `ft_optimize` with the explicit condition "if fc_0 is below 5% of the time, closed",
  // and that measurement was never made.
  //   prof_ft     = feature transformer (accumulator + transform)
  //   prof_fc0    = the AffineTransformSparseInput 1024->16 alone
  //   prof_layers = all the rest (activations + fc_1 + fc_2)
  extern unsigned long long prof_ft, prof_fc0, prof_layers;
  // prof_catchup = `nn_dirty_catch_up` (07/10/2026, single board v2): threats and pawns computed late, before
  // the evaluation, undoing and redoing the pending moves on local copies. It is inside prof_eval but OUTSIDE prof_ft/fc0/layers. Until
  // 06/10/2026 it measured the replay of the moves on the network's mirror board.
  extern unsigned long long prof_catchup;
  // Histogram of the accesses to the rows of `threatWeights` (threat + PawnPair +
  // PassedPawns folded: 59808 + 4560 + 96 = 64464 rows). It serves to answer the
  // question that decides whether the LOCALITY PERMUTATION makes sense: are the accesses
  // concentrated on few features or spread over all of them?
  //   - concentrated => packing the hot ones into a contiguous region reduces the pages
  //     touched and the TLB pressure (110 MB of weights = 27,000 pages of 4 KB, and
  //     large pages are not available: no tester enables them)
  //   - flat         => there is nothing to pack, dead idea
  static constexpr int PROF_FEAT_N = 64464;
  extern unsigned long long prof_feat_hist[PROF_FEAT_N];
  // Same histogram for the HalfKA rows (`weights`): 22,528 rows of 2048 bytes = 46 MB,
  // the BIGGEST of the transformer and those on which the prefetch paid off most (+1.3%).
  // Their access distribution has never been looked at.
  static constexpr int PROF_PSQ_N = 22528;
  extern unsigned long long prof_psq_hist[PROF_PSQ_N];
  // CO-OCCURRENCE matrix between the hot rows: how many times two rows appear
  // in the SAME update. It is the target the frequency ordering does not see:
  // frequency puts the most used rows first, but what matters is how many
  // PAGES a single update touches. Two features that always appear together
  // should sit in the same 4 KB page (= 4 rows of 1024 bytes) even if
  // neither of the two is among the hottest in absolute terms.
  // 🔑 With the frequency permutation already active, the hot rows ARE the first ones:
  // collecting the pairs among the indices < PROF_COOC_N is enough.
  // 8192 hot rows cover ~95% of the accesses (4096 covered ~87%).
  // 16-bit counts WITH SATURATION: clustering needs the relative order of the
  // neighbours, not the exact value, and at 32 bits the matrix would be 268 MB instead of 134.
  static constexpr int PROF_COOC_N = 8192;
  extern unsigned short* prof_cooc;  // PROF_COOC_N^2, allocated at first use
  // Breakdown of the feature transformer (prof_ft), which the first round of measurements
  // revealed to be 39.3% of the wall time and 91.2% of the forward: the number one
  // bottleneck of the engine. Here we separate WHERE that time goes.
  //   prof_acc_inc     = incremental update of the accumulator (add/sub of columns)
  //   prof_acc_refresh = full refresh from the cache (when the incremental one is not enough)
  //   prof_ft_out      = clipped ReLU + pairwise multiplication producing the input of fc_0
  // EVENT counters, not cycle counters: how many times each path is taken.
  extern unsigned long long prof_acc_inc, prof_acc_refresh, prof_ft_out;
  extern unsigned long long prof_n_inc, prof_n_refresh, prof_n_eval;
  // prof_n_cols / prof_n_upd = weight columns added or subtracted per update.
  extern unsigned long long prof_n_cols, prof_n_upd;
  // Threat tuples generated vs DISCARDED by the filter (map<0 => pawn->pawn, excluded from the set).
  extern unsigned long long prof_n_thr_seen, prof_n_thr_dead;
  // Maximum filling of the IndexLists, against MaxActiveDimensions = 288.
  extern unsigned long long prof_max_active, prof_max_inc;
  // At REFRESH: active columns per block. The two pawn blocks depend only on the
  // pawn structure => they are the only cacheable ones between consecutive refreshes (which are
  // triggered by KING moves). The ratio pawn/(pawn+threat) is the ceiling of the gain.
  extern unsigned long long prof_cols_thr, prof_cols_pawn, prof_n_refresh_calls;
  // Columns per INCREMENTAL update, per block: it is the count that tells "longer
  // lists" from "more expensive columns" in the comparison with a reference engine (09/09/2026).
  extern unsigned long long prof_cols_psq_inc, prof_cols_thr_inc, prof_cols_pawn_inc;
  // --- SEARCH SUB-BUCKETS (09/09/2026) -----------------------------------------
  // The "other" bucket was worth 1,297 cycles/node on kiwipete, the second of the engine after
  // the accumulator, and it was completely blind: eval/movegen/make/tt/score were the
  // only guards. The reference engine on the same node spends 929 cycles in EVERYTHING that is not
  // eval, do_move and movegen. Without these counters one does not know what to attack.
  //   prof_mp      = MovePicker (mp_init + mp_next: stages, lazy generation, selection)
  //   prof_hist    = EVERY write into a history (td_update_history, both
  //                  overloads): main, capture, continuation, pawn, lowply. The tables
  //                  are huge and sparse (continuation_history alone is 590 KB), and
  //                  until 09/09 they were covered by no guard.
  //   prof_corr    = correction history: indices, reads, updates
  //   prof_gc      = gives-check / legality computed OUTSIDE make (LMP guard, SEE)
  extern unsigned long long prof_mp, prof_hist, prof_corr, prof_gc;
  //   prof_thr     = td_compute_threats: the threat masks for the ordering,
  //                  recomputed ~1.5 times per node going over all the enemy pieces
  //   prof_see     = SEE (td_see_ge / td_see_at_least), called per move in the pruning
  //   prof_isatk   = td_is_square_attacked (legality, checks, evasions)
  //   prof_rep     = repetition detection + cuckoo
  extern unsigned long long prof_thr, prof_see, prof_isatk, prof_rep;
  //   prof_idx_thr / prof_idx_pawn = generation of the INDICES in the incremental update,
  //   separated per block. It serves to answer a question the comparison with
  //   the reference engine opens: it has THREE blocks (Threats, HalfKA, PP_3Wide), we have FOUR
  //   (+ PassedPawns), and `passers()` rebuilds the set of passers BEFORE and AFTER at
  //   every pawn event. If it is expensive, it is a cost they do not pay at all.
  extern unsigned long long prof_idx_thr, prof_idx_pawn;
  extern unsigned long long prof_n_thr_calls, prof_n_see, prof_n_isatk;
  extern unsigned long long prof_n_mg;   // calls to move generation (SF: 0.52/node)
  // Refresh cache of the pawn blocks: how many times the key is already in the cache.
  // A low rate says it pays to enlarge it, a high one that the ceiling has been reached.
  extern unsigned long long prof_pawn_hit, prof_pawn_miss;
  // Ceiling of `update_accumulator_hybrid` (SF db98633b). HalfKAv2_hm::requires_refresh
  // is true for EVERY move of one's own king, but the indices of threat/PawnPair/PassedPawns
  // depend on OrientTBL[ksq], which has only two values and changes ONLY if the king
  // crosses the d/e file. Every refresh in which the orientation does NOT change is
  // rebuilding from scratch features that would have stayed valid.
  extern unsigned long long prof_refresh_same_orient, prof_refresh_cross_orient;
  extern unsigned long long prof_dead_pair[8][8];
  inline unsigned long long prof_now() { return __rdtsc(); }
  struct ProfGuard {
      unsigned long long& c; unsigned long long t;
      ProfGuard(unsigned long long& cc) : c(cc), t(__rdtsc()) {}
      ~ProfGuard() { c += __rdtsc() - t; }
  };
  #define PROF_GUARD(counter) ProfGuard _pg(counter)
#else
  #define PROF_GUARD(counter) ((void)0)
#endif

#endif // TRIUMV_PROFILE_H
