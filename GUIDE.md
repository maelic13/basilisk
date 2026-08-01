# Basilisk Development Workflow Guide

> **Audience: developer / maintainer.** This file and `PLAN.md` are the
> internal pair — phases, gates, evidence, what to run next. The **user-facing**
> documents are `README.md`, `CHANGELOG.md` and the GitHub release notes; never
> move phase numbers, step bookkeeping or roadmap items into those. Full rule:
> **PLAN §1, "Document audience"**.
>
> *(Renamed from `user_dev_guide.md` on 2026-07-29 — the old name read as if it
> were for users, which is exactly backwards.)*

Quick human-side companion to `PLAN.md`. The plan is authoritative for scope,
dependencies, evidence and gates; this guide is the at-a-glance answer to
*where are we, what is next, what do I run*.

**Checklist format:** `- [ ]` / `- [x]`, then the step number, then — if the box
is ticked — the **reason for closure** (`PASSED`, `REJECTED`, `DONE`, `CLOSED`,
`SKIPPED`, `MOVED`), then the description.

---

## ▶ Where we are

> **1.9.2 is release-ready** on `development`; Phase 9 is CLOSED. 1.9.1 remains
> the last public release only until the manual squash/publish. Single-thread
> behavior remains identical (bench 11,941,440).
>
> **9.12 CLOSED:** the formal 4T gate passed; Colosseum confirmed positive
> direction against 1.9.1 at 1T fast (+11 ±24), 4T fast (+14 ±34), and 4T
> `10+0.1` (+26 ±35), with no reported Basilisk incident in 5,800 games. The
> standalone NPS/scaling reruns and longer duplicates were explicitly waived
> by the user because the existing fingerprint, pooled-PGO, MT telemetry and
> deployment-game evidence already cover them. Full record: **PLAN §5 9.12**.
>
> **9.1–9.4 DONE, 9.5 implemented** (2026-07-29/30). Only the
> 600-game 4T null spent games. ⚠ **Re-run `./tools/setup_tools.ps1`** before
> any tune — the 9.1 tuner patch ships as a tracked overlay and `spsa.ps1`
> refuses to launch on a clone that does not carry it. ⚠ Carry forward: that
> null bounds 4T harness bias at ~±28 nElo, not the pre-registered ±5, so a 4T
> result inside ±10 Elo is not separable from placement bias until the long
> null runs.
>
> **🎉 9.4 PASSED: +30.42 ± 8.77 Elo @4T, 0 forfeits** — the phase's accepted
> MT strength result. It met the original 1.10.0 trigger, but the user chose
> **1.9.2** on 2026-08-01 because the final accepted production scope is
> focused. The estimate is SPRT-stopped/optimistic and belongs to the complete
> clock/helper bundle. 1T is still byte-identical.
>
> **9.5 CLOSED, no feature banked** (2026-07-31). The original bundle was
> negative; `b` (pool windows) washed at **+1.26 ± 5.28 Elo** after 6,320 games
> and was reverted; `a` v1 was invalid (it selected refuted moves), and corrected
> sparse `a` was user-stopped on a negative trend at **−6.98 ± 8.51 Elo** after
> 2,450 games. `c` stays out: its one repair attempt was conditional on the
> `(a)+(b)+(d)` gate passing. The accepted head remains **9.4 / Clock94**.
>
> **9.6 PASSED: +1.29% pooled-PGO NPS** (95% CI [−0.25%, +2.64%]),
> bench-identical. **9.7 removed helper-history blending. 9.8 rejected the
> TT-depth provenance gate.** 9.9 closed neutral and was removed; 9.10 is not
> applicable without rejected 9.5(c). **9.11's valid five-phase refit washed
> and was restored. **9.12 passed and Phase 9 is closed.**
>
> **Deployment is 1T AND 4T** (your decision 2026-07-28). 4T Elo counts exactly
> as much as 1T Elo — every MT gate runs at Threads=4 / Hash 256 / ≥10k games,
> and every release validates both conditions. Gauntlets and tournaments run in
> **Colosseum**; LittleBlitzer is out of scope (historical numbers stay in
> `CHANGELOG.md` as record). ⚠ Colosseum 1.0.2 hard-codes 2 s of timeout grace,
> so it gates strength/crashes/illegal moves but cannot replace the 20 ms
> fastchess clock-safety evidence; 9.4 already passed that tight gate with zero
> forfeits.
>
> After Phase 9: the NNUE runway (8.5.3/14/15/16) → rebase `nnue` once →
> **Phase 10 = baseline NNUE, ships as 2.0.0**.

### Branch sequence

```text
development: … → 1.9.1 → Phase 9 → ✅ 1.9.2 release-ready
             → publish → NNUE runway (8.5.3/14/15/16) → record handoff SHA
nnue:        frozen → rebase once onto the handoff SHA → Phase 10
```

