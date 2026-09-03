# Basilisk development plan

This is the maintainer-facing source of truth. [GUIDE.md](GUIDE.md) is its
short checklist mirror; [EXPERIMENTS.md](EXPERIMENTS.md) remains the detailed,
historical evidence ledger.

## 1. Current checkpoint

| Item | State |
|---|---|
| Branch | development |
| Released baseline | Basilisk 1.9.3; bench-13 fingerprint 11,941,440 |
| Current head | Bench 12,709,666; last recorded CTest 12/12; WAC 137/300 |
| Strength baseline | basilisk-5912-slim-pext-pgo; accepted HCE line about +12 Elo versus 1.9.3 |
| Current phase | Phase 6, step 6.1.c |
| Evaluation | HCE is unfrozen for structural improvement and complete, controlled refits |
| Corpus rule | Game-result labels only; no engine-evaluation labels |
| Match/data rule | Natural termination by default; score-based adjudication requires explicit opt-in and registration |
| Long job | 6.1.c upper-range safety sweep prepared; maintainer command pending |
| Release target | Classical release after Phase 8; NNUE 2.0.0 after Phase 10 |

Step 6.1.c remains open. Its corrected refinement proved that stronger
diagonal slope is useful but placed the best aggregate result on the tested
boundary and exposed a live truth loss there. A final safety-bounded upper
sweep is prepared; no later leaf may proceed until it is returned and analyzed.

## 2. Operating contract

- Work strictly in numbered order. A later step may be prepared, but may not
  change engine policy or consume experimental budget before its dependencies close.
- Commit each completed step with PLAN and GUIDE synchronized.
- Run python tools/diag/check_roadmap.py before committing either roadmap file.
- Consult and update EXPERIMENTS before retrying a mechanism.
- Preserve source, compiler, binary, book, corpus, split, seed, tablebase and
  command provenance. Hash immutable inputs and outputs.
- A behavior-neutral change needs the relevant static checks, CTest and exact
  bench. A playing change additionally needs its registered game gate.
- Strength tests use paired UHO openings and normalized Elo. Default SPRT is
  [0,3] nElo; broad/risky bundles use [-3,3]; simplification uses [-3,0].
- Score-based draw/resign adjudication is off by default in every tool. An
  opt-in compatibility run must record the exact adjudication policy and must
  never be mixed with natural-termination evidence.
- Do not run competing CPU-heavy work while a long tournament, tune, datagen
  or fit occupies the machine.
- Long jobs are run by the user after the model prepares and verifies the
  instrument. The model analyzes returned artifacts and applies the registered verdict.
- Reference engines teach mechanisms and experimental design. Reimplement in
  Basilisk's idiom and fit Basilisk's scale; never copy constants as acceptance evidence.
- Texel fits linear HCE coordinates. A bounded HCE SPSA prices only nonlinear,
  capped or contextual evaluation terms after the linear fit. Search SPSA runs
  only after the final HCE freezes; the later NNUE transition gets its own tune.

## 3. Required evidence

| Change | Minimum gate |
|---|---|
| Tool/docs/refactor | Syntax/static checks; focused tests; full tests/bench when execution semantics can change |
| Endgame evaluator | Deterministic truth corpus, paired WDL/DTZ evidence, conversion floors, tactical regression, SPRT |
| HCE refit | Frozen corpus/splits/surface, fixed K, untouched test set, source restore proof, clean PGO SPRT |
| Search change | Deterministic regression/telemetry, 1T STC SPRT, relevant LTC/4T confirmation |
| Time/root/SMP | 1T STC, 1T 10+0.1, 4T 10+0.1, zero forfeits, topology/hash recorded |
| Release | Reproducible PGO assets, correctness matrix, prior-release and external-cohort games |

Use tools/books/UHO_Lichess_4852_v1.epd for paired gates. SPRT decides strength;
node counts, traces, tactical suites, WDL/DTZ and fit loss explain the result.

## 4. Closed release phases

- [x] **1.0** Foundations and first strength line — 1.0.0 through 1.8.0.
  - [x] **1.0.a** Board, move generation, UCI, PVS/qsearch, TT, histories, SEE and Syzygy.
  - [x] **1.0.b** Time management, Lazy SMP, reproducible tests and accepted HCE.

- [x] **2.0** Correctness and search architecture — 1.9.0.
  - [x] **2.0.a** State, repetition/rule-50, TT/mate and SEE/pin correctness.
  - [x] **2.0.b** Staged ordering, correction/history, root-instability timing and dense TT.

- [x] **3.0** Hardening, CI and PGO speed — 1.9.1.
  - [x] **3.0.a** Centralized parameters, invariants, fuzzing, CI and telemetry.
  - [x] **3.0.b** Behavior-identical PGO speed pass accepted at +4.34% NPS.

- [x] **4.0** SMP durability and release tooling — 1.9.2/1.9.3.
  - [x] **4.0.a** SPSA/MT harness and helper clock/node/thread safety repaired.
  - [x] **4.0.b** Four-thread bundle accepted; PGO tool matching fixed without search change.

## 5. Completed development foundation

Everything complete remains here. Phase 6 is the first open engine phase, so no
completed item has a number after unfinished work.

- [x] **5.0** Reproduce the 1.9.3 baseline.
  - [x] **5.0.a** Freeze benchmark, test and compiler evidence.

- [x] **5.1** Measure search/evaluation authority.
  - [x] **5.1.a** Search oracle measured +322.7 +/-36 Elo.
  - [x] **5.1.b** HCE oracle measured +232.8 +/-32 Elo.

- [x] **5.2** Build differential diagnostics and inventory.
  - [x] **5.2.a** Add the 107-position diagnostic suite and diag-kv telemetry.
  - [x] **5.2.b** Split candidate work into dependency-complete clusters.

- [x] **5.3** Close search cluster A: ordering, histories and LMR.
  - [x] **5.3.a** Reject reduction magnitude on harness evidence.
  - [x] **5.3.b** Reject check-depth change by games.

- [x] **5.4** Close search cluster B: static-eval, TT and qsearch contracts.
  - [x] **5.4.a** Confirm existing contracts; accept no engine change.

- [x] **5.5** Close search cluster C: main selectivity.
  - [x] **5.5.a** Record the history-pruning defect and exhausted budget boundary.

- [x] **5.6** Close completed extension and root evidence.
  - [x] **5.6.a** Retain only mechanisms supported by completed gates.
  - [x] **5.6.b** Defer singular-extension depth and clock work to Phase 8.

- [x] **5.7** Audit shallow-depth node cost.
  - [x] **5.7.a** Measure rather than assume a width deficit.
  - [x] **5.7.b** Withdraw the target after the constant-factor diagnosis.

- [x] **5.8** Enlarge and freeze the HCE feature surface.
  - [x] **5.8.a** Add seven coverage terms.
  - [x] **5.8.b** Add bishop outpost and split king-protector structure.
  - [x] **5.8.c** Identify endgame technique as the remaining structural gap.

- [x] **5.9** Diagnose the first joint-fit failure.
  - [x] **5.9.a** Run the distilled-corpus refit.
  - [x] **5.9.b** Trace mate-drive loss to score-adjudicated corpus truncation.
  - [x] **5.9.c** Establish on-policy self-play WDL labels as the fit contract.

- [x] **5.10** Accept the repaired HCE line.
  - [x] **5.10.a** Accept king-safety refit.
  - [x] **5.10.b** Accept full-surface refit and freeze its baseline artifact.

