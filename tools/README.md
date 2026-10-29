# Tools

One line per tool: what it does, and the procedure, step or ledger entry that
uses it. Run everything from the repository root. The measurement rules are in
`agents/measurement.md` and the procedures in `docs/PROCESS.md`. Raw outputs go
to ignored `tools/results/`, and test binaries to ignored `tools/test_engines/`.
Ledger rows cite tool and fixture names, so these names do not change.

## Games and harness

Colosseum CLI is the main path. fastchess and weather-factory stay installed
and working as the backup, which PROCESS *Harness* says when to use.

### Main path: Colosseum CLI

| Tool | Purpose | Used by |
|---|---|---|
| `colosseum.ps1` | A gate, fixed match, null pair or gauntlet on the pinned Colosseum CLI, from the committed run files. It applies every provenance, policy and idle-host guard, writes a per-run manifest, then runs the post-run fault rule and an independent recount. It resumes a `-Dir` at its recorded seed | PROCESS *Harness*, *Common commands*; every gate since A.3 |
| `colosseum/` | The run files that hold Basilisk's conditions (shared, the SPRT brackets, fixed match, gauntlet, tune, null), and `colosseum.pin.json`, which pins the runner by revision and SHA-256 | `colosseum.ps1`, `setup_tools.ps1`; `colosseum/README.md` |
| `spsa_colosseum.ps1` | One guarded Colosseum SPSA block from the generated search surface | PROCESS *Harness*; BAS-S22 |
| `spsa_block_rule.py` | PLAN rule 7c's movement rule applied to a finished block | PROCESS *Common commands*; BAS-S22 |
| `diag/colosseum_parity.py` | A Colosseum dry run compared field by field with a recorded `sprt.ps1` manifest | the harness parity of A.3 |
| `diag/colosseum_recount.py` | A run's paired sample recounted from its PGN | `colosseum.ps1`'s post-run check |
| `diag/pgn_census.py` | One engine's head-to-head records from PGN headers | `colosseum.ps1`'s gauntlet census; BAS-M08 |
| `diag/export_tournament_pgn.py` | One Colosseum desktop tournament exported to a PGN archive, the database read in place | desktop-app pool reads (BAS-M11) |
| `diag/tb_gate_audit.py` | A tablebase-enabled gate audited from its PGN | BAS-S19 |
| `diag/conversion_audit.py` | Games thrown away after a persistent material advantage | BAS-M08 |
| `ponder_match.py` | A ponder-on match, the protocol gate fastchess cannot run | BAS-C11, BAS-C15 |

### Frozen launchers

Each runs one registered experiment with its binaries pinned by SHA-256, and is
kept so the run can be reproduced from its ledger entry.

| Launcher | Experiment |
|---|---|
| `run_a72_gauntlet.ps1` | BAS-M10, the 4-thread baseline gauntlet (void) |
| `run_a73_oracle_g0.ps1` | BAS-O05, the oracle deficit meter G(0) |
| `run_a74_nps_baseline.ps1` | BAS-P12, the pooled-PGO NPS baseline of 1.10.1 |
| `run_a820_nps_baseline.ps1` | BAS-P15, the pooled-PGO NPS baseline of A.8's head |
| `run_b02_oracle_g32.ps1` | BAS-S18, the oracle deficit meter G(32) |
| `run_b1_nps_gate.ps1` | BAS-P16, B.1's NPS gate |
| `run_b20_nps_gate.ps1` | BAS-P17, B.2.0's NPS pool |
| `run_b22_games.ps1` | BAS-S21, cluster 1's unfitted paired run |
| `run_b23_block1_match.ps1` | BAS-S24, SPSA block 1's centres against the seeds |
| `run_b23_vs_1102.ps1` | BAS-S25, the block-1 state against 1.10.2 |
| `run_b24_gates.ps1` | BAS-S23, cluster 1's two gates |
| `run_b10_gate.ps1` | BAS-S26, the piece-square orientation repair |
| `run_head_reads.ps1` | BAS-S27, the head against 1.10.2 at `10+0.1`, four threads and with tablebases |
| `run_a94_nps_reads.ps1` | BAS-P18 and BAS-P19, the pooled NPS read's null and the clang 22 to 23 read |
| `run_a15_nps_reads.ps1` | BAS-P20, the universal binary and the tier ladder |
| `run_a156_nps_reads.ps1` | BAS-P21, the universal binary with 64-byte function alignment |
| `run_a158_nps_reads.ps1` | BAS-P22, the native release build and MSVC against the universal binary |
| `run_a159_nps_reads.ps1` | BAS-P24, the universal binary's assembly route for Clang and GCC |