Do not implement Phase-8.5 StateInfo/search changes independently on `nnue`.
Do not merge partial NNUE work back into `development`.

| Version | Content |
|---|---|
| 1.9.0 ✅ | Phases 8 + 8.5: correctness, infra/state, eval-independent search, NNUE prep |
| 1.9.1 ✅ | Phase 8.6 (hardening/CI/telemetry, neutral +0.17 ± 4.41) + Phase 8.7 (speed, **+4.34% NPS ≈ +8.69 ± 6.63 Elo**) |
| 1.9.2 **READY** | Phase 9 — focused accepted SMP safety/scaling and speed work; 9.4 measured +30.42 ± 8.77 Elo @4T with 0 forfeits (SPRT-stopped/bundle caveat), and the 1T/4T boundary campaign showed no reversal. User selected a patch; manual publish remains |
| 2.0.0 | Phase 10: accepted embedded baseline NNUE via `net_trainer` |
| 2.x | Phase 11 final 1T search + tune · Phase 12 MT scaling beyond Phase 9 · Phase 13 NNUE architecture/data ladder |

---

## ✅ CLOSED — Phase 9 (PLAN §5)

Order below is execution order. `9.1`/`9.2` are interchangeable; everything
else depends on what precedes it, except `9.6`, which has no dependency at all
and can be pulled into any gate wait.

- [x] **9.1 DONE** (2026-07-29) SPSA harness repair + doctrine — **no games, no
      engine code.** Verify any time with `python tools/verify_spsa_schedule.py`
      (30 checks). What changed, from the operator's side:
      - **Re-run `./tools/setup_tools.ps1` after pulling.** The tuner clone is
        gitignored, so our patched `spsa.py`/`main.py` now live tracked in
        `tools/weather-factory-overlay/` and are copied into the clone by setup.
        `spsa.ps1` hash-checks them at setup *and* launch and refuses to run on
        a stale clone — a stale clone silently restores the ~8×-too-fast decay.
      - **`-REnd` replaces hand-picking `a`** (default `0.0031`). The whole
        schedule is back-solved from the horizon: `A = 0.1N`, `c = N^gamma`,
        `a = r_end·(A+N)^alpha`. Changing `-Iterations` can no longer silently
        change end behaviour.
      - **`-Iterations` below 5,000 now throws** (gate 11) unless you pass
        `-AllowShortRun`.
      - **The run stops itself** at its horizon, and the horizon is frozen at
        first launch: on `-Resume`, `-Iterations`/`-REnd` are ignored and both
        the script and the tuner say so loudly. Continuing past `N` means a
        fresh run, not a longer one.
      - **Nothing is truncated on resume any more:** the log appends (a fresh
        setup rotates the old one into `tuner/archive_*`) and every iteration's
        whole parameter vector is appended to **`tuner/trajectory.csv`** —
        that file is what a tail-mean bake should read.
      - Progress lines now carry iteration/target/percent/ETA.
      *Nothing was re-tuned: every accepted bake still stands on its SPRT, and
      the repair buys future tunes (first user: 11.7).*
