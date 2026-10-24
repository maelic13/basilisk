#include "search_diagnostics.h"
#include <algorithm>
#include <format>
#include <string>

#if defined(BASILISK_TUNE) || defined(BASILISK_DIAGNOSTIC)
void DecisionTrace::start(bool enabled) {
    count_ = 0;
    overflow_ = false;
    enabled_ = enabled;
    if (enabled_)
        records_ = std::make_unique<std::array<TraceRecord, CAPACITY>>();
    else
        records_.reset();
}

void DecisionTrace::record(TraceEvent event, int ply, int depth, Move move,
                           int alpha, int beta, int estimated_score,
                           int improving, int correction, int history,
                           int move_count, int reduction, int margin, int result,
                           int cutoff_count, int lmr_depth, int skip_quiets,
                           TraceFamily family) {
    if (!enabled_ || ply < 1 || ply > 2)
        return;
    if (count_ >= CAPACITY) {
        overflow_ = true;
        return;
    }
    TraceRecord& r = (*records_)[count_];
    r.event = event;
    r.move = move;
    r.sequence = static_cast<int>(count_);
    r.ply = ply;
    r.depth = depth;
    r.alpha = alpha;
    r.beta = beta;
    r.estimated_score = estimated_score;
    r.improving = improving;
    r.correction = correction;
    r.history = history;
    r.move_count = move_count;
    r.reduction = reduction;
    r.cutoff_count = cutoff_count;
    r.margin = margin;
    r.result = result;
    r.lmr_depth = lmr_depth;
    r.skip_quiets = skip_quiets;
    r.family = family;
    ++count_;
}

void DecisionTrace::print(const InfoCallback& info, const std::vector<Move>& root_moves) const {
    if (!info || !enabled_)
        return;
    auto event_name = [](TraceEvent event) {
        switch (event) {
            case TraceEvent::CheckExtension:       return "check_extension";
            case TraceEvent::TtCutoff:             return "tt_cutoff";
            case TraceEvent::RfpPrune:             return "rfp_prune";
            case TraceEvent::RazorPrune:           return "razor_prune";
            case TraceEvent::NullCutoff:           return "null_cutoff";
            case TraceEvent::ProbcutCutoff:        return "probcut_cutoff";
            case TraceEvent::IirReduction:         return "iir_reduction";
            case TraceEvent::FutilityPrune:        return "futility_prune";
            case TraceEvent::LmpPrune:             return "lmp_prune";
            case TraceEvent::HistoryPrune:         return "history_prune";
            case TraceEvent::QuietSeePrune:        return "quiet_see_prune";
            case TraceEvent::CaptureFutilityPrune: return "capture_futility_prune";
            case TraceEvent::CaptureSeePrune:      return "capture_see_prune";
            case TraceEvent::SingularExtension:    return "singular_extension";
            case TraceEvent::SingularMulticut:     return "singular_multicut";
            case TraceEvent::SingularNegative:     return "singular_negative";
            case TraceEvent::LmrReduction:         return "lmr_reduction";
            case TraceEvent::LmrResearch:          return "lmr_research";
            case TraceEvent::BetaCutoff:           return "beta_cutoff";
            case TraceEvent::QsTtCutoff:           return "qs_tt_cutoff";
            case TraceEvent::QsStandPatCutoff:     return "qs_stand_pat_cutoff";
            case TraceEvent::QsDeltaPrune:         return "qs_delta_prune";
            case TraceEvent::QsFutilityPrune:      return "qs_futility_prune";
            case TraceEvent::QsSeePrune:           return "qs_see_prune";
            case TraceEvent::QsLatePrune:          return "qs_late_prune";
            case TraceEvent::QsBetaCutoff:         return "qs_beta_cutoff";
            case TraceEvent::SkipQuiets:           return "skip_quiets";
        }
        return "unknown";
    };
    auto family_name = [](TraceFamily family) {
        switch (family) {
            case TraceFamily::None:            return "none";
            case TraceFamily::Razor:           return "razor";
            case TraceFamily::Rfp:             return "rfp";
            case TraceFamily::SkipQuiets:      return "skip_quiets";
            case TraceFamily::ContHist:        return "cont_hist";
            case TraceFamily::QuietFutility:   return "quiet_futility";
            case TraceFamily::QuietSee:        return "quiet_see";
            case TraceFamily::CaptureFutility: return "capture_futility";
            case TraceFamily::CaptureSee:      return "capture_see";
        }
        return "unknown";
    };

    info("info string trace begin version=2 plies=1-2 root="
             + move_to_uci(root_moves.front())
             + " cutoff_count=" + (kTraceCutoffCountAvailable ? "available" : "unavailable")
             + " value_none=" + std::to_string(VALUE_NONE)
             + " bool_unknown=-1 capacity=" + std::to_string(CAPACITY));
    for (size_t i = 0; i < count_; ++i) {
        const TraceRecord& r = (*records_)[i];
        info("info string trace record seq=" + std::to_string(r.sequence)
               + " event=" + event_name(r.event)
               + " ply=" + std::to_string(r.ply)
               + " depth=" + std::to_string(r.depth)
               + " move=" + (r.move == MOVE_NONE ? std::string("none") : move_to_uci(r.move))
               + " alpha=" + std::to_string(r.alpha)
               + " beta=" + std::to_string(r.beta)
               + " estimated_score=" + std::to_string(r.estimated_score)
               + " improving=" + std::to_string(r.improving)
               + " correction=" + std::to_string(r.correction)
               + " history=" + std::to_string(r.history)
               + " move_count=" + std::to_string(r.move_count)
               + " reduction=" + std::to_string(r.reduction)
               + " cutoff_count=" + std::to_string(r.cutoff_count)
               + " window_alpha=" + std::to_string(r.alpha)
               + " window_beta=" + std::to_string(r.beta)
               + " margin=" + std::to_string(r.margin)
               + " result=" + std::to_string(r.result)
               + " lmr_depth=" + std::to_string(r.lmr_depth)
               + " skip_quiets=" + std::to_string(r.skip_quiets)
               + " family=" + family_name(r.family));
    }
    info("info string trace end status="
             + std::string(overflow_ ? "overflow" : "ok")
             + " records=" + std::to_string(count_));
}
#endif

