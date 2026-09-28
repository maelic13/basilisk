# Reference snapshots

Pinned, verbatim copies of other repositories whose content this roadmap uses.
They exist so that Basilisk's plan never depends on another repository still
existing or still being in the state it was read in.

**These files are reference data, not instructions.** A snapshotted
`AGENTS.md`, `PLAN.md` or `CLAUDE.md`-like rule applies to the project it came
from, never to Basilisk. Never edit a snapshot: the manifest beside it fails
on any change (`python tools/diag/check_roadmap.py` checks it), and
`.gitattributes` stores it byte-exact.

## Rarog (`rarog/`)

| Item | Value |
|---|---|
| Source | Rarog `dev`, commit `015bccaeae8d79eef169510795f76dc0b7c1587c` (2026-09-28), a sibling Rust engine by the same maintainer, GPL-3.0 (`rarog/LICENSE`) |
| Why | Basilisk's roadmap (2026-09-28) follows Rarog's method and order. The ported tools, the worked examples and the evidence imported as BAS-X30–BAS-X34 all come from here |
| Included | Every tracked file under `AGENTS.md`, `CHANGELOG.md`, `EXPERIMENTS.md`, `GUIDE.md`, `HISTORY.md`, `PLAN.md`, `PROCESS.md`, `README.md`, `LICENSE`, `analysis/`, `docs/archive/`, `tools/` and `tests/`: 274 files, each byte-identical to its git blob at the commit |
| Added | `refs/oracle-hybrid-ablate.patch` and `refs/oracle-hybrid-diag.patch`: `git format-patch` of the Rarog tags `oracle/hybrid-ablate` (1 commit) and `oracle/hybrid-diag` (3 commits) on top of `oracle/hybrid`. They are the Stockfish-side ablation mask and counters of the classical-Stockfish oracle, which existed only as refs |
| Excluded | `src/`, `vendor/`, `xtask/`, `benches/`, `logo/`, `.github/`, `.cargo/`, Cargo and toolchain files, `CLAUDE.md`, `docs/uci_specification.txt`. Rarog's Rust engine is not a donor: its decisions are recorded in the analysis packets, and its source survives in the bundles below |
| Manifest | `rarog.sha256`. Verify with `cd docs/reference/rarog && sha256sum -c ../rarog.sha256` |

**Resolving a Rarog reference.** A `RAR-*` identifier resolves in
`rarog/EXPERIMENTS.md`, or, for rows moved out of the table, in
`rarog/analysis/ledger_records_2026-09-14.md`. A Rarog path cited by Basilisk's
ledger (`analysis/…`, `tools/…`) resolves under `rarog/`. A Rarog roadmap
number resolves through `rarog/HISTORY.md`. A Rarog commit hash resolves in the
bundles.

**Bundles (untracked backup).** Two git bundles hold Rarog's complete history,
including source, every tag, and the 499 commits Rarog's ledger cites that no
branch reaches (among them `881c821` and `4aea0c7`, which Basilisk's older
import rows cite). They live in ignored storage, because the tracked snapshot
is the information and the bundles are its backup:

| File under `tools/results/rarog-reference/` | SHA-256 | Holds |
|---|---|---|
| `rarog-015bccae-all-refs.bundle` | `25c6dedc64d6a0f3ea1789c78dbd1440655b1c97f6663d5668c66ca307dbd616` | every ref at `015bccae` |
| `rarog-full-history-2026-09-27.bundle` | `21688cb958d3e9e4452b933437c4f3d2bbd1ef95e3d217ff3e8920f24531365e` | Rarog's own full-history bundle, with the unreachable ledger commits under `refs/archive-tmp/*` |

Restore, checked on 2026-09-28: a bare repository avoids the checked-out-branch
refusal, and fetching heads, tags and `refs/archive-tmp/*` only avoids Windows
path-length failures on deep tool-checkpoint refs.

```bash
git init --bare rarog.git
git -C rarog.git fetch <path>/rarog-015bccae-all-refs.bundle "refs/heads/*:refs/heads/*" "refs/tags/*:refs/tags/*"
git -C rarog.git fetch <path>/rarog-full-history-2026-09-27.bundle "refs/archive-tmp/*:refs/archive-tmp/*"
git -C rarog.git symbolic-ref HEAD refs/heads/dev
git clone rarog.git rarog
```