- [x] **9.2 DONE** (2026-07-30, null accepted short — see the caveat) — multi-thread
      SPRT harness. No engine code. `sprt.ps1` now takes `-Threads` (and
      `-ThreadsA`/`-ThreadsB` for a scaling run); concurrency is threads-aware
      (**T1→14, T2→7, T4→3, T8→1** on this box, oversubscription throws); Hash
      defaults to **64 × that side's threads**; **`-use-affinity` is dropped
      whenever threads > 1** (fastchess pins one core per *game* regardless of
      Threads — Rarog measured −100 Elo of pure starvation that way); time
      forfeits are counted on every run and flagged as run-invalidating at MT.
      The 1T path is unchanged (pinned, Hash 64, conc 14) — smoke-tested.
      **Null run 2026-07-30:** 600 games @4T — Elo +4.63 ± 18.24, nElo +7.07 ±
      27.80, **0 time forfeits**, mechanics confirmed under load. ⚠ **You
      accepted this short**, so 4T harness bias is bounded at ~±28 nElo, not the
      pre-registered ±5 — a 4T result inside ±10 Elo is therefore not separable
      from placement bias on this evidence alone (matters most for 9.5).
      Measured throughput **1,155 games/h at 4T**: ±5 needs ~18,000 games
      (~16 h); `-Games 10000` reaches only ±6.8 and would FAIL against the
      default tolerance 5. The long null, if you want it later:
      ```powershell
      ./tools/sprt.ps1 -EngineA <head.exe> -EngineB <head.exe> `
          -NameA Self -NameB Self2 -Mode calibrate -Threads 4 -Games 18000
      ```
- [x] **9.3 DONE** (2026-07-30) — thread-count safety + node-counter batching +
      MT telemetry. **bench 11,941,440 identical, CTest 12/12.**
      - Cap: **flat `1024`, as Stockfish does it** (`Option(1, 1, 1024)`) —
        your decision 2026-07-30; choosing a thread count is the operator's
        job. The genuine fix is that the cap is now defined **once**, so the
        advertisement and the pool's real limit cannot drift apart (they were
        computed independently in two files). ⚠ The "we advertise 1024 and
        accept it" framing was overstated — the old formula was already 1024 on
        any host under 256 logical CPUs, and SF accepts the same.
      - Batching: **neutral at 1T and 4T, +12.8% at 16 threads.** It removes a
        Phase-12 scaling ceiling, not a 1T/4T speed win — do not quote it as Elo.
      - 🐛 Found on the way: **`bench` could not measure MT NPS at all** —
        `run_bench()` documented a threads argument the command parser dropped.
        Fixed (`bench [depth] [repeats] [threads]`; default 1, fingerprint
        untouched). The README's "bench honours the Threads option" was wrong
        and is corrected.
      - **MT baseline archived → [analysis/mt_baseline_9.3.md](analysis/mt_baseline_9.3.md)**:
        main 25.6% of pool nodes, main TT hit 39.60% vs pool 40.68%, same-key
        store share 33.90%, depths 16/15/15/16. 9.4/9.5/9.10 are read against it.
- [x] **9.4 PASSED** (2026-07-30) — **+30.42 ± 8.77 Elo @4T** (nElo +47.97 ±
      13.76, LOS 100%, H1 @ 2,450 games), **0 time forfeits**. Gated `[-3,0]`
      as a correctness step expected to be neutral; it is the phase's first
      real MT strength, and it **met the original 1.10.0 trigger on its own**
      (PLAN 9.12 — later superseded by the user's 1.9.2 version choice).
      ⚠ SPRT-stopped estimate, so the magnitude is optimistically biased, and
      it is a **bundle** number — never quote it as (a)'s value.
      — SMP clock safety.
      Helpers no longer own a clock (they broke their own iterative-deepening
      loop and idled exactly when main *extends*), no longer inherit the depth
      limit, and the time reserve gains `+30 ms` once Threads>1 (our 2048-node
      poll stretches to 50–100 ms under contention against a ~20 ms reserve —
      the configuration in which Rarog measured 10 forfeits in 240 games @4T).
      **1T byte-identical; bench 11,941,440; CTest 12/12.**
      Mechanism confirmed before spending games — main's share of pool nodes at
      4T (25% = fully-used pool): `26.1 33.4 26.5 24.8 25.1` → `24.9 24.9 24.9
      25.2 25.1`. The variance collapse is the tell.
      Head is now `basilisk-phase94-clocksafety-pext-pgo`.
- [x] **9.5 CLOSED** (2026-07-31) — **no candidate accepted.** The bundle
      was stopped negative; `b` washed and was removed; corrected `a` was
      stopped on a sustained negative trend. `c` remains deferred because its
      repair gate was conditional on the `(a)+(b)+(d)` regate passing. Restore
      `Clock94` before every later Phase-9 measurement.
      *(Historical implementation record follows.)*
      **9.5 IMPLEMENTED** (2026-07-30) — SMP coordination
      wave, the phase's value item. (a) bound-typed publication of *every*
      searched root move, with `ordering_score()` consuming the bound;
      (b) pool-seeded aspiration windows (Exact only); (c) symmetric majority
      soft-stop — helpers vote instead of breaking, so the pool can also
      **extend** main; (d) the merge is no longer depth-then-score, and its
      mate branch can no longer prefer a shallower mate.
      **1T inert by construction and by test; bench 11,941,440; CTest 12/12.**
      4T telemetry vs the 9.3 baseline: main share 25.6% → 23.7%, same-key
      stores 33.90% → 32.94%, helpers now finish depth 17 while main honours
      `go depth 16`.
      ```powershell
      ./tools/sprt.ps1 `
          -EngineA tools	est_enginesasilisk-phase95-coordination-pext-pgo.exe `
          -EngineB tools	est_enginesasilisk-phase94-clocksafety-pext-pgo.exe `
          -NameA Coord95 -NameB Clock94 -Elo1 3 -Threads 4
      ```
      ⚠ Rarog's +102.78 was an undecomposed bundle against its *original* SMP;
      its one decomposed number is +43 for the coordination half and we already
      own two of those pieces. Honest prior is **the remainder, +5…+25** — and
      +100 would not be consistent with the evidence.