### Backup path: fastchess and weather-factory

| Tool | Purpose | Used by |
|---|---|---|
| `sprt.ps1` | An SPRT between two Basilisk binaries on fastchess | PROCESS *Harness*, *Adjudication* |
| `gauntlet.ps1` | Fixed-game matches against reference binaries on fastchess | PROCESS *Adjudication* |
| `spsa.ps1` | A weather-factory SPSA tune, set up and run in one command | PROCESS *Harness* |
| `weather-factory-overlay/` | Basilisk's copies of weather-factory's runner, SPSA core and state tools, copied over the clone and checked byte-identical | `spsa.ps1`, `setup_tools.ps1` |
| `verify_spsa_schedule.py` | weather-factory's games-against-iterations schedule checked without playing | `spsa.ps1` |
| `watch.ps1` | A console-noise filter for long fastchess and weather-factory runs | `sprt.ps1`, `spsa.ps1` |
| `setup_tools.ps1` | Stage Colosseum CLI (from its pinned release, both digests checked), fastchess, the UHO book and weather-factory | a fresh checkout |

## Builds, speed and profiles

| Tool | Purpose | Used by |
|---|---|---|
| `build_test.ps1` | A final-PGO PEXT test binary of one flavor (Release, Tune, Diag, Ablate, Profile) with its manifest; it warns when the local clang's LLVM major is not the release pin | PROCESS *Build, bench and NPS* |
| `nps_pool.ps1` | A pool of four independent PGO builds of one clean revision, built (another revision from a temporary worktree) or adopted | PROCESS *Build, bench and NPS*; A.9.4 |
| `nps_read.ps1` | The pooled NPS read: two or more pools and named contrasts between them, interleaved cycles of a pinned `bench 13 3`, every run archived, step 1 and `-Extend` | PROCESS *Build, bench and NPS*; BAS-P18, BAS-P19, BAS-P21 |
| `diag/nps_read.py` | The read's decision from its archive: per-build medians, a Welch t-interval per contrast, the two steps and the disturbance rule | `nps_read.ps1` |
| `nps_ab.ps1` | The earlier bootstrap NPS A/B, kept only to compare with rows before A.9.4 | BAS-P12 to BAS-P17 |
| `profile.ps1` | A user-mode sampling profile of a Profile-flavor engine over `bench 13` or an EPD cohort | PROCESS *Build, bench and NPS*; B.7.2, C.12 |
| `diag/profile_regions.py`, `.json` | The profile's samples charged by inline chain to the innermost engine file's region, and refused when that attribution cannot be trusted | `profile.ps1` |
| `diag/verify_isa.py` | Each asset's CPU-tier contract from its disassembly: portable, AVX2, PEXT, and ARM64 with its required `prfm` | CI, Release |
| `diag/release_check.py` | The release checks: version sources, CHANGELOG section, GUIDE's released row and fingerprint, the release line | PROCESS *Release*; `release.yml` |

## Records

| Tool | Purpose | Used by |
|---|---|---|
| `diag/check_roadmap.py` | PLAN's structure, order, register and fingerprint; the ledger's index against its entries, dispositions included; regenerates GUIDE's step list | AGENTS; every PLAN change; CI |
| `diag/check_citations.py`, `diag/citation_exceptions.tsv` | Every cited commit stays reachable, or is declared another repository's | `agents/records.md`; PROCESS *Delivery*; CI |

## Search instruments