- [x] **5.11** Remove redundant HCE terms.
  - [x] **5.11.a** Gate simplification without losing accepted strength.

- [x] **5.12** Inventory and improve endgames.
  - [x] **5.12.a** Inventory twenty reference endgame families.
  - [x] **5.12.b** Improve KBNK conversion from 13% to 54.5%.

- [x] **5.13** Add deterministic conversion floors.
  - [x] **5.13.a** Cover KQK, KRK, KBBK and KBNK with fixed-seed tests.
  - [x] **5.13.b** Record denominators and avoid treating tiny percentage changes as truth.

- [x] **5.14** Repair basic mate drive and diagnose KBNK residue.
  - [x] **5.14.a** Complete KXK/KBBK drive.
  - [x] **5.14.b** Classify KBNK failures: stalled drive, bishop-move ties and rule-50 loss.
  - [x] **5.14.c** Preserve the 198-position cohort for paired follow-up.

- [x] **5.15** Port the generalized endgame-truth instrument.
  - [x] **5.15.a** Support named <=6-man families with family-stable deterministic seeds.
  - [x] **5.15.b** Separate Syzygy WDL truth, WDL preservation, DTZ progress and conversion.
  - [x] **5.15.c** Emit per-position records and honest denominators for paired analysis.
  - [x] **5.15.d** Record engine identity/hash and prevent the tested engine from using Syzygy.

- [x] **5.16** Make no-adjudication the toolchain default.
  - [x] **5.16.a** Default SPRT, fixed gauntlet and datagen to natural termination.
  - [x] **5.16.b** Default weather-factory SPSA and its reinstall patch to natural termination.
  - [x] **5.16.c** Remove adjudication from all Colosseum strength, SPSA, tournament and datagen profiles.
  - [x] **5.16.d** Retain explicit opt-in only for registered legacy-compatibility runs.

- [x] **5.17** Synchronize the roadmap mechanically.
  - [x] **5.17.a** Reorder phases around endgame-first HCE development.
  - [x] **5.17.b** Add a PLAN/GUIDE checklist consistency checker.

## 6. Endgame maturity and truth

### 6.0 Evidence contract

- [x] **6.0** Establish the truth baseline before another evaluator edit.
  - [x] **6.0.a** Freeze 770 Syzygy-verified positions across 21 endgame families.
  - [x] **6.0.b** Measure the accepted Basilisk head and a strong reference at identical nodes.
  - [x] **6.0.c** Record attained family reference results; neither 100% nor the reference result is a finite-node ceiling.
  - [x] **6.0.d** Define paired confidence rules: aggregate beyond 2 SE reports, family beyond 3 SE blocks.
  - [x] **6.0.e** Add hard theory vetoes for clean-win discard, illegal play, crash and rule-50 regression.
  - [x] **6.0.f** Census disagreements between self-play WDL labels and Syzygy on every <=6-man corpus row.
  - [x] **6.0.g** Port Rarog's exact search-tree occurrence census for the 20 reference endgame families.

Step 6.0.a artifact: tools/diag/endgame_cohort_v1.epd and its manifest use
seed 0x4E9A2 and SHA-256
3CCF28EA3C8BC6C7E995BDA0BDD4833496B4C46758FF55C0B812E30E8AC6BF1B.
The 770 unique records contain 480 clean wins, 289 draws and one cursed win;
each carries family-stable seed, FEN, exact WDL and signed DTZ. The requested
24-clean-win/16-rule-draw family mix is left explicitly short where theory
does not supply that class. KRPP-KRP is seven men and remains outside the
available six-man tables.

Step 6.0.b used 60,000 nodes/move, one engine thread, 16 MB hash, a
100-ply diagnostic limit, disabled engine tablebases and no score adjudication.
All 770 IDs, FENs and theory labels paired exactly. Accepted head `294a3e2`
(binary SHA-256 D0E558F8A113CD9D17905B6EF701040CF5B0FB450FC058EFB9C49DF2329F8430)
converted 293/480 clean wins (61.04%); Stockfish
`dev-20260716-ebcea3ef` (binary SHA-256
91AE61DFCAEF1A5FDFEE9722EDE0591DA1FCB124D1DE7FBD44DD8786BD6531E3)
converted 389/480 (81.04%). The paired conversion matrix was 277 both, 16
Basilisk only, 112 reference only and 75 neither. Aggregate move-level
win-preservation was 98.08% versus 99.78%, and absolute-DTZ progress was
43.46% versus 56.41%. These variable-length move samples are descriptive and
autocorrelated; 6.0.c sets finite-budget ceilings and 6.0.d owns confidence.
The largest conversion deficits were KBP-K (6/24 versus 22/24), KQ-KR
(11/24 versus 24/24), KNN-KP (1/24 versus 14/24), KBN-K (12/24 versus 24/24)
and KQ-KRP (13/24 versus 23/24).

Step 6.0.c freezes `tools/diag/endgame_ceilings_v1.json`. Its historical field
name `attained_single_engine_ceiling` means only the best conversion count
observed in the two complete 60k runs. The accurate term is **attained reference
result**: it is neither a theoretical nor empirical upper bound, Basilisk may
surpass it, and failure to equal it is not itself a rejection. The paired union
is a stretch diagnostic only: it proves that each included position was
converted by at least one engine, not that one engine can convert the union.
A future accepted head may ratchet the recorded reference result upward through
another complete run under the identical cohort and search contract.
Conversion is not applicable to KNN-K because the frozen family contains no
clean theoretical wins. Move-level WDL/DTZ rates remain explanatory because
their variable game lengths make the samples dependent; 6.0.d owns their
comparison rule.

| Family | Clean wins | Accepted | Attained 60k reference | Paired union |
|---|---:|---:|---:|---:|
| KQ-K | 24 | 24 | 24 (100%) | 24 |
| KR-K | 24 | 24 | 24 (100%) | 24 |
| KBB-K | 24 | 24 | 24 (100%) | 24 |
| KBN-K | 24 | 12 | 24 (100%) | 24 |
| KNN-K | 0 | n/a | n/a | n/a |
| KP-K | 24 | 24 | 24 (100%) | 24 |
| KPP-K | 24 | 10 | 12 (50.0%) | 13 |
| KBP-K | 24 | 6 | 22 (91.7%) | 23 |
| KR-KP | 24 | 23 | 24 (100%) | 24 |
| KR-KB | 24 | 21 | 24 (100%) | 24 |
| KR-KN | 24 | 19 | 23 (95.8%) | 23 |
| KQ-KP | 24 | 24 | 24 (100%) | 24 |
| KQ-KR | 24 | 11 | 24 (100%) | 24 |
| KNN-KP | 24 | 1 | 14 (58.3%) | 15 |
| KRP-KR | 24 | 3 | 9 (37.5%) | 10 |
| KRP-KB | 24 | 5 | 9 (37.5%) | 12 |
| KBP-KB | 24 | 7 | 16 (66.7%) | 18 |
| KBP-KN | 24 | 13 | 15 (62.5%) | 19 |
| KP-KP | 24 | 24 | 24 (100%) | 24 |
| KQ-KRP | 24 | 13 | 23 (95.8%) | 23 |
| KBPP-KB | 24 | 5 | 6 (25.0%) | 9 |