- [x] **9.6 PASSED: +1.29% NPS** (2026-07-31)
      Node-invariant index hoists II — **no games; NPS-gated; pays at
      1T *and* 4T.** 8.7 hoisted the continuation rows for +3.03%; the
      pawn-history slot, the `main[side]` row and the `low_ply[ply]` row are
      the three remaining row bases now hoisted by 9.6. ⚠ Do not assume the compiler does
      it — Rarog predicted exactly that, was wrong, and this was its single
      biggest speed win. Corrected implementation: isolated post-9.5 baseline
      `206cd60`, candidate `28dead9`, exact 9.4 main-only clock gate retained,
      and the required row-base/full-index unit test added. Bench
      **11,941,440**, CTest **12/12**; two clean, independently generated PGO
      builds per arm. On the idle box, pooled candidate-vs-baseline measured
      **+1.29% median NPS**, 95% CI **[−0.25%, +2.64%]**, best-of **+0.50%**,
      A faster in 9/16 rounds, with identical fingerprints. Accepted under the
      Phase-8.7 strict-work-reduction precedent (which retained +0.36% with a
      similarly narrow zero-crossing CI); quote +1.29% with its CI, not as a
      precise Elo gain. The earlier busy-box self-pair (−3.77%, 3.03–3.64M)
      remains recorded as an invalid-run warning: a concurrency-14 Rarog tuner
      occupied every physical core. Per user direction, no redundant self-pair
      was rerun after that tuner stopped; the harness had already been validated.
- [x] **9.7 CLOSED: BLENDING REMOVED** (2026-07-31). Same-binary 4T NPS,
      removal vs current behavior: **+0.48%**, 95% CI **[−1.66%, +3.45%]**,
      best-of **+0.97%**, 9/16. The simplify SPRT was user-stopped as an
      obvious long wash: last complete report at 1,400 games was
      **+0.99 ±11.70 Elo**, nElo **+1.54 ±18.20**, LLR **+0.23**, zero
      forfeits; 34 further games completed without another report. This is not
      a formal H1. Removal was accepted by engineering decision: no adverse
      trend, positive NPS direction, Rarog's cheaper analogue was neutral, and
      the merge touched ~1.6 MB/helper/search with no demonstrated value.
      Permanently deleted the merge implementation and temporary UCI switch;
      retained the generally useful same-binary options and correct multi-core
      affinity added to `nps_ab.ps1`.
- [x] **9.8 CLOSED: REJECTED** (2026-07-31). The 1T `[0,3]` SPRT for excluding
      depth-0 qsearch TT entries was user-stopped as a wash. Last complete
      report at 4,120 games: **−2.02 ±6.76 Elo**, **−3.18 ±10.61 nElo**,
      **LLR −0.48**, Ptnml **[85,508,898,484,85]**, zero anomalies; 38 more
      games completed without another report. The LLR ranged only from +0.27
      to −0.48 and formal resolution projected many more hours. No strength
      evidence means no behavior change: removed the temporary option, guard,
      and tests. Larger minimum depths remain part of 11.7's joint group.
- [x] **9.9 CLOSED: NEUTRAL, BUNDLE REMOVED** (2026-07-31). User-stopped
      early, not a formal H0: last complete report at 640 games was
      **+0.54 ±17.20 Elo**, **+0.85 ±26.92 nElo**, **LLR −0.01**, Ptnml
      **[15,71,146,74,14]**, zero anomalies; three further games completed.
      There was no positive trend worth hours more. More importantly, the
      rationale for retrying Rarog's robust null (**−0.81 ±2.55 over 28,362**)
      was that accepted 9.5 pool windows would correlate our helpers; all of
      9.5 was rejected, so that premise disappeared. The pre-registered wash
      rule therefore applies: LMR jitter, quiet jitter, root rotation and the
      temporary switch/tests were removed.
- [x] **9.10 CLOSED: NOT APPLICABLE** (2026-07-31; no code, no games). The
      4-of-4 sweep required accepted 9.5(c) majority-stop plumbing; none was
      accepted. The 9.5 bundle that contained it was already strongly negative
      (**−41.45 ±17.75 Elo at 640 games**), and Rarog's 2T unanimity result does
      not extrapolate to 4T: 2-of-2 is unanimity, whereas 3-of-4 already waits
      beyond a bare majority. Reintroducing rejected ballot code solely to wait
      even longer would be a new, riskier feature, not a one-line sweep.