| Tool | Purpose | Used by |
|---|---|---|
| `diag/run_suite.py`, `diag/suite_v1.epd`, `diag/suite_v2.epd` | The fixed differential suite, run at a fixed depth with per-position counters | PROCESS *Fixed-budget search probes* |
| `diag/build_suite.py` | Builds the versioned differential suite | `diag/suite_v1.epd` |
| `diag/baseline_v1.json` | The counter baseline of `suite_v1` at depth 12 | `run_suite.py` comparisons |
| `diag/fixed_budget_probe.py` | Fixed-budget depth, PV and node probes over an EPD suite | B.0, B.2.3 |
| `diag/branching.py` | A reference-anchored consecutive-depth branching profile | PROCESS *Fixed-budget search probes* |
| `diag/sweep.py` | Engine options swept against the fixed suite, reporting tree shape only | BAS-D10, BAS-D11 |
| `diag/decision_trace.py` | The engine's bounded diagnostic decision trace, captured and validated | B.0, B.2 |
| `diag/ablation_liveness.py` | Every matched `AblationMask` bit proven to change fixed-depth search work | B.0, B.2 |
| `diag/canary.py`, `diag/canary_v1.json` | Immutable classical-oracle WAC canaries, built and enforced | B.0 |
| `diag/scale_ratio.py` | The evaluation scale ratios and the search-minus-static residual | B.0; BAS-C14 |
| `diag/winrate_fit.py` | Stockfish's win-rate model fitted to Basilisk's own games | BAS-C14 |
| `diag/source_census.py` | A mechanical census of the engine source for an architecture review | B.2.0 |
| `diag/uci_probe.py` | Black-box UCI probes for protocol and clock defects | A.8 |
| `oracle/build_oracle.ps1`, `oracle/bridge-board-access.patch` | The classical-Stockfish search oracle rebuilt around a Basilisk revision's evaluation | BAS-O05, BAS-S18 |
| `generate_spsa_surface.py` | The SPSA inputs generated from the search-parameter X-macro | BAS-S22 |
| `spsa_configs/` | The generated search surface: weather-factory configs and the Colosseum tune files | `spsa.ps1`, `spsa_colosseum.ps1`; `spsa_configs/README.md` |

## Endgames

The endgame instruments and frozen cohorts. Most were built in the retired
endgame phase, which HISTORY's *Number map* continues as C.5; the two label
tools serve C.2's corpus work.

| Tool | Purpose | Used by |
|---|---|---|
| `diag/endgame_truth.py` | The Syzygy-truth endgame corpus and conversion baseline | C.5; BAS-E47 |
| `diag/endgame_cohort.py`, `diag/endgame_cohort_v1.epd`, `.manifest.json` | The frozen Syzygy endgame cohort, generated and verified | C.5; A.8.13 |
| `diag/endgame_compare.py` | Position-paired confidence analysis of two truth reports | C.5 |
| `diag/endgame_ceilings.py`, `diag/endgame_ceilings_v1.json`, `_v2.json` | Finite-node family ceilings derived from paired truth reports | C.5; BAS-E08, BAS-E50 |
| `diag/endgame_vetoes.py` | Absolute theory and engine-anomaly vetoes for truth candidates | C.5 |
| `diag/endgame_label_census.py`, `diag/endgame_label_census_v1.json` | Self-play labels checked against Syzygy on every corpus row of six men or fewer | C.2 |
| `diag/datagen_label_audit.py` | How often datagen's WDL labels contradict tablebase truth | C.2 |
| `diag/endgame_search_occurrence.py` | How often reference endgame families occur inside search trees | C.5 |
| `diag/endgame_drawn_bias.py` | Per-class evaluation bias on the drawn subset | BAS-E54 |
| `diag/kbnk_cohort.py`, `diag/kbnk_cohort_v1.epd`, `.manifest.json` | The 198 Syzygy KBN-K wins of BAS-E35's census, frozen and verified | C.5; BAS-E35 |
| `diag/kbnk_outcomes.py` | A KBN-K outcome census, classified and Syzygy-validated | BAS-E35, BAS-E36 |
| `diag/krppkrp_cohort_v1`, `.manifest.json` | The frozen KRPP-KRP cohort | C.5 |
| `diag/narrow_node_probe.py` | Whether the narrow-window rook failure is resolvable at all | BAS-E53 |
| `diag/rook_ending_failure_profile.py` | Why won rook endings are thrown, against a control | C.5 |
| `diag/passer_king_geometry.py` | Whether king-to-passer distance explains the pawn-family losses | C.5 |

## Evaluation data and fitting

| Tool | Purpose | Used by |
|---|---|---|
| `datagen.ps1` | A self-play PGN dataset for Texel fitting | PROCESS *Adjudication*; C.2 |
| `texel/` | The Texel pipeline: `extract.py` and `extract_parallel.py` (PGN to labelled FENs), `audit_starts.py`, `sample_fens.py`, `tuner.cpp`, `bake.py` (weights back into `src/eval_params.h`), `phase911.ps1` (the registered joint fit's launcher), and their tests. `import_beast.py` is obsolete and kept only as history | PROCESS *Texel procedure*; `texel/README.md` |

## Tests of the tools

Each `diag/test_*.py` or `diag/test_*.ps1` tests its namesake. Run a Python one
with `python tools/diag/test_<name>.py` and a PowerShell one with
`pwsh -NoProfile -File tools/diag/test_<name>.ps1`. CI's documents job runs
`test_release_check.py` and `test_verify_isa.py`, and
`check_roadmap.py --self-test` tests the roadmap checker itself.