Across all families the attained reference result is 389/480 (81.04%); the non-additive
paired union is 405/480 (84.38%). `tools/diag/endgame_ceilings.py` validates
the source schemas, hashes, identical contract and exact ID/FEN/theory pairing
before reproducing the artifact, so neither number can silently mix cohorts.
The frozen artifact SHA-256 is
19E43A2E7EEF9069E1EE8575ABF0E622BDF97887557A64023ED15F8C7E46508D.

Step 6.0.d is implemented by `tools/diag/endgame_compare.py`. The independent
unit is one paired frozen position, not one engine move. Conversion is binary;
win-preservation and DTZ progress are first reduced to one rate per position,
giving long games no artificial extra weight. A clean-win position whose
candidate discards before any DTZ comparison receives zero DTZ progress rather
than disappearing from the sample. Aggregate movement at or beyond 2 SE is
reported and must be explained but is not by itself a veto. Within a family,
movement at or beyond 2 SE is reported and a regression at or beyond 3 SE
blocks the candidate. Improvements never establish strength or bypass SPRT;
they nominate a later accepted result for ceiling/floor ratcheting. Invalid or
unpaired reports fail closed, while the absolute theory rules remain 6.0.e's
separate responsibility.

Step 6.0.e is implemented by `tools/diag/endgame_vetoes.py` and fail-closed
reporting in `endgame_truth.py`. Engine crash/error, illegal move and a no-move
response from a nonterminal position are absolute vetoes anywhere in the
cohort. Existing accepted-head theory debt is grandfathered, but a candidate
may not introduce a clean-win discard or rule-50 failure on a clean-win
position where the accepted baseline avoided it. These position-level vetoes
are objective trajectory regressions and cannot be traded against aggregate
improvement or a positive SPRT. The accepted report passes itself with zero
vetoes. As an intentional independence check, the stronger reference is not an
acceptable Basilisk candidate under this contract: despite its aggregate gain,
it newly discards clean wins at KNN-KP EG0459/EG0460 and newly reaches rule 50
at EG0464. This confirms that the ceiling oracle and the candidate correctness
gate answer different questions.

Step 6.0.f is implemented by `tools/diag/endgame_label_census.py` and freezes
`tools/diag/endgame_label_census_v1.json`. It scanned all 1,052,632 rows of the
accepted Arm C corpus (1,000,000 train and 52,632 holdout), verified exact
`0/0.5/1` White-perspective labels, and probed every one of the 173,750 rows
with at most six pieces. There were 24,530 self-play/Syzygy disagreements
(14.12%): 22,392 are self-play draws in tablebase-decisive positions, 2,128
are decisive game labels on tablebase draws, and only 10 reverse the decisive
winner. KBN-K alone contributes 6,117/6,547 disagreements and K-KBN contributes
4,778/5,248; together, all 10,895 disagreements are natural-termination draws
where Arm C failed to convert the theoretical KBNK win. This directly supports
doing 6.1 before the next full HCE refit. Syzygy's seven cursed and fourteen
blessed rows remain draws in the label domain. The census deliberately uses
zeroing-clock WDL and does not authorize Phase 7.4 relabeling without its
separate halfmove-clock and row-domain analysis. Corpus hashes and parent
no-adjudication PGN provenance are embedded in the artifact; its SHA-256 is
609E60489838F6708ADF83D851B8CDB5D1E963102A2C599879ECC9C9F2E5CB46.

Step 6.0.g adds tune-build-only exact counters at the start of every full
evaluation and `tools/diag/endgame_search_occurrence.py` to aggregate a fixed
suite at fixed depth. The runner defaults to 30 independent workers on this
32-thread machine, while every engine remains `Threads=1`; worker count changes
wall time, not the deterministic tree. The report records engine and suite
hashes, nodes, full evaluations and the <=7-man denominator. Counts are a
candidate-priority screen, never an Elo gate. KPK overlaps the KPsK aggregate
and KBNK overlaps KXK. Unlike the source instrument, Basilisk's KXK counter
requires actual mating material: queen, rook, bishop+knight, or a bishop pair
tested by SQUARE COLOUR rather than by count, matching `apply_endgame`'s own
KXK gate. Dead KBK, KNK and same-coloured KBBK are therefore excluded. Queen
and rook stay in the family census even though the engine deliberately leaves
their search-solved mates unoverridden (BAS-E30, BAS-E34): the instrument
measures family frequency, not `kxk_score` firings.

### 6.1 Complete KBNK mate drive

- [x] **6.1** Implement and tune the missing KBNK technique (historical step 5.9.22).
  - [x] **6.1.a** Start from Rarog's useful finding: bishop-color corner diagonal potential can solve the drive without a bishop-position term.
  - [x] **6.1.b** Make the bishop-colour diagonal mechanism explicit and prove the existing Manhattan form was algebraically identical.
  - [x] **6.1.c** Scale coefficients to Basilisk and test the required diagonal dominance against its existing edge, king-distance and knight-distance terms.
  - [x] **6.1.d** Do not retry bishop proximity or escape-square count unless new evidence overturns their earlier failure.
  - [x] **6.1.e** Compare on all 198 positions, with positions 61-198 as the held-out confirmation set; report WDL preservation, rule-50 failures, conversion and mate efficiency.
  - [x] **6.1.f** Require KQK/KRK/KBBK non-regression, tactical stability and bench accounting; exact bench identity is necessary but cannot prove this path-dependent evaluation change behaviorally neutral.

Step 6.1.a is frozen in `analysis/kbnk_diagonal_port_v1.md`. Rarog commit
`4aea0c7` replaced a coarse corner drive with a weak-king diagonal potential
selected only by the winning bishop's square colour. At 60k nodes it moved
KBN-K conversion from 19.4% to 96.9% and eliminated 61 rule-50 failures; the
successful sweep also showed that the corner pull must dominate the king pull.
For Basilisk, a dark-squared bishop uses `abs(7-rank-file)` toward a1/h8 and a
light-squared bishop uses `abs(rank-file)` toward a8/h1. Basilisk's existing
exact-material dispatcher already supplies the required bishop colour, weak
king square, score sign and narrow activation. Therefore 6.1.b owns only the
geometry port; 6.1.c owns Basilisk-scale constants and interaction with the
current edge, king and knight pulls. No bishop-position term is licensed.

Step 6.1.b found that the proposed geometry was already present. For the a1/h8
corner complex, Basilisk's existing `14 - min(Manhattan)` is algebraically
`7 + abs(7-rank-file)`; for a8/h1 it is `7 + abs(rank-file)`. The constant seven
cannot affect ordering. `kbnk_score()` now spells those diagonal formulas
directly while retaining the constant and Basilisk's existing weight, making
the rewrite score- and bench-identical. Both bishop-colour orientations have
focused regression coverage. Consequently no new geometry candidate exists:
6.1.c owns the remaining Rarog finding—scale and dominance relative to the
edge, friendly-king and knight terms. Release `test_eval` and `test_endgames`
pass, and `bench 13` remains exactly 12,709,666 nodes. No conversion gain is
claimed because the score is unchanged.

Step 6.1.c used a tune-build-only atomic `KBNK Drive` option in
`diagonal,edge,king,knight` order. The option rejects malformed, negative and
combined vectors whose largest legal KBNK score would enter the mate-score
band. The release UCI remains clean. The historical BAS-E35 LCG source is
frozen as `tools/diag/kbnk_cohort_v1.*`: exactly 198 Syzygy clean wins retained
from the original 200 generated positions, with source indices, WDL/DTZ labels
and tablebase inventory provenance.