- [x] **9.11 REJECTED: USER-STOPPED WASH** — one final HCE Texel re-fit on post-1.9.1
      labels. ⚠ Not a repair: the Grand Bullet arena does **not** show a 1.9.1
      regression (head-to-head **1.9.1 beats 1.9.0 27–57–24**; the pool gap is
      0.79% ≈ 5.5 Elo inside one SE; the search is bit-identical and 1.9.1
      searches *deeper*, 12.8 vs 12.7). The real question is whether a ≈+52 Elo
      stronger label generator beats the cycle-6 wash. **Against it:** the
      cliff is measured three times across two engines, and a re-fit de-tunes
      `hcefinal`, so a standalone gate is rigged low. **Why it is offered
      anyway:** the corpus is dual-use — the same PGN feeds the Texel extractor
      *and* Phase 10.2's NNUE training, so the datagen is not wasted on a wash.
      **Datagen: 1T, `tc=inf nodes=N`, diverse book** (at a fixed node budget
      Lazy SMP is strictly *worse* per node and non-reproducible; the quality
      axis is nodes-per-move, not threads). ONE dataset, ONE fit, ONE `[0,3]`.
      **Pre-registered: a wash closes the HCE evaluator permanently** — no
      second cycle and no pre-NNUE margin re-tune to chase it. Infrastructure
      prepared 2026-07-31: datagen now refuses accidental append, fixes and
      records `-srand`, and writes engine/book/command/output hashes;
      `tools/texel/phase911.ps1` enforces the sequential joint bake and exact
      reconstruction checks. **Dataset correction 2026-07-31:** the first
      200,000-game corpus yielded 3,334,121 natural training positions, but the
      registered 1:1.5:1.5 phase cap retained only 1,237,744—below the 1.5M
      warning floor and v17's 3.35M train set. Its fit/candidate was voided
      before games and reverted in `dfebea7`. **Final data-policy correction
      2026-08-01:** the user stopped the disjoint extension after 224,071 games,
      giving **424,071 complete, exact-unique starts** total. The old 16-ply skip
      contradicted Basilisk's original Beast-EPD `--skip-start 0` recipe, and
      1:1.5:1.5 had no measured justification. Extraction now uses five equal
      Hydra/Rarog-style material-phase reservoirs, per-phase sampling inside
      each game, a winning-capture quiet filter, global dedup, exact balanced
      game-level holdout and atomic output. It must produce exactly **3.5M**
      train positions, 700k in each phase, and hard-fails without publishing if
      any phase is short. No further datagen occurs unless that audit proves a
      real short bucket. It remains an initial NNUE slice;
      Phase 10.2 extends it to the separately required 30–60M positions.
      **Exact extraction passed:** 3.5M train (700k × five phases) + 184,211
      balanced holdout, zero parse errors. Deep endgame was limiting at 745,848
      eligible train rows, still 6.5% above quota. Datagen is closed.
      **Registered fit complete:** the joint holdout improved 0.105861 →
      0.105147 (best epoch 28 restored), followed by the required king-safety
      holdout improvement 0.105322 → 0.105126 after restoring its best
      validation checkpoint (pass 27, rather than the overfit pass 39). Exact reconstruction passed
      on 10k train + 10k holdout; 12/12 C++ and 13/13 Python tests pass. The
      candidate bench fingerprint is 15,312,647. **SPRT binary ready:**
      `basilisk-phase911-texel5-pext-pgo.exe`, clean revision `15ddd26`, SHA-256
      `A917BCDE687CF77A6C2F75D23D06F081EB1973170F8FEE965A9C09F373ABF765`.
      Its engine-source diff from `phase911-datagen` is only
      `src/eval_params.h`. Ignore the preserved `VOID-no-best-checkpoint`
      artifact. **Final SPRT:** user-stopped at 6,398 games, +1.52 ± 5.77
      Elo / **+2.24 ± 8.51 nElo**, LLR +0.12/2.94. The estimate spent the
      measured history inside the `[0,3]` indifference region and offered no
      evidence for H1; two symmetric time losses (one per engine) did not
      explain the score. The fitted weights were restored; bench is again
      **11,941,440**. No second HCE cycle or pre-NNUE search SPSA. The upgraded
      five-phase tools and 424,071-game corpus stay for Phase 10; the tested
      binary is retained only as `REJECTED-wash`.
- [x] **9.12 PASSED / PHASE 9 CLOSED** (2026-08-01) — 1.9.2 release gate.
      Bench **11,941,440**, CTest 12/12, Texel/Python 13/13. Primary 4T
      fastchess evidence remains 9.4's **+30.42 ±8.77 Elo**, zero forfeits.
      Colosseum 1.0.1 direct results against 1.9.1: **+11 ±24** at 1T fast
      (107-199-94), **+14 ±34** at 4T fast (55-98-47), and **+26 ±35** at 4T
      `10+0.1` (59-97-44); no reported Basilisk incidents across 5,800 games.
      User explicitly waived the standalone 1T NPS, 4T diag/scaling, duplicate
      1T `10+0.1`, and `60+0.6` runs because fingerprint identity, 9.6 pooled
      PGO NPS, 9.3/9.4 MT evidence and the deployment gauntlets already cover
      the release risk. Version/README/changelog are final; manual publish
      remains.

**Explicitly not in Phase 9** (no duplicates — see the table in PLAN §5):
`cutoffCnt` and the LMR `gives_check` fix stay in 11.1/11.7; the
history-coverage residue stays in 11.7's tune; correction-history semantics
stay at 8.5.5 (**EV downgraded** — Rarog's decomposition puts the tactical
guard at **−56** and the rest at ≈ +1.4); fail-soft qsearch and do-deeper are
closed with measurements on both sides.

---

## ▶ Next — NNUE line