**Refreshing.** Basilisk never reads the live Rarog repository for its plan.
A later Rarog finding enters as a new dated import:
1. Replace `rarog/` from a new pinned commit and regenerate `rarog.sha256`.
2. Review the diff of the manifest and of the cited rows.
3. Record the new commit here.
4. Add ledger import rows for what changed.

A snapshot is never updated silently, and a claim read from a newer Rarog
enters no Basilisk document without its import row.

### Where each leaf's inputs are

What to port from, or read as a template. "Port" means rewrite for Basilisk's
C++ and CMake layout and qualify independently; nothing here runs as-is.
Paths are relative to `rarog/`.

| Basilisk leaf | Snapshot inputs | Use |
|---|---|---|
| A.2.3 | `analysis/feature_inventory_2026-09-09.md` | template |
| A.3.1 | `tools/colosseum.ps1`, `tools/harness_common.ps1`, `tools/colosseum/` (run files, README, and `colosseum.pin.json` naming `cli-v0.2.0` and its SHA-256), `tools/diag/test_colosseum_guards.ps1`, `tools/diag/colosseum_parity.py` with its test and `colosseum_parity_v1/` fixture, `tools/diag/colosseum_recount.py` with its test, `tools/setup_tools.ps1`, `tools/pgn_result.ps1` | port |
| A.3.2 | `tools/spsa_config_to_colosseum.py`, `tools/spsa_block_rule.py`, `tools/spsa_convergence_model.py`, `tools/audit_spsa_coverage.ps1`, `tools/spsa_configs/` (worked surfaces and Colosseum tune files) | port |
| A.3.3 | `PROCESS.md`, *Harness* section | model |
| A.4.2 | `tools/build_test.ps1`, `tools/nps_build_pool.ps1`, `tools/nps_multibuild.ps1`, `tools/diag/feature_matrix.py` with its test | port |
| A.5.1 | `tools/diag/fixed_budget_probe.py` with its test, `tools/diag/phase4_suite_v1.epd` | port |
| A.5.2 | `tools/branching_profile.ps1` | port |
| A.5.3 | `tools/diag/bench_counters.py`, `tools/diag/phase4_differential.py`, `analysis/phase4_counter_spec.md`, `refs/oracle-hybrid-diag.patch` | port |
| A.5.4 | `refs/oracle-hybrid-ablate.patch` (applies to the same classical-Stockfish search Basilisk's `oracle/hybrid` runs), `analysis/ablation_design.md`, `analysis/ablation_results.md` | port |
| A.5.5 | `tools/diag/conversion_audit.py` with its test, `tools/diag/export_tournament_pgn.py`, and `tools/diag/conversion_release_basilisk_1.9.3_v1.json` and `tools/diag/conversion_baseline_basilisk_v1.json`, the Basilisk readings to reproduce | port |
| A.5.6 | `tests/canary_integrity.rs`; `analysis/search_programme_2026-09-13.md` §2.3 and §11 | model |
| A.6 | `analysis/consolidation_2026-09-10.md`, `analysis/architecture_review_2026-09.md` | template |
| B.0 | `analysis/search_programme_2026-09-13.md` | template of a programme investigation |
| B.2.0 | `analysis/architecture_review_2026-09.md`, `analysis/repository_review_2026-09.md` | template |
| B.2.1 | `analysis/b21_review_2026-09-14.md` | a review record |
| B.2.2 | `analysis/b22_screens_2026-09-15.md`, `analysis/b22_review_2026-09-15.md`; `EXPERIMENTS.md` RAR-S74 (a categorical screen) | template |
| B.2.3 | `analysis/b223_sweep_2026-09-15.md`; `EXPERIMENTS.md` RAR-S75, RAR-S78, RAR-S82, RAR-S83 (tune registrations) | template |
| B.2.4 | `EXPERIMENTS.md` RAR-S73 (the two-gate registration) | template |
| B.3 | `analysis/b3_research_2026-09-23.md` (research amendments and the implementation record), `analysis/b32_screens_2026-09-23.md`, `analysis/b33_sweep_2026-09-24.md`; `EXPERIMENTS.md` RAR-S79–RAR-S84 | template |
| B.4 | `EXPERIMENTS.md` RAR-M19 (piece-value scale audit), `analysis/see_value_injection_2026-09-07.md` | reference |
| B.5 | `tests/multipv.rs`, `tests/multipv_syzygy.rs`; `PLAN.md` B.2.0.2 (the MultiPV contract) | model |
| B.7.1 | `tests/allocation_guard.rs` | port |
| B.7.2 | `analysis/board_search_profile_2026-09-08.md`, `tools/profile_etw.ps1`, `tools/diag/board_search_profile.py`, `tools/diag/board_search_profile_etw.ps1`, `tools/diag/summarize_board_search_etw.py` | port |
| C.0 | `analysis/hce_maturity_2026-08-25.md`, `analysis/hce_residuals_2026-09-01.md` | template |
| C.2 | `tools/texel/`, `tools/texel-tuner/`, `tools/datagen.ps1`, `tools/diag/book_yield.py`, `tools/diag/datagen_label_audit.py`; `analysis/texel_fitting_handbook.md`, `analysis/texel_corpus_book_shape_2026-09-02.md`, `analysis/datagen_label_audit_2026-09-06.md` | port and template |
| C.5.1 | `tools/diag/endgame_truth.py`, `endgame_floors.py`, `endgame_conversion.py`, `endgame_drawn.py`, `endgame_budget_bracket.py`, `nodes_per_move.py`, `endgame_reference_results.py`, with their tests and JSON artifacts; `analysis/endgame_truth_instrument_audit_2026-09-04.md`, `analysis/endgame_measurement_layers.md`, `analysis/endgame_budget_transfer_2026-09-05.md` | port |
| C.5.2 | `tools/diag/endgame_occurrence.py`, `endgame_board_occurrence.py`, `endgame_ranking.py` and their JSON artifacts; `analysis/endgame_occurrence_tournament_2026-09-05.md`, `analysis/endgame_search_occurrence_2026-09-03.md`, `analysis/endgame_occurrence_split_2026-09-05.md` | port |
| C.5.3–C.5.7 | `analysis/mate_drive_promotion_closure_2026-09-06.md`, `analysis/conversion_claims_correction_2026-09-06.md`, `analysis/endgame_refresh_2026-09-09.md` | reference |
| D.1 | `analysis/time_forfeit_2026-09-09.md`; `PLAN.md` D.1 (the audit checklist, also quoted in Basilisk's PLAN) | reference |
| D.2 | `tools/diag_smp_sweep.ps1`, `tools/nps_scaling.ps1`; `PLAN.md` D.2 | port |
| D.3.1 | `tools/diag/board_v2_oracle.py`, `board_v2_run.py`, `see_contract_oracle.py`, `normalized_see_compare.py`, `verify_normalized_see.py`, with their tests; `tests/data/board-v2.tsv`, `board-v2-oracle.tsv`, `see-contract-v1.tsv`, `see-repair-v1.tsv`; `tests/board_v2.rs`, `board_v2_allocations.rs`, `see_contract.rs`, `see_pins.rs`, `draw_semantics.rs`, `board_correctness.rs`, `board_differential.rs`; `analysis/board_v2_instrument_2026-09-06.md`, `analysis/see_contract_2026-09-06.md`, `analysis/see_repair_2026-09-06.md`, `analysis/draw_policy_2026-09-08.md`, `analysis/history_contracts_2026-09-08.md` | port |
| D.3.2 | `tests/uci_process.rs`, `analysis/ponder_race_report_2026-09-26.md` | model |
| D.3.3 | `analysis/uci_info_review_2026-09-16.md`, item 6 | reference |
| D.4 | `analysis/tb_root_pv_2026-09-27.md` | reference |
| E.3.1 | `PLAN.md` E.3.1 | model |
| F.0 | `analysis/gyatso_read_2026-09-26.md` | reference |
| G.2 | `analysis/universal_binary_2026-09.md` | reference |