The registered screen runs baseline, diagonal 600/1000, each competing-term
ablation, edge+knight ablation, and three increasingly Rarog-shaped dominant
diagonal vectors. It uses the first 60 frozen positions, 60,000 nodes/move,
100 plies, 30 independent one-thread workers, persistent TT only within each
game, engine tablebases disabled, and natural termination with no score
adjudication. All variants use the same tune binary and positions. Reject any
engine/protocol anomaly; then rank conversion on the paired positions, using
clean-win preservation, DTZ progress and mate efficiency diagnostically and
preferring the simpler vector when practically tied. The two leaders both
converted 31/60 versus baseline 26/60. `dominant-diagonal` (`1000,0,220,0`)
won the tie: it discarded only 3 clean wins versus 12 for
`diagonal-1000` (`1000,900,220,220`), had no hard anomaly, raised clean-win
move preservation from 99.5800% to 99.8676%, reduced stalemates 7 to 2, and
removed the edge and knight pulls. Its 15 paired gains and 10 losses made it a
promising first-pass bundle rather than proof; the correction below supersedes
its provisional completion. Summary SHA-256:
`ED0A554855D9B61273E968EDF73A5FBEE96046AA5E8F5D0DBCC1616A77FAAEDF`.

**6.1.c correction (2026-09-03):** that first-pass comparison did not isolate
the claimed mechanism. The implementation was `(7 + diagonal) * weight`, so
800 -> 1000 strengthened the position-dependent slope but also raised every
KBNK score by 1,400. Absolute score changes can alter pruning and search even
though they do not alter static move ordering inside the class. The selected
vector also removed edge and knight pulls, so its 31/60 cannot be attributed
to diagonal dominance alone. The completion was revoked before 6.1.e.

The corrected parameterization is `base + diagonal*slope + edge*E +
king-distance*K + knight-distance*N`, controlled atomically in tune builds as
`base,diagonal,edge,king,knight`. Its current defaults reproduce the provisional
first-pass score exactly, so the refactor itself is behavior-neutral. The old
four-field form remains accepted and maps to `base = 10000 + 7*diagonal`,
allowing the original artifacts and script to remain reproducible.

The registered refinement first requires the exact old baseline
`15600,800,900,220,220` and first-pass winner `17000,1000,0,220,0` to reproduce
their earlier paired results. It then holds base at 15,600 while testing a
3-by-3 diagonal/king grid, adds a lower diagonal boundary, and independently
tests base 14,200/17,000/18,400 at the centre vector. All other cohort, node,
ply, worker, tablebase and no-adjudication conditions remain identical. Any
control drift or hard anomaly invalidates the run. Otherwise rank paired
conversion first, clean-win discards second, and use WDL preservation, DTZ
progress and mate efficiency diagnostically. Because these 60 positions select
among 15 arms, they are development data, not confirmation: 6.1.e must make
positions 61-198 its primary held-out verdict and report the all-198 aggregate
only as secondary context. The refinement completed with summary SHA-256
`FAF6D1CC1A4AB43AF67DAD4D97B9CA2E247AC65F36807A3260C3D3F55814F5FA`.
Both behavior-neutral controls reproduced their original per-position
fingerprints exactly.

The result rose strongly through the diagonal boundary: legacy converted
26/60; the best 1250 arm converted 39/60; and diagonal 1450 converted 42, 46,
and 47 with king weights 140, 220, and 300. The independent base axis at
diagonal 1250/king 220 converted 35, 36, 31, and 36 at bases 14200, 15600,
17000, and 18400, so base-score inflation neither explains nor monotonically
drives the gain. However, all three diagonal-1450 arms discarded the clean win
at ply zero on `KBNK0039`; the 220 and 300 arms then lost a piece at ply four.
They are correctness failures regardless of aggregate conversion. The current
provisional default therefore remains only a control, not an accepted choice.

The final upper-range screen retained exact legacy and 1450/300 fingerprints,
tested diagonal 1350-1900 against king weights 340/460 where mate-band safety
allowed, and admitted no candidate with a new truth discard before ply 80. A
one-position 60k preflight excluded combinations already known to discard
`KBNK0039`; diagonal 2000 also fails that gate, so 1900 plus the static
mate-score bound closed the useful upper search interval.

Both registered controls reproduced exactly: legacy converted 26/60 at
99.5800% clean-win preservation with seven stalemates, and the 1450/300
boundary control converted 47/60 while still discarding `KBNK0039` at ply
zero into `insufficient_material`. The run is therefore valid. Its engine was
built from `12b40b6`, before the pending 6.0.g diagnostic landed; engine
SHA-256 `34AB7B682425020D8BBD97C80924BF2140119D127A57532811D09FDC5D2963B8`
and summary SHA-256
`12D7B76C69662C1B91A01AAE02E4CD6FEAD4792C30DB73762EA083FE7E6D4B59`.

The truth veto removed exactly two arms, and no arm produced a hard anomaly:
the 1450/300 control (`KBNK0039`, ply 0) and candidate `d1650-k460`
(`KBNK0023`, ply 6). Ranking the survivors by paired conversion selects
`15600,1750,0,340,0`, now the provisional default. It converted 51/60 against
the legacy 26/60 for 27 paired gains and 2 losses, cut discarded clean wins
from 10 to 3, raised clean-win preservation to 99.8448%, posted the best
admissible DTZ progress at 0.6192, reduced stalemates from 7 to 1 and
fifty-move draws from 27 to 8 -- the next best arm reaches only 14 -- and
solved `KBNK0039` in 43 plies, faster than any other variant.

Registered design caveat carried into 6.1.e: the winner is separated from the
legacy baseline overwhelmingly (paired z approximately +4.6) but is not
separated from its own plateau. Against `d1750-k460`, `d1850-k340` and
`d1900-k460` the paired z values are only +1.61, +1.70 and +1.21, and the
king-340 row is non-monotone across diagonal 1350-1900 (37, 33, 41, 51, 44,
40). These 60 positions have now selected across three rounds and 42 arms, so
51/60 is a winner's-curse point estimate and the held-out result should be
expected to fall. The defensible claim from 6.1.c is that the
diagonal-dominant region as a whole beats the legacy vector decisively, not
that (1750, 340) is its optimum. Accordingly 6.1.e must treat positions
61-198 as its primary verdict, and should carry `15600,1900,0,460,0` (46/60,
2 discards, 99.8987% preservation) as a second arm so that a noisy peak can
be distinguished from the true plateau level.

Step 6.1.d records the closure rather than implementing anything. BAS-E36
diagnosed the KBNK failures as stuck rather than slow and traced the cause
correctly: nothing in `kbnk_score` depends on the bishop, so every bishop move
scores alike and the potential has a flat maximum that is not mate. The
inference drawn at the time was to add a bishop-dependent term, and both
attempts failed. Bishop proximity (weight 300) moved conversion 94 to 88,
inside one SE, but took piece loss before ply 10 from zero to two: a long-range
piece parked beside a bare king gets captured. Escape-square count (weight 400)
dropped KBN-K conversion from 14/16 to 9/16, roughly 3.8 SE, failing the
deterministic CTest floor.