// End-of-search diagnostic dump (UCI `Diag`). One line per family; shares are
// of interior (negamax) nodes. Never a gate: these size candidates and verify
// that a mechanism fires where it should.
void print_search_diag(const DiagCounters& d, const Evaluator& evaluator,
                       const InfoCallback& info) {
    if (!info) return;
    auto pct = [](int64_t a, int64_t b) {
        return b > 0 ? 100.0 * double(a) / double(b) : 0.0;
    };
    // Each line is built whole by std::format: a fixed buffer once truncated
    // the tail field silently, corrupting the kv mirror.
    std::string buf;
    auto emit = [&](const std::string& text) { info(std::string("info string diag ") + text); };
    buf = std::format("nodes interior {} qsearch {} | in_check {} ({:.2f}%) check_ext {} tt_pv {} ({:.2f}%)",
        d.interior_nodes, d.qs_nodes,
        d.in_check_nodes, pct(d.in_check_nodes, d.interior_nodes),
        d.check_exts,
        d.tt_pv_nodes, pct(d.tt_pv_nodes, d.interior_nodes));
    emit(buf);
    buf = std::format("tt probes {} hits {} ({:.2f}%) cutoffs {}",
        d.tt_probes, d.tt_hits, pct(d.tt_hits, d.tt_probes),
        d.tt_cutoffs);
    emit(buf);
    buf = std::format("prune rfp {} razor {} null {}/{} probcut {}/{} fut {} lmp {} hist {} see {}",
        d.rfp_cuts, d.razor_cuts,
        d.null_cuts, d.null_tries,
        d.probcut_cuts, d.probcut_tries,
        d.fut_prunes, d.lmp_prunes,
        d.hist_prunes, d.see_prunes);
    emit(buf);
    buf = std::format("lmr applied {} researched {} ({:.2f}%) | hist updates cutoff {} reward {} | qs evasion {}",
        d.lmr_applied, d.lmr_researched,
        pct(d.lmr_researched, d.lmr_applied),
        d.hist_cutoff_updates, d.hist_reward_updates,
        d.qs_evasion_nodes);
    emit(buf);
    // The differential harness's lines: read against the oracle's tree
    // shape, not in isolation.
    buf = std::format("order fail_highs {} first {} ({:.2f}%) mean_idx {:.3f} | src tt {} goodcap {} quiet {} badcap {}",
        d.fail_highs, d.fail_high_first,
        pct(d.fail_high_first, d.fail_highs),
        d.fail_highs > 0 ? double(d.fail_high_index_sum) / double(d.fail_highs) : 0.0,
        d.cutoff_src_tt, d.cutoff_src_good_tactical,
        d.cutoff_src_quiet, d.cutoff_src_bad_tactical);
    emit(buf);
    buf = std::format("lmrgate eligible {} applied {} ({:.2f}%) mean_r {:.3f} clamp0 {}",
        d.lmr_eligible, d.lmr_applied,
        pct(d.lmr_applied, d.lmr_eligible),
        d.lmr_applied > 0 ? double(d.lmr_reduction_plies) / double(d.lmr_applied) : 0.0,
        d.lmr_clamped_zero);
    emit(buf);
    buf = std::format("lmrblock depth {} searched {} in_check {} movetype {} gives_check {}",
        d.lmr_blocked_depth, d.lmr_blocked_searched,
        d.lmr_blocked_in_check, d.lmr_blocked_movetype,
        d.lmr_blocked_gives_check);
    emit(buf);
    buf = std::format("moveloop tested {} conthist {} capfut {} quietsee {} capsee {} quietfut {} "
        "| skip_quiets nodes {} | lmr floor {} extended {} deeper {} shallower {} "
        "| hindsight up {} down {}",
        d.moveloop_tested, d.cont_hist_pruned, d.capture_futility_pruned,
        d.quiet_see_pruned, d.capture_see_pruned, d.fut_prunes,
        d.skip_quiets_nodes, d.lmr_floor_hits, d.lmr_extended,
        d.lmr_research_deeper, d.lmr_research_shallower,
        d.hindsight_up, d.hindsight_down);
    emit(buf);
    // Machine-readable mirror. The lines above are shaped for a human reading
    // one search; the harness aggregates over a 107-position suite and must not
    // have to reverse-engineer prose, percentages or floats to do it. Integers
    // only, canonical names, one token per counter — derived ratios are the
    // consumer's job, since summing a percentage across positions is wrong.
    {
        buf = std::format("kv fail_highs={} fail_high_first={} fail_high_index_sum={} "
            "cutoff_src_tt={} cutoff_src_goodcap={} cutoff_src_quiet={} "
            "cutoff_src_badcap={}",
            d.fail_highs, d.fail_high_first,
            d.fail_high_index_sum,
            d.cutoff_src_tt, d.cutoff_src_good_tactical,
            d.cutoff_src_quiet, d.cutoff_src_bad_tactical);
        emit(buf);
        buf = std::format("kv lmr_eligible={} lmr_applied={} lmr_researched={} "
            "lmr_reduction_plies={} lmr_clamped_zero={} "
            "lmr_blocked_depth={} lmr_blocked_searched={} "
            "lmr_blocked_in_check={} lmr_blocked_movetype={} "
            "lmr_blocked_gives_check={} lmr_clamped_high={}",
            d.lmr_eligible, d.lmr_applied,
            d.lmr_researched, d.lmr_reduction_plies,
            d.lmr_clamped_zero,
            d.lmr_blocked_depth, d.lmr_blocked_searched,
            d.lmr_blocked_in_check, d.lmr_blocked_movetype,
            d.lmr_blocked_gives_check, d.lmr_clamped_high);
        emit(buf);
        buf = std::format("kv interior_nodes={} qs_nodes={} tt_probes={} tt_hits={} "
            "tt_cutoffs={} in_check_nodes={} check_exts={} tt_pv_nodes={}",
            d.interior_nodes, d.qs_nodes,
            d.tt_probes, d.tt_hits, d.tt_cutoffs,
            d.in_check_nodes, d.check_exts,
            d.tt_pv_nodes);
        emit(buf);
        buf = std::format("kv rfp_cuts={} razor_cuts={} null_tries={} null_cuts={} "
            "probcut_tries={} probcut_cuts={} fut_prunes={} lmp_prunes={} "
            "hist_prunes={} see_prunes={}",
            d.rfp_cuts, d.razor_cuts,
            d.null_tries, d.null_cuts,
            d.probcut_tries, d.probcut_cuts,
            d.fut_prunes, d.lmp_prunes,
            d.hist_prunes, d.see_prunes);
        emit(buf);
        buf = std::format("kv hist_prune_tested={} hist_below_half={} "
            "hist_below_quarter={} hist_below_eighth={} "
            "qs_evasion_nodes={} hist_cutoff_updates={} hist_reward_updates={}",
            d.hist_prune_tested, d.hist_below_half,
            d.hist_below_quarter, d.hist_below_eighth,
            d.qs_evasion_nodes, d.hist_cutoff_updates,
            d.hist_reward_updates);
        emit(buf);
        buf = std::format("kv tt_stores={} tt_stores_same_key={} "
            "asp_windows={} asp_fail_low={} asp_fail_high={} "
            "asp_researches={} asp_giveup={}",
            d.tt_stores, d.tt_stores_same_key,
            d.asp_windows, d.asp_fail_low,
            d.asp_fail_high, d.asp_researches,
            d.asp_giveup);
        emit(buf);
        buf = std::format("kv sing_fired={} sing_double={} sing_in_check={} "
            "sing_triple={} sing_ttbeta={}",
            d.sing_fired, d.sing_double,
            d.sing_in_check, d.sing_triple,
            d.sing_ttbeta);
        emit(buf);
        // The selectivity core's counters (zero under the legacy kernel).
        buf = std::format("kv rfp_cuts_d1_3={} rfp_cuts_d4_7={} rfp_cuts_d8p={} "
            "razor_cuts_d1_3={} razor_cuts_d4_7={} razor_cuts_d8p={} "
            "hindsight_up={} hindsight_down={} skip_quiets_nodes={} "
            "corr_cont2_updates={} corr_cont4_updates={} "
            "tt_cutoff_quiet_bonus={} tt_cutoff_graph_refused={}",
            d.rfp_cuts_d1_3, d.rfp_cuts_d4_7, d.rfp_cuts_d8p,
            d.razor_cuts_d1_3, d.razor_cuts_d4_7, d.razor_cuts_d8p,
            d.hindsight_up, d.hindsight_down, d.skip_quiets_nodes,
            d.corr_cont2_updates, d.corr_cont4_updates,
            d.tt_cutoff_quiet_bonus, d.tt_cutoff_graph_refused);
        emit(buf);
        buf = std::format("kv lmr_floor_hits={} lmr_extended={} "
            "lmr_research_deeper={} lmr_research_shallower={} "
            "moveloop_tested={} cont_hist_pruned={} capture_futility_pruned={} "
            "quiet_see_pruned={} capture_see_pruned={}",
            d.lmr_floor_hits, d.lmr_extended,
            d.lmr_research_deeper, d.lmr_research_shallower,
            d.moveloop_tested, d.cont_hist_pruned, d.capture_futility_pruned,
            d.quiet_see_pruned, d.capture_see_pruned);
        emit(buf);
    }
    // Speed telemetry: the evaluation rate, the pawn-cache hit rate, the
    // gives_check rate and SEE calls per node.
    {
        const int64_t total_nodes = d.interior_nodes + d.qs_nodes;
        buf = std::format("speed eval {} ({:.2f}%/node) pawncache {}/{} ({:.2f}% hit) "
            "gives_check {} ({:.2f}%/node) see_ge {} ({:.3f}/node)",
            evaluator.eval_calls, pct(evaluator.eval_calls, total_nodes),
            evaluator.pawn_hits, evaluator.pawn_probes,
            pct(evaluator.pawn_hits, evaluator.pawn_probes),
            d.gives_check_calls, pct(d.gives_check_calls, total_nodes),
            d.see_ge_calls,
            total_nodes > 0 ? double(d.see_ge_calls) / double(total_nodes) : 0.0);
        emit(buf);
        buf = std::format("kv eval_calls={} pawn_probes={} pawn_hits={} "
            "gives_check_calls={} see_ge_calls={}",
            evaluator.eval_calls,
            evaluator.pawn_probes, evaluator.pawn_hits,
            d.gives_check_calls, d.see_ge_calls);
        emit(buf);
    }
#ifdef BASILISK_TUNE
    {
        const auto& e = evaluator.endgame_occurrence;
        buf = std::format("endgames <=7men {} ({:.3f}% eval, {:.3f}% node)",
            e.classified,
            pct(e.classified, evaluator.eval_calls),
            pct(e.classified, d.interior_nodes + d.qs_nodes));
        emit(buf);
        buf = std::format("kv eg_classified={} eg_krpkr={} eg_krpkb={} eg_kpsk={} "
            "eg_kpk={} eg_krkp={} eg_kbpsk={} eg_kpkp={}",
            e.classified, e.krpkr, e.krpkb,
            e.kpsk, e.kpk, e.krkp,
            e.kbpsk, e.kpkp);
        emit(buf);
        buf = std::format("kv eg_kqkp={} eg_kbpkb={} eg_kbppkb={} eg_krkn={} "
            "eg_krkb={} eg_kbpkn={} eg_knnkp={} eg_knnk={}",
            e.kqkp, e.kbpkb, e.kbppkb,
            e.krkn, e.krkb, e.kbpkn,
            e.knnkp, e.knnk);
        emit(buf);
        buf = std::format("kv eg_kqkr={} eg_kqkrps={} eg_krppkrp={} eg_kxk={} eg_kbnk={}",
            e.kqkr, e.kqkrps, e.krppkrp,
            e.kxk, e.kbnk);
        emit(buf);
    }
#endif
    {
        std::string b;
        b = std::format("aspiration windows {} fail_low {} fail_high {} researches {} giveup {}",
            d.asp_windows, d.asp_fail_low,
            d.asp_fail_high, d.asp_researches,
            d.asp_giveup);
        emit(b);
    }

    {
        std::string b;
        b = std::format("singular fired {} double {} in_check {} triple {} ttbeta {} ({:.2f}% of fired)",
            d.sing_fired, d.sing_double,
            d.sing_in_check, d.sing_triple,
            d.sing_ttbeta,
            d.sing_fired ? 100.0 * double(d.sing_in_check) / double(d.sing_fired) : 0.0);
        emit(b);
    }

    if (evaluator.lazy_fires > 0) {
        buf = std::format("lazy fires {} sign_flips {} crossings {} absdelta mean {:.1f} max {}",
            evaluator.lazy_fires, evaluator.lazy_sign_flips,
            evaluator.lazy_margin_crossings,
            double(evaluator.lazy_absdelta_sum) / double(evaluator.lazy_fires),
            evaluator.lazy_absdelta_max);
        emit(buf);
    }
}

