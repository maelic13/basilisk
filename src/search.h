#pragma once

// The search's public surface for the engine, bench, WAC and tests: the value
// types of one `go`, the root table and result sanitation, one search worker
// (Searcher) and the Lazy SMP pool. The modules behind it are
//
//   search_types.h        limits, results, score bands
//   search_root.h         root move table, legality and info-line helpers
//   search_diagnostics.h  Diag counters and the decision trace
//   move_picker.h         staged move ordering
//   search_worker.h       the per-thread worker and its state
//   search_history.cpp    history and correction policy
//   search_thread_pool.h  the persistent thread pool

#include "search_types.h"
#include "search_root.h"
#include "search_worker.h"
#include "search_thread_pool.h"