BAS-E39 has since removed the motivation as well as leaving the refutations
standing. The flat-maximum diagnosis was right, but the remedy was never a
bishop term: steepening the diagonal gradient and deleting the competing edge
and knight pulls fixed most of the stuckness with the bishop still absent from
the potential, cutting fifty-move draws from 27/60 to 8/60 and stalemates from
7 to 1. A bishop feature would now pay its known costs against a much smaller
residual. Both mechanisms are entered in the EXPERIMENTS.md retry map with
narrow triggers: proximity only if an instrument against the accepted vector
still shows bishop shuffling dominant, gated on zero clean-win loss and zero
piece loss before ply 10; escape-square count only if stalemate adjacency is
tested directly, the term is shown to reinforce rather than compete with the
corner drive, and it clears the KBN-K floor it broke. Neither is licensed for
6.1.e or 6.1.f.


Step 6.1.e runs the complete frozen 198-position cohort once per arm and
decides on positions 61-198 only. Three arms: the legacy control
`15600,800,900,220,220`, the 6.1.c selection `15600,1750,0,340,0`, and the
plateau probe `15600,1900,0,460,0` carried forward because BAS-E39 could not
separate them. Conditions are identical to the screens it confirms: 60,000
nodes/move, 100-ply limit, 16 MB hash, 30 independent one-thread workers,
engine tablebases disabled, natural termination, no score adjudication.

Two preparation measurements license the design. First, batch size does not
perturb per-position results: the first 60 records of a 198-position legacy
run reproduce the published BAS-E39 legacy fingerprint
`A28D2843263A8DDCB760C1133D5F105AE08E1F110AE4BB410210C683A15F1BF8`
byte-for-byte, so one run can be split into development and held-out halves
without changing what the development half means. Second, two independent
198-position legacy runs produced the identical fingerprint
`8CCB2C20380879C2025B017A661167CC60309A469C996F0C69F545D985CFD539`, so the
instrument is deterministic at this worker count. Both are hard gates in
`tools/diag/kbnk_holdout_summary.py`; either mismatch voids the run before any
candidate may be read. The drift gate was verified to fire on a perturbed
control, and the truth veto to override a large positive conversion result.

Disclosed pre-exposure: those preparation runs were the legacy control, whose
all-198 conversion is 95/198 at 0.9958 win preservation, giving a held-out
baseline of 69/138. The control is not a candidate and the verdict rule below
was registered before any candidate arm was run.

Pre-registered verdict. Reject any arm with a hard anomaly, or with a live
truth discard before ply 80 on either split, regardless of conversion. The
primary verdict is paired conversion against the legacy control on held-out
positions 61-198, accepted at McNemar z >= 2.0. Two surviving candidates count
as separated only at |z| >= 2.0 between them; otherwise prefer fewer discarded
clean wins, then higher DTZ progress, then the simpler vector. Positions 1-60
are reported solely to quantify shrinkage from the selection estimate and can
never rescue a failed held-out verdict. The all-198 aggregate, rule-50
failures, stalemates, WDL preservation and mate efficiency are diagnostic
context, not the verdict. Expect roughly four minutes at 30 workers. Run:
`powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\diag\run_kbnk_holdout.ps1`.


Step 6.1.e result: the held-out set rejected the 6.1.c winner and confirmed
the plateau probe, so the accepted vector is now `15600,1900,0,460,0`. Both
control fingerprints reproduced. On held-out `KBNK0061`
(`8/8/8/2K5/8/3k4/8/N1B5 w`, clean win at DTZ 52) the 6.1.c winner plays
1.Kd5 Ke2 2.Nc2, which Syzygy scores from +2 to 0; Black replies 2...Kd1
forking the undefended bishop on c1 and knight on c2, and after 3.Ke4 Kxc1 the
game is K+N versus K by ply 5. That is a correctness veto and outranks its
higher raw conversion of 103/138. The accepted vector plays 1.Nb3 and converts,
scoring 98/138 held out for 42 paired gains against 13 losses at z +3.91, with
no live discard anywhere in the 198.

The cause is the flat-maximum pathology BAS-E36 named for the bishop, now
visible for the knight: with edge and knight weights zero, no term in
`kbnk_score` refers to the knight, so all knight moves score alike and the
search tiebreak can pick a losing one. Steepening the diagonal does not remove
that degree of freedom. It is not licensed for repair inside 6.1 — 6.1.d closed
the bishop analogue and any knight-referencing term is a new mechanism needing
its own registration.

The registered caveat held in both directions. Head-to-head on held-out rows
the candidates are 24/19 discordant at z +0.76, statistically
indistinguishable, which is the plateau BAS-E39 predicted; and shrinkage was
real, 85.0% to 74.6% for the rejected arm and 76.7% to 71.0% for the accepted
one. Carrying the second arm is the only reason 6.1.e ends with a replacement
rather than an empty result. Diagnostics across all 198: conversion 95 to 144,
rule-50 draws 87 to 53, stalemates 16 to 1, clean-win preservation 99.5750% to
99.8467%, DTZ progress 0.4897 to 0.5880, discarded clean wins 32 to 10 with the
earliest at ply 96. Full record in EXPERIMENTS.md as BAS-E41; summary SHA-256
`FD09E1E1E872144E7476DE045D65A439C4F17D69496340F49B371EF0CCC437A1`.

Two limits carried into 6.1.f and 6.2. Conversion on a truth cohort is not Elo.
And the accepted vector's largest static KBNK score is 31,660 against the
31,872 mate-band floor, only 212 points of headroom, so the diagonal and king
weights are now effectively bounded from above by that check rather than by
evidence.


Step 6.1.f closes 6.1 with acceptance accounting rather than new games.
Non-regression is exact rather than merely absent: KQ-K, KR-K and KBB-K at 100
positions each, seed 424242, produce byte-identical per-position records under
the accepted vector, the legacy vector and the rejected 6.1.c winner — KQ-K
98/100 at 99.91% preservation, KR-K 97/100 at 99.93%, KBB-K 99/100 at 99.93%.
`g_kbnk_drive` feeds only `kbnk_score`, which the dispatcher reaches only for
bishop and knight against a bare king, so those families cannot see the change;
the run confirms that rather than assuming it. WAC at depth 12 is likewise
identical across all three vectors at 244/300 and 83,444,716 nodes with the
same failure list.

The mate-band bound was checked against the search rather than re-read from the
validator. Every mate comparison in the engine — TT store and probe adjustment,
null-move verification, razoring, futility, singular extension and root
reporting — uses `MATE_SCORE - MAX_PLY`, with no lower threshold anywhere, so
32000 - 128 = 31872 is exactly `STATIC_MATE_FLOOR`. The accepted vector's
largest legal static score is 31,660, leaving 212 points of headroom, and a new
test pins the boundary behaviourally: `15600,1900,0,495,0` reaches 31,870 and is
accepted while `15600,1900,0,496,0` reaches 31,876 and is rejected, so a future
change to `MATE_SCORE` or `MAX_PLY` fires a test instead of silently letting an
evaluation be stored as a mate score.

Bench accounting: `bench 13` is 12,709,666 nodes at geomean EBF 2.876, identical
to the pre-6.1 head, and it proves nothing here because the bench suite contains
no KBNK position. The behavioural evidence is BAS-E41's held-out cohort. Full
CTest 12/12, test_eval 112/112, test_endgames 47/47, and the release default
plays 1.Nb3 on `KBNK0061`. Recorded as BAS-E42. Conversion on a tablebase cohort
is still not Elo; 6.2 owns that.