// The pool section. Printed by the main thread after the join (the helpers
// finish after its own search and per-thread lines), so it appends rather
// than replacing anything.
void print_pool_diag(const DiagCounters& main, const DiagCounters& pool, int thread_count,
                     const std::vector<int>& completed_depths, const InfoCallback& info) {
    auto pct = [](int64_t a, int64_t b) {
        return b > 0 ? 100.0 * double(a) / double(b) : 0.0;
    };
    std::string buf;
    auto emit = [&](const std::string& text) { info(std::string("info string diag ") + text); };

    const int64_t pool_nodes = pool.interior_nodes + pool.qs_nodes;
    const int64_t main_nodes = main.interior_nodes + main.qs_nodes;
    buf = std::format("pool threads {} nodes {} (main {} = {:.1f}%) | main tt {}/{} ({:.2f}% hit) "
        "pool tt {}/{} ({:.2f}% hit)",
        thread_count, pool_nodes, main_nodes,
        pct(main_nodes, pool_nodes),
        main.tt_hits, main.tt_probes,
        pct(main.tt_hits, main.tt_probes),
        pool.tt_hits, pool.tt_probes,
        pct(pool.tt_hits, pool.tt_probes));
    emit(buf);

    // Same-key share: how much of the pool's TT traffic updates an entry for a
    // position the table already holds, versus evicting a different one. Read
    // it as a share, never as an absolute.
    buf = std::format("pool tt_stores {} same_key {} ({:.2f}%) | main stores {} same_key {} ({:.2f}%)",
        pool.tt_stores, pool.tt_stores_same_key,
        pct(pool.tt_stores_same_key, pool.tt_stores),
        main.tt_stores, main.tt_stores_same_key,
        pct(main.tt_stores_same_key, main.tt_stores));
    emit(buf);

    // Per-thread completed depth. ⚠ A DIAGNOSTIC, never a verdict: its
    // rep-to-rep spread at fixed time is ~±2 iterations, the same size as any
    // effect worth measuring. Likewise divide aspiration/re-search counts by
    // the thread count before comparing across thread counts.
    std::string depths = "pool depths";
    for (size_t i = 0; i < completed_depths.size(); ++i) {
        depths += (i == 0 ? " main=" : " t" + std::to_string(i) + "=");
        depths += std::to_string(completed_depths[i]);
    }
    depths += "  (diagnostic only: +/-2 iterations rep-to-rep at fixed time)";
    emit(depths.c_str());
}