- [ ] **Phase 8.5 (runway only)** — pure NNUE data prep, on `development` after
      Phase 9, then the single rebase:
  - [ ] **8.5.3** dirty-piece contract (accumulator inputs; 0 Elo; attaches to
        8.6.10's centralized `do_move`/`undo_move` + `PlyContext`).
  - [ ] **8.5.14** TT graph-history semantics (down-scoped; eval-adjacent parts
        are easier post-NNUE).
  - [ ] **8.5.15** frozen teacher benchmark (baselines the released **1.9.2**
        HCE head after its release gates pass).
  - [ ] **8.5.16** `net_trainer` preflight (split/dedup/filter/manifest/tests).
  - [ ] **Handoff** — record the exact `development` SHA; rebase `nnue` once.
- [ ] **Phase 10 — baseline NNUE (`nnue`, ships as 2.0.0):**
  - [ ] **10.0** rebase/inventory the partial NNUE against the raw Bullet
        `quantised.bin` contract (payload, padding, inferred H, SHA).
  - [ ] **10.1** `net_trainer` validation, best checkpoint, resume, manifests,
        hashes, explicit seeds, strict CLI/errors, CI.
  - [ ] **10.2–10.3** 30–60M unique-position dataset, controlled label/node/
        adjudication experiments, reproducible H1024 chess768/1×8 training.
        ⚑ **Candidate axis (2026-07-30):** select positions by *behavioural*
        disagreement with the teacher (its best move vs ours), **not** by eval
        error. An external report measured cp-loss mining at **0.5%** vs
        recall-failure mining at **~30%** on the same infrastructure — the same
        shape as our own lesson 3 (holdout MSE does not predict Elo). Our
        selection today is diversity and nothing else. Define the value-net
        analogue before spending machine time; "large eval error" is precisely
        the criterion that measured 0.5%.
  - [ ] **10.4** scalar loader / full-recompute exact H32 + all-bucket
        conformance first.
  - [ ] **10.5** incremental accumulator, property verification, scalar/SIMD
        parity, ISA kernels, new PGO profile. **Includes the LLVM-pin rider.**
  - [ ] **10.6** net vs the released HCE at STC, `10+0.1`, frozen-teacher
        cohorts and NPS; iterate one variable at a time.
  - [ ] **10.7** emergency search-scale safety calibration only — the final
        tune waits for 11.7.
  - [ ] **10.8** 2.0.0 release: embedded-net SHA + production asset gate.

## Later — post-NNUE

- [ ] **Phase 11 — final 1T search architecture + tune** (prior +15–40 @1T,
      heavily non-additive; architecture first, one tune at the end):
  - [ ] **11.0** ordering/pruning **recall telemetry** — no games, bench-neutral.
        We count how many moves LMP/futility/history pruning discards but never
        whether **the eventual best move was among them**. Add a cutoff-rank
        histogram + first-move-cutoff rate + best-move-was-pruned counter.
        Motivation is our own record: the 8.5.10 ladder was vetoed three times
        on node counts, and the rung that reached a gate read **−26% nodes and
        −84 Elo** — node count conflates *orders better* with *prunes harder*.
        Diagnostic only, never a gate.
  - [ ] **11.1** unified contextual reduction (one signed `r`, reused for
        futility/history/SEE). **MUST INCLUDE the LMR post-move-`gives_check`
        fix by construction** (its standalone fix lost −21 as a de-tuning
        victim). ⚠ `cutoffCnt` is now a *candidate input with a negative
        prior*, not a must-include — Rarog gated it and lost −7.78.
  - [ ] **11.2** result-dependent verification.
  - [x] **11.3 MOVED** — TT density & replacement was pulled forward to 8.5.D1
        and shipped in 1.9.0. Slot kept so 11.4–11.7 references stay stable.
  - [ ] **11.4** bound quality (also unblocks the 8.5.6 in-check qsearch store).
  - [ ] **11.5** ProbCut / null verification region / IIR audit.
  - [ ] **11.6** correction-history consumption v2.
  - [ ] **11.7** the ONE comprehensive SPSA, under the 9.1 schedule and gate-11
        doctrine (≥5,000 iterations, merged groups, tail-mean bake). Carries the
        LMR family, the re-tune of the `gives_check` fix, `TmInstability`, and
        the history-coverage residue.
- [ ] **Phase 12 — MT scaling beyond Phase 9** (re-validate 9.5 on the net,
      the ≥8-thread regime, voting at scale, shared state at NNUE scale,
      NUMA/memory). ⛔ engine-side thread pinning is permanently out of scope.
- [ ] **Phase 13 — NNUE architecture, data & frontier loop** (13.0 evidence
      review → 13.1 versioned architecture ladder → 13.2 data/training ladder →
      13.3 HCE recovery menu, closed by default → 13.4 frontier acceptance).

---

## Completed

- [x] **Phases 0–7 PASSED** — 1.4.9 → 1.8.0: test harness, search baseline,
      mature HCE. ≈ +93 Elo/1.7.0 at 3+0.03 from time management, a
      search-efficiency pass and a from-scratch eval refresh; the sixth
      self-play cycle washed, which closed result-label refitting.
- [x] **Phase 8 + 8.5 (pre-1.9.0) PASSED → 1.9.0** (≈ +52 Elo/1.8.0):
      audit-driven correctness/infra (rule-50/mate precedence, null clock,
      legal-EP hashing, SEE pin-awareness) + the pre-NNUE strength ladder +
      the `hcefinal` SPSA (+35.94).
- [x] **Phase 8.6 PASSED (neutral by design) → 1.9.1** — X-macro params, TT
      contract + static asserts, sanitizer gate repaired, the structure-era
      refactor (one `do_move`/`undo_move` seam = the NNUE attach point), C++23
      pass, clang-tidy zero, search telemetry, CI in Rarog's shape. Verified
      +0.17 ± 4.41 vs 1.9.0.
- [x] **8.6.7 REJECTED** — blanket check-extension removal, −10.17 ± … at 4.7k
      games. Requeued post-NNUE as a bundle with the consumers it de-tunes.
- [x] **8.6.8A CLOSED** — accept-audit after the placement-lottery incident:
      every re-measured 1.9.0 accept was REAL, nothing removed, headlines
      ~40–55% bias-inflated. Two reusable methods: disjoint probes combine in
      quadrature; a ~+2 Elo feature needs ~200k games to separate from 0.
- [x] **Phase 8.7 PASSED → 1.9.1** — profile-guided speed pass: built the NPS
      instrument first, then took only measured wins (conthist-hoist +3.03%,
      mobility switch-hoist +0.89%, eval slider-reuse +0.39%, SEE-verdict-memo
      +0.36%, all bench-identical; baked-magic startup 603→38 ms). Batch SPRT
      **+8.69 ± 6.63**. Rejected/retired: CheckInfo check-hint (−1.8%),
      pin-sharing (−0.16%), pawn-cache (89–99% hit), PGO enrichment.

---

## Development rhythm

> **Strength-first rule (user decision 2026-08-01):** if the model discovers a
> wrong, weak, sub-optimal, obsolete or avoidably constrained implementation,
> it must surface it instead of silently working around it. Explain the
> evidence, likely benefit, cost and impact on the active experiment. If the
> repair changes scope or protocol, stop and decide with the user whether to do
> it now, schedule it, or knowingly retain the constraint. Existing code and
> this guide are not authorities merely because they are existing; the goal is
> the strongest possible engine. The one-candidate and measurement gates still
> apply after that decision.

```text
You   → “Implement the next step.”
Model → inspect state; surface avoidable weaknesses/constraints; after any
        needed user decision, build ONE semantic candidate, test, commit on
        development, hand you exactly the long-running command.
You   → run SPRT/SPSA/gauntlet/datagen and paste the result.
Model → accept+document or revert+document, then advance the plan.
```

| Claim | Gate |
|---|---|
| Strength gain (1T) | `elo0=0, elo1=3` SPRT, `tc=3+0.03`, Hash 64, Threads 1 |
| Strength gain (MT) | same bounds at **Threads=4, Hash 256, ≥10k games**, after a 4T null |
| Mandatory correctness | regression/oracle tests + pre-registered non-inferiority |
| Behaviour-neutral enabler | exact bench parity + correctness/performance evidence |
| Speed-only change | bench-identical + pooled-PGO `nps_ab`, self-pair validated |
| Eval/network checkpoint | validation selects within a run; SPRT decides strength |

Never bundle unrelated behaviour changes. Where a bundle *is* the gate (9.5),
the decomposition order is pre-registered before the run.

**Durable measurement rules:** NPS via `tools/nps_ab.ps1` — self-pair-validate
to ~0.00%, pin a HIGH-numbered core (CPU 0 carries interrupt work), pool ≥2 PGO
builds/arm, idle box (a YouTube video alone invalidated a sub-1% run); never
two pinned harnesses at once; **bench-identity ≠ NPS-identity**; and an early
SPRT positive is the default shape of a null — read the estimate at the CI you
actually need.

---

## Common commands

```powershell
cd D:\code\basilisk
.\tools\build_test.ps1 -Suffix mystep-name

# 1T strength gate
.\tools\sprt.ps1 `
    -EngineA tools\test_engines\basilisk-candidate-pext-pgo.exe `
    -EngineB tools\test_engines\basilisk-baseline-pext-pgo.exe `
    -NameA "Candidate" -NameB "Baseline" -Elo1 3
# Non-regression: -Elo0 -3 -Elo1 0      LTC: -TC "10+0.1"

# 4T gate. Null FIRST — require zero forfeits — then the candidate. Hash (256),
# concurrency (3) and the dropped affinity pin all follow from -Threads 4;
# ~10k games is the floor for a 4T reading, and the null takes ~10-12 h.
.\tools\sprt.ps1 -EngineA <copy.exe> -EngineB <same.exe> `
    -NameA Self -NameB Self2 -Mode calibrate -Threads 4 -Games 10000
.\tools\sprt.ps1 -EngineA <cand.exe> -EngineB <base.exe> `
    -NameA Candidate -NameB Baseline -Elo1 3 -Threads 4

# Thread-scaling diagnostic (NOT a gate): same binary, 4T vs 1T, same TC.
.\tools\sprt.ps1 -EngineA <head.exe> -EngineB <head.exe> `
    -NameA "4T" -NameB "1T" -ThreadsA 4 -ThreadsB 1 -Mode fixed -Games 4000

# NPS A/B — self pair first, must read ~0.00%
.\tools\nps_ab.ps1 -EngineA <cand.exe> -EngineB <cand.exe>
.\tools\nps_ab.ps1 -EngineA <cand.exe> -EngineB <base.exe> -Rounds 12

# SPSA. -Iterations is the horizon: the whole schedule is back-solved from it,
# the run STOPS ITSELF there, and it is FROZEN at first launch (a -Resume
# ignores a new -Iterations/-REnd and says so). <5000 throws (-AllowShortRun
# overrides). -REnd 0.0031 is the end-of-run step ratio; leave it alone unless
# you have re-run the verifier.
.\tools\spsa.ps1 -ConfigGroup <group> -EngineSuffix <base> -Iterations 5000
.\tools\spsa.ps1 -ConfigGroup <group> -Resume        # continue an interrupted run
python .\tools\verify_spsa_schedule.py               # schedule self-test, no games
# Bake the TAIL MEAN of the whole vector from tools\weather-factory\tuner\trajectory.csv

# Field gauntlet — Colosseum is the tournament manager; this is the fallback.
# Run at BOTH 1T and 4T.
.\tools\gauntlet.ps1 -Engine <candidate> `
    -Opponents <prior-release>,<field...> -TC "10+0.1"

# NNUE Bullet pipeline (Phase 10; NVIDIA CUDA training)
cd D:\code\net_trainer\trainer
cargo build --release --features cuda
cd ..
python tools\datagen.py --engine <hce.exe> `
    --book data\books\openings.epd --rounds 500000 --out data\pgn\selfplay.pgn
python tools\extract_nnue.py data\pgn\selfplay.pgn --out data\txt\train.txt
.\trainer\target\release\net-trainer.exe convert data\txt\train.txt data\txt\train.bf
.\trainer\target\release\net-trainer.exe shuffle data\txt\train.bf `
    data\txt\train_shuffled.bf --seed 42
.\trainer\target\release\net-trainer.exe train data\txt\train_shuffled.bf `
    --hidden 1024 --id run1 --out trainer\checkpoints --superbatches 160 --wdl 0.3
```

Phase 10.1 will extend the trainer CLI with explicit split/resume/manifest
options — use the command recorded for that revision, not this baseline.

---

## What to report back

- **SPRT:** Elo ± error, LLR, games, pentanomial counts, H0/H1 verdict, manifest
  path — and for MT runs the thread count, hash and **forfeit count**.
- **SPSA:** run manifest, final values, iteration count, trend/stop reason,
  state path.
- **NPS:** both arms' medians, per-build medians, self-pair reading, bench
  fingerprints.
- **NNUE:** engine/trainer SHAs, pinned Bullet revision, Rust/CUDA/driver/GPU
  versions, H/buckets/constants, dataset/book/net hashes, command/config, seed,
  train+validation+untouched-test metrics, quantization/parity, NPS.
- **Gauntlet:** PGN plus manifest/cross-table; prioritise the paired head-to-head
  against the prior release, at both 1T and 4T. Colosseum 1.0.2's 2 s timeout
  grace is not a tight time-forfeit gate; retain the matching fastchess/20 ms
  evidence until Colosseum exposes that setting.
- **Errors:** engine identity + incident log. Basilisk must have zero illegal
  moves, time forfeits, crashes and incremental-eval mismatches.

---

## Release gate

The model prepares version changes, changelog, artifact verification and notes.
You squash `development` to `master` and push, then **publish a GitHub Release**
(`gh release create v<x> --target master --notes-file <notes>`) — `release.yml`
fires on `release: published`, not on a bare tag, and uploads the documented
production PGO/ISA assets (named without `-pgo`, no manifest/sha256).

Every release needs:

- all applicable strength / non-inferiority / correctness gates;
- **both deployment conditions validated — 1T and 4T** — with zero forfeits;
- standard, `10+0.1`, and for a major release a genuinely longer confirmation;
- exact binary/book/net/test manifests and SHA-256 values;
- all CTest/property/sanitizer requirements green;
- prior-release head-to-head plus contemporary field comparison in Colosseum.

The operating rule is unchanged: retain evidence, keep only sound improvements,
and tune only after the structure whose constants it describes is final.