One residual defect is documented and deliberately not repaired inside 6.1. The
accepted vector sets edge and knight weights to zero, so no term in
`kbnk_score` refers to the knight and all knight moves score alike, which is the
flat-maximum pathology BAS-E36 named for the bishop. It cost the rejected
candidate a clean win on `KBNK0061`. The accepted vector happens to break that
tie safely on all 198 rows, but nothing makes it do so. A knight-referencing
term is a new mechanism needing its own registration, and 6.1.d closed the
bishop analogue on evidence, so it does not belong to this step.


### 6.2 Gate endgame Group A

- [ ] **6.2** Gate KBNK and accepted mate-drive changes.
  - [ ] **6.2.a** Run a fresh no-adjudication [0,3] nElo SPRT against the accepted head.
  - [ ] **6.2.b** Treat the old approximately 5,860-game adjudicated Group A run as preliminary only.
  - [ ] **6.2.c** Never resume that run if the KBNK candidate or match policy changes.

Step 6.2 expectation, revised on evidence before the run (BAS-E43). The 6.0.g
census was executed for the first time at depth 13 on `suite_v1.epd`. Its
headline number is an artifact and must not be quoted: 21 of that suite's 107
roots are already seven men or fewer, and those roots are themselves bare-king
endgames, so they generated essentially every KBNK evaluation. Restricted to
the 86 roots with eight or more men, KBNK occurs **zero times** in 30.5M nodes
and 14.1M evaluations, and the whole <=7-man band is 0.283% of evaluations.

KBN-K therefore cannot reach the score sheet through search-tree influence; it
pays only when the position is actually reached on the board, which is rare.
**6.2 is a non-regression gate, not a strength gate.** The pre-registered
expectation is that a [0,3] nElo test accepts H0, and H0 acceptance here means
"no measurable gain", not "the change is harmful"; it must not be read as a
rejection of 6.1, whose evidence is BAS-E41's held-out truth cohort. Retention
is licensed by that cohort plus the absence of a strength regression, so if the
[0,3] run accepts H0 the follow-up that matters is a non-regression bound, not
another attempt at a gain.

Because the bounds question is a plan-structure change it is left to the
maintainer rather than altered here. `tools/sprt.ps1 -Mode gainer -Elo1 3` runs
the registered [0,3] test; `-Mode simplify` runs the [-5,0] non-regression test
the revised purpose actually calls for.

Step 6.2.a is prepared as a single-variable A/B that isolates 6.1. The baseline
is commit `6accfe6` (Complete 6.1.b), the last revision whose KBNK scoring is
the pre-6.1 one -- 6.1.b was proven score- and bench-identical, so everything
outside `kbnk_score` matches the candidate exactly. That deliberately differs
from the superseded Group A run's `5.9.18-base`: 6.2.b already declares that
approximately 5,860-game adjudicated run preliminary, and re-running a stale
multi-change comparison would not isolate what 6.2 gates. Adjudication is off,
as the default requires.

Harness note found during preparation: `tools/sprt.ps1` and
`tools/build_test.ps1` are UTF-8 without a BOM and contain non-ASCII em-dashes,
which Windows PowerShell 5.1 mis-decodes into unterminated string literals, so
both fail to parse there. They run correctly under `pwsh` 7. Every prepared
command therefore names `pwsh` explicitly.


### 6.3 Passed-pawn king approach

- [ ] **6.3** Add general king-to-passed-pawn approach logic.
  - [ ] **6.3.a** Derive the feature from Basilisk truth failures, not reference constants.
  - [ ] **6.3.b** Verify KP-K, KPP-K, KBP-K and mixed rook/minor pawn families.
  - [ ] **6.3.c** Gate the isolated candidate with no adjudication.

### 6.4 Magnitude and coverage audit

- [ ] **6.4** Audit every endgame term before broadening the evaluator.
  - [ ] **6.4.a** Test score resolution, saturation and interaction at Basilisk's scale.
  - [ ] **6.4.b** Keep theory truth, move quality, conversion and game strength separate.
  - [ ] **6.4.c** Freeze the accepted Group A head and truth report.

### 6.5 Group B endgames

- [ ] **6.5** Implement high-value rook and bishop-pawn families.
  - [ ] **6.5.a** Cover KRPP-KRP and KRP-KR.
  - [ ] **6.5.b** Cover KR-KP, KQ-KRP and KR-KB.
  - [ ] **6.5.c** Cover bishop-pawn families, including wrong-bishop/rook-pawn draw logic.
  - [ ] **6.5.d** Add deterministic truth cases before coefficient fitting.

- [ ] **6.6** Gate Group B.
  - [ ] **6.6.a** Require paired truth improvement and no family veto.
  - [ ] **6.6.b** Run no-adjudication SPRT on the frozen Group A baseline.

### 6.7 Group C and closure

- [ ] **6.7** Evaluate remaining lower-yield families.
  - [ ] **6.7.a** Cover KPs-K, KQ-KP, KR-KN, KQ-KR, KP-KP and KNN-KP.
  - [ ] **6.7.b** Implement only mechanisms with a measurable truth gap and plausible game frequency.
  - [ ] **6.7.c** Stop the group when marginal value no longer pays for complexity.

- [ ] **6.8** Close endgame maturity.
  - [ ] **6.8.a** Freeze the accepted evaluator, truth corpus, reports and thresholds.
  - [ ] **6.8.b** Record every rejected mechanism and its retry trigger.
  - [ ] **6.8.c** Authorize post-endgame corpus generation only after closure.

## 7. Post-endgame corpus and complete HCE refit

This phase mirrors the useful experimental shape of Rarog step 4.10 while
correcting its unresolved ambiguity: post-hoc tablebase relabeling and
datagen-v3 game adjudication are distinct arms and must not be conflated.
The target is a mature final classical evaluator, not merely another fit.

### 7.0 HCE maturity and feature-completeness audit

- [ ] **7.0** Define and close the final handcrafted-evaluation surface.
  - [ ] **7.0.a** Compare Basilisk conceptually with strong maintained HCE engines in D:/code; learn coverage and interactions without copying code or constants.
  - [ ] **7.0.b** Audit material/imbalance, PST, mobility, pawn structure, passers, outposts, threats, space, king safety, initiative/winnability and draw scaling.
  - [ ] **7.0.c** Measure feature firing, phase/material coverage, correlation and ablation value on a phase-balanced corpus.
  - [ ] **7.0.d** Identify dead, duplicate, saturated and uncovered terms; simplify or add mechanisms only with position-level evidence.
  - [ ] **7.0.e** Add deterministic tests for every new categorical mechanism and freeze the architecture before production datagen.

### 7.1 Fit-tooling contract

- [ ] **7.1** Harden the fit pipeline before generating expensive data.
  - [ ] **7.1.a** Fit K once on training data and freeze it across all compared fits.
  - [ ] **7.1.b** Accept an explicit initial vector and record every surface coordinate.
  - [ ] **7.1.c** Freeze train/validation/test splits; open the test set once after selection.
  - [ ] **7.1.d** Enforce exact surface coverage, gauge anchors and source restore on failure.
  - [ ] **7.1.e** Hash corpora, splits, configs, binaries, tablebases, fitted vectors and reports.
  - [ ] **7.1.f** Enforce the row-label domain exactly as 0, 0.5 or 1; publish counts plus every rejected-row reason before fitting.
  - [ ] **7.1.g** Give materially different corpus semantics a new versioned contract; never make an old contract appear compatible by silently widening its gates.

### 7.2 Corpus design

- [ ] **7.2** Design a phase-efficient, natural-termination corpus.
  - [ ] **7.2.a** Locate the actual source position store under D:/chess before relying on it; record its canonical path, format, row count, duplicate rate, material-phase distribution and content hash rather than importing an unverified Rarog path.
  - [ ] **7.2.b** Define phase buckets from Basilisk's evaluator/material phase, never nominal game ply; pilot a start book and measure the phase-yield matrix on extracted rows so randomized preflight games cannot masquerade as opening coverage.
  - [ ] **7.2.c** Freeze one extractor contract across every arm, including skip_start, max_per_game, sampling, deduplication, row filters, ordering and split seed; document any intentional difference from Rarog defaults.
  - [ ] **7.2.d** Derive the initial corpus target from effective rows per identifiable tunable coordinate and label quality, then confirm adequacy with a held-out learning curve instead of treating a raw row count as sufficient.
  - [ ] **7.2.e** Register extracted-row phase/material targets, duplicate and rejection ceilings, learning-curve stop conditions and maximum generation budget before production launch.

### 7.3 Generate and freeze the source corpus

- [ ] **7.3** Generate self-play with the accepted post-endgame head.
  - [ ] **7.3.a** Use no adjudication and game-result WDL labels.
  - [ ] **7.3.b** Verify termination mix, duplicate rate, phase coverage and <=6-man yield.
  - [ ] **7.3.c** Freeze corpus A, its row order and hashes before any relabeling.

### 7.4 Build matched label arms

- [ ] **7.4** Create the tablebase-relabel comparison.
  - [ ] **7.4.a** Corpus A keeps original self-play game-result labels.
  - [ ] **7.4.b** Corpus B is a byte-order-preserving copy except eligible <=6-man rows receive Syzygy truth labels.
  - [ ] **7.4.c** Treat cursed wins/losses as draws for rule-50-compatible WDL labels.
  - [ ] **7.4.d** Preserve identical rows, ordering and train/validation/test membership.
  - [ ] **7.4.e** At execution time analyze exactly which positions may be relabeled; never propagate an ending verdict backward into non-tablebase rows without a separately justified rule.
  - [ ] **7.4.f** Publish changed-row count, fraction, family distribution and before/after label matrix.

### 7.5 Analyze datagen-v3 separately

- [ ] **7.5** Decide whether datagen-v3 deserves a third arm.
  - [ ] **7.5.a** Inspect its semantics and provenance when this step is reached.
  - [ ] **7.5.b** Distinguish whole-game tablebase adjudication from row-local post-hoc relabeling.
  - [ ] **7.5.c** Pilot corpus C only if it can be matched closely enough for causal comparison.
  - [ ] **7.5.d** Never merge corpus C evidence into the registered A-versus-B verdict.

### 7.6 Initialization control

- [ ] **7.6** Measure optimizer dependence before the production fit.
  - [ ] **7.6.a** Fit identical targets from accepted-head and neutral initial vectors.
  - [ ] **7.6.b** Compare validation convergence, parameter distance and held-out loss.
  - [ ] **7.6.c** Register the production initialization rule before opening the test set.

### 7.7 Complete matched fits

- [ ] **7.7** Refit every relevant Texel-tunable HCE coordinate.
  - [ ] **7.7.a** Use the same complete surface, fixed K, optimizer budget and initial rule for A and B.
  - [ ] **7.7.b** Alternate nonlinear blocks where joint fitting is not valid.
  - [ ] **7.7.c** Produce independently applicable candidate vectors and exact manifests.
  - [ ] **7.7.d** Reject any fit with missing/frozen-by-accident coordinates or source drift.

### 7.8 Registered label-contract gate

- [ ] **7.8** Test whether tablebase relabeling transfers.
  - [ ] **7.8.a** Compare each candidate with the same accepted pre-fit baseline.
  - [ ] **7.8.b** Run the pre-registered A-versus-B no-adjudication gate.
  - [ ] **7.8.c** Use truth reports to explain endgame effects; use SPRT for strength.
  - [ ] **7.8.d** Accept the label policy and vector only by the registered rule.

### 7.9 Iterative refresh

- [ ] **7.9** Refresh data from the accepted fitted head.
  - [ ] **7.9.a** Generate a new no-adjudication corpus from the accepted engine.
  - [ ] **7.9.b** Reapply the accepted label contract and complete-surface fit.
  - [ ] **7.9.c** Repeat only while each cycle passes its independent gate.
  - [ ] **7.9.d** Stop at the first rejected cycle; never average rejected vectors into the head.

### 7.10 Nonlinear HCE tuning

- [ ] **7.10** Tune evaluation terms that Texel cannot price correctly.
  - [ ] **7.10.a** Inventory nonlinear, capped, thresholded and contextual terms after the accepted linear fit.
  - [ ] **7.10.b** Include only live, sufficiently frequent coordinates; likely candidates include the king-danger funnel and validated contextual scaling.
  - [ ] **7.10.c** Exclude sparse recognizer switches, exact endgame truth rules and every linear coordinate already handled by Texel.
  - [ ] **7.10.d** Wire only the selected coordinates as bounded tune options, generate configuration from current defaults and verify perturbation visibility.
  - [ ] **7.10.e** Run natural-termination SPSA and accept its clean PGO candidate only through an independent SPRT and truth/correctness gates.

### 7.11 HCE closure

- [ ] **7.11** Freeze the classical evaluator.
  - [ ] **7.11.a** Revalidate score scale, calibration, tactical suites and all endgame floors.
  - [ ] **7.11.b** Ablate new mechanisms and low-information fitted coordinates.
  - [ ] **7.11.c** Compare held-out loss, truth quality and game strength against the pre-Phase-7 head and selected HCE references.
  - [ ] **7.11.d** Archive the final surface, corpus policy, fit/tune artifacts and retry triggers.

## 8. Classical search consolidation and release

The final HCE invalidates assumptions embedded in centipawn margins and changes
the distribution consumed by pruning, histories and reductions. The release
toolchain is selected first because compiler throughput changes depth reached
at clock time controls; categorical search work and SPSA then run on the
toolchain that will actually ship.

### 8.0 Toolchain refresh and freeze

- [ ] **8.0** Update compilers and build tools to the newest validated stable versions.
  - [ ] **8.0.a** Inventory exact local, Linux CI, Windows MSYS2 and macOS AppleClang/compiler, standard-library, CMake, Ninja and profile-tool versions.
  - [ ] **8.0.b** Test current versus newest stable compiler families one change at a time; newest is a candidate, not an automatic winner.
  - [ ] **8.0.c** Require clean compile, CTest, sanitizers and identical cross-platform bench search before accepting a toolchain.
  - [ ] **8.0.d** Compare old/new release-mode and PGO throughput with pooled independent builds; retain the faster non-regressing production toolchain.
  - [ ] **8.0.e** Freeze validated major lines where exact package pins are impractical and record exact resolved versions/hashes in release manifests.
  - [ ] **8.0.f** Keep compiler-matched llvm-profdata and verify every supported architecture.

### 8.1 Finish categorical search work

- [ ] **8.1** Revisit singular-extension gate depth.
  - [ ] **8.1.a** Re-measure only on the frozen post-refit evaluator and selected release toolchain.
  - [ ] **8.1.b** Gate isolated search behavior before tuning constants.

### 8.2 Rebuild the search tuning surface

- [ ] **8.2** Audit and regenerate SPSA parameters from the final HCE head.
  - [ ] **8.2.a** Map every tunable consumer to eval scale, history scale, depth, node type and time control.
  - [ ] **8.2.b** Stage A contains live eval-coupled margins: reverse futility, razoring, futility, ProbCut, null-eval scaling, SEE pruning and aspiration as supported.
  - [ ] **8.2.c** Stage B contains coupled history/LMR coordinates and their consumers only where telemetry shows signal.
  - [ ] **8.2.d** Exclude categorical mechanism switches, mate/endgame constants, TT/hash/thread settings and clock policy from ordinary search SPSA.
  - [ ] **8.2.e** Replace stale config seeds with exact accepted defaults; verify every plus/minus perturbation at start, midpoint and end.
  - [ ] **8.2.f** Register dimensions, ranges, step sizes, schedule, game budget, seed/tail estimator and independent acceptance gates.

### 8.3 Run staged classical search SPSA

- [ ] **8.3** Tune the final-HCE search surface without adjudication.
  - [ ] **8.3.a** Calibrate the runner and use at least the current 5,000-iteration doctrine per production block unless a validated estimator changes it.
  - [ ] **8.3.b** Run and independently gate Stage A against the frozen HCE head.
  - [ ] **8.3.c** Start Stage B from the accepted Stage A head; run and independently gate it.
  - [ ] **8.3.d** Permit one narrow final polish only if residual sensitivity and budget were pre-registered.
  - [ ] **8.3.e** Bake a tail/averaged candidate chosen by the registered estimator, then require clean PGO SPRT, CTest, tactics and endgame truth.
  - [ ] **8.3.f** Preserve rejected tunes as evidence; never combine their apparent gains arithmetically.

### 8.4 Remeasure authority

- [ ] **8.4** Remeasure search/evaluation authority.
  - [ ] **8.4.a** Repeat the oracle split on the final tuned classical head.
  - [ ] **8.4.b** Use the result to prioritize post-release work, not rewrite completed evidence.

### 8.5 Complete clock and time management

- [ ] **8.5** Complete clock and time-management work.
  - [ ] **8.5.a** Diagnose remaining root-instability and time-allocation issues.
  - [ ] **8.5.b** If parameters need tuning, use a separate clock-based tune and gate; never mix them into fixed-node/search SPSA.
  - [ ] **8.5.c** Pass 1T and 4T clock gates with zero forfeits.

### 8.6 Correctness hardening

- [ ] **8.6** Complete correctness hardening.
  - [ ] **8.6.a** Run state, repetition/rule-50, TT/mate, SEE/pin and sanitizer matrices.
  - [ ] **8.6.b** Add regressions for every defect found.

### 8.7 Portability and ISA

- [ ] **8.7** Complete portability and ISA validation.
  - [ ] **8.7.a** Validate target-native execution and exact search agreement.
  - [ ] **8.7.b** Publish executable ISA and same-target performance evidence.

### 8.8 SMP validation

- [ ] **8.8** Complete SMP validation.
  - [ ] **8.8.a** Revalidate node/thread/helper-clock safety.
  - [ ] **8.8.b** Pass registered 1T/4T strength and scaling gates.

### 8.9 Classical release

- [ ] **8.9** Release the final classical line.
  - [ ] **8.9.a** Reproduce clean PGO binaries and manifests with the frozen toolchains.
  - [ ] **8.9.b** Pass cumulative 1.9.3 and external-cohort matches.
  - [ ] **8.9.c** Publish the warranted version from measured cumulative strength.

## 9. NNUE runway

- [ ] **9.0** Freeze the NNUE state and feature contract.
  - [ ] **9.0.a** Specify inputs, perspective, accumulators, serialization and refresh rules.
  - [ ] **9.0.b** Add scalar oracle and incremental-state differential tests.

- [ ] **9.1** Prepare the trainer and corpus.
  - [ ] **9.1.a** Audit D:/code/net_trainer against the frozen contract.
  - [ ] **9.1.b** Generate, validate, hash and split the teacher corpus.
  - [ ] **9.1.c** Complete trainer preflight and reproducibility manifest.

## 10. Baseline NNUE and 2.0.0

- [ ] **10.0** Train and integrate the baseline network.
  - [ ] **10.0.a** Train registered baselines and select on frozen validation data.
  - [ ] **10.0.b** Integrate inference, accumulator updates and network packaging.
  - [ ] **10.0.c** Pass scalar/incremental equality, bench and performance gates.

- [ ] **10.1** Adapt search to NNUE.
  - [ ] **10.1.a** Reprice evaluation-dependent pruning and correction mechanisms.
  - [ ] **10.1.b** Run the single reserved post-NNUE search SPSA.
  - [ ] **10.1.c** Gate 1T, LTC and 4T deployment conditions.

- [ ] **10.2** Release 2.0.0.
  - [ ] **10.2.a** Pass correctness, network provenance and fallback checks.
  - [ ] **10.2.b** Pass prior-release and external-cohort gates.

## 11. Post-NNUE frontier

- [ ] **11.0** Improve architecture and data only from measured bottlenecks.
  - [ ] **11.0.a** Evaluate larger/sparser architectures and better feature transforms.
  - [ ] **11.0.b** Refresh data only under a registered label and sampling contract.

- [ ] **11.1** Extend search selectively.
  - [ ] **11.1.a** Revisit rejected classical mechanisms only when NNUE changes their retry trigger.
  - [ ] **11.1.b** Require isolated gates and preserve Basilisk-specific design.

## 12. Scaling and platform

- [ ] **12.0** Improve parallel scaling.
  - [ ] **12.0.a** Profile split points, contention and TT traffic at 2/4/8 threads.
  - [ ] **12.0.b** Gate strength and throughput independently.

- [ ] **12.1** Expand supported platforms.
  - [ ] **12.1.a** Validate compilers, ISAs and packaging on target-native hardware.
  - [ ] **12.1.b** Keep portable fallbacks behaviorally checked.

## 13. Optional classical fallback

- [ ] **13.0** Reopen HCE only if NNUE is abandoned or a release blocker demands it.
  - [ ] **13.0.a** Require a new feature surface or new data contract; never refit the unchanged surface again.
  - [ ] **13.0.b** Register budget and acceptance before work begins.

## 14. Historical number map

Old references remain valid in EXPERIMENTS and git. Use this map rather than
rewriting historical evidence.

| Historical work | Current location |
|---|---|
| 5.0–5.3 | 5.0–5.2 |
| 5.4–5.6 | 5.3–5.5 |
| completed 5.7/5.8 evidence | 5.6 |
| 5.14 shallow cost | 5.7 |
| 5.9.1–5.9.6 | 5.8–5.9 |
| 5.9.11/5.9.15 | 5.9 |
| 5.9.12–5.9.14 | 5.10 |
| 5.9.16 | 5.11 |
| 5.9.7/5.9.17–5.9.21 | 5.12–5.14 |
| open 5.9.22–5.9.39 | 6.1–6.8 |
| open 5.7.5 | 8.1 |
| open 5.8.7 | 8.5 |
| old 5.10–5.13 | 8.6–8.9 |
| old Phases 6–10 | new Phases 9–13 |
