# The search kernel's literals — B.6.7's screen list and the constants census

**Status: the list, built 2026-11-13 on the B.6.6 head (`fdb35fa`), before any
screen read.** PLAN B.6.7 (maintainer 2026-11-10 and 2026-11-11): every number
the node kernel, quiescence, the history updates and the root record read that
is no search option is listed with its donor line, nothing left off for looking
unimportant. Each one that can be read is exposed on a screen build and read at
20% either side by B-102's rule; one that passes joins B.6.7's surface as a
coordinate, one that does not stays a literal, with its read recorded here.
This file is also the constants census B.11.3's disclosure reads.

- **Source.** `src/kernel_literals.h` holds every screened literal once, as
  `L(field, Lit<Name>, value)`. A release or tune build reads each as a
  namespace-scope constant: the release executable's code is instruction-for-
  instruction the head's (normalised `llvm-objdump -d` of the plain release
  build, 352,068 lines, differing only in the file name). The screen build
  (CMake `LITERAL_SCREEN` on a Tune build) advertises each as a UCI option,
  range about half to twice its value.
- **Donor.** `sf_19` (`edb0d9db`), `src/search.cpp` unless another file is
  named; `history.h` for the bounds. A value marked *ours* has no donor
  literal: it is this engine's own, or the donor's converted into this
  engine's evaluation units (one donor unit is 0.282 of ours) and named so.
- **Bench at the defaults:** 3,783,135 on the release and the screen builds.
- **Liveness** (`tools/results/b67-20261113/liveness.py`, the bench-13 replay
  over UCI with one option moved by 20%, at least 1, each way): 115 of 118 move
  the node total. Three do not, and each is read in a position that reaches it:
  `LitTtRule50` moves a rook ending at a rule-50 count of 90 (188,953 nodes at
  77, 242,263 at 96, 110,616 at 115, depth 18); `LitRootUnderpromo` and
  `LitShuffleNullPlies` reach no position tried (a rook or bishop promotion
  scoring above the queen's at the root; a null move within six plies at ply 20
  or deeper), and their wire is the one read site each.
- **Three points crash the engine:** `LitHindDownDepth` 1, `LitFhRedMin` 2 and
  `LitFhRedBy` 4. Each lets a main-search node reach depth 0, which the kernel
  never sees at the head: the hindsight reduction runs from depth 2, and the
  fail-high cut takes 3 plies only above depth 3. These three are guards in
  that direction and are read only in the other.

## Screened (118)

| Option | Value | Basilisk | Donor | Role |
|---|---:|---|---|---|
| `LitLmrWindowPv` | 577 | `search_b5.h` `lmr_window_term` | 1887 | LMR: the PV node's window share |
| `LitLmrWindowNull` | 163 | `search_b5.h` `lmr_window_term` | *ours* (577 × 0.282) | LMR: a null window in the donor's units |
| `LitLmrHistDivScale` | 1024 | kernel move loop, `LMR_HISTORY_DIVISOR` | 55–56 (the table) | the 16-entry divisor table, scaled as one |
| `LitPruneMainHist` | 69 | kernel move loop | 1208 | main history's weight in the pruning history |
| `LitLmrTtPvCut` | 940 | kernel LMR | 1318 | ttPv reduction for a deep entry at a cut node |
| `LitLmrBase` | 697 | kernel LMR | 1321 | constant reduction |
| `LitLmrMoveCount` | 65 | kernel LMR | 1323 | reduction relief per move searched |
| `LitLmrCorrDiv` | 7419 | kernel LMR | 1324 (26310 × 0.282) | correction's slope |
| `LitLmrCorrCap` | 2695 | kernel LMR | *ours* | correction term's cap |
| `LitLmrBias` | 2179 | kernel LMR | 1340 | relief for the TT move |
| `LitStatCapture` | 873 | kernel stat score | 1343 | captured value's weight |
| `LitStatMain` | 2252 | `history_core.h` `QuietStatScore` | 1347 | main history in the stat score |
| `LitStatCont1` | 1126 | same | 1347 | first continuation |
| `LitStatCont2` | 1093 | same | 1348 | second continuation |
| `LitLmrAlphaLo` | −64 | kernel LMR | 1355 | clamp of alpha less the estimate |
| `LitLmrAlphaHi` | 96 | kernel LMR | 1355 | same |
| `LitLmrAllSlope` | 256 | kernel LMR | 1359 | all-node scaling's depth slope |
| `LitLmrAllOffset` | 268 | kernel LMR | 1359 | its offset |
| `LitLmrNoTtStep` | 1127 | kernel, both full-depth sites | 1399 | step bias without a TT move |
| `LitLmrStep1` | 5234 | same | 1403 | first step threshold |
| `LitLmrStep2` | 5487 | same | 1403 | second step threshold |
| `LitLmrStep2Depth` | 2 | same | 1403 | the second step's depth floor |
| `LitLmrMinDepth` | 2 | kernel LMR | 1362 | LMR's least depth |
| `LitPvTtMoveDepth` | 1 | kernel, both sites | 1419 | PV TT move kept a ply deep from this entry depth |
| `LitResearchContBonus` | 1334 | kernel re-search | 1390 | continuation bonus of a reduced move that failed high |
| `LitRootUnderpromo` | 200 | kernel root | *ours* | root rook/bishop promotion's margin over the queen |
| `LitTiePromoPlies` | 2 | kernel move loop | 1511 | an equal move's promotion window near the horizon |
| `LitHindUpReduction` | 3 | kernel hindsight | 867 | parent's reduction that deepens |
| `LitHindDownReduction` | 2 | same | 869 | parent's reduction that shallows |
| `LitHindDownDepth` | 2 | same | 869 | least depth (guard below) |
| `LitHindDownEval` | 47 | same | 869 (166 × 0.282) | the evals' sum |
| `LitTtNodeTypedDepth` | 4 | kernel TT cutoff | 877 | depth past which the node type is not checked |
| `LitTtPrevMoveCount` | 5 | same | 887 | parent's early-move limit for the malus |
| `LitTtPrevMalus` | −2210 | same | 888 | that malus |
| `LitTtGraphDepth` | 7 | same | 895 | depth of the graph-history check |
| `LitTtRule50` | 96 | `search_b3.h` | 893 | rule-50 count under which a cutoff is taken |
| `LitTtPenaltyDepth` | 5 | kernel TT penalty | 916 | its least depth |
| `LitEvalDiffLo` | −189 | kernel | 981 | the parent's eval difference, clamp |
| `LitEvalDiffHi` | 194 | kernel | 981 | same |
| `LitEvalDiffOffset` | 60 | kernel | 981 | its offset |
| `LitEvalDiffMain` | 11 | `search_history_core.cpp` | 982 | main history's share |
| `LitEvalDiffPawn` | 13 | same | 985 | pawn history's share |
| `LitNmpMinPlyNum` | 3 | kernel NMP verification | 1035 | the verification's ply share, in quarters |
| `LitSingTtDepth` | 3 | kernel singular | 1248 | entry depth below the node |
| `LitSingBetaDen` | 63 | `search_b3.h` | 1250 | singular margin's denominator |
| `LitSingTmhDen` | 114178 | kernel singular | 1261 | TT-move history's scale in the double margin |
| `LitShuffleRule50` | 10 | `search_b3.h` | 154 | shuffling guard: rule-50 count |
| `LitShuffleNullPlies` | 6 | same | 156 | plies since null |
| `LitShufflePly` | 20 | same | 156 | ply |
| `LitFhRedMin` | 3 | kernel after alpha rises | 1535 | depth above which the rest go shallower (guard below) |
| `LitFhRedMax` | 12 | same | 1535 | depth below which |
| `LitFhRedBy` | 3 | same | 1536 | by how much (guard above) |
| `LitTtMoveBonus` | 918 | kernel | 1575 | TT-move history, best |
| `LitTtMoveMalus` | −747 | kernel | 1575 | not best |
| `LitCorrNoMoveScale` | 1024 | kernel correction update | 1635 (18 / 12) | the no-best-move ratio 3/2, scaled |
| `LitCorrShare` | 1061 | same | 1637 | the update's share |
| `LitCorrPawnW` | 15341 | `search_history_core.cpp` | 100 | pawn correction's weight |
| `LitCorrMinorW` | 10569 | same | 100 | minor |
| `LitCorrNonPawnW` | 12906 | same | 100 | non-pawn, each side |
| `LitCorrContNone` | 64049 | same | 98 | continuation term without a move |
| `LitCorrMinorShare` | 150 | same | 120 | minor update share |
| `LitCorrNonPawnShare` | 186 | same | 116, 121–122 | non-pawn, both sides |
| `LitCorrCont2Share` | 130 | same | 128 | continuation correction, ply 2 |
| `LitCorrCont4Share` | 70 | same | 129 | ply 4 |
| `LitContW1` … `LitContW6` | 520, 390, 145, 251, 66, 209 | same | 2019 | continuation update's per-ply weights |
| `LitContConsistencyScale` | 1024 | same | 2022 (the table) | the 7-entry agreement multiplier, scaled as one |
| `LitContNearBonus` | 73 | same | 2038 | flat bonus to the two nearest rows |
| `LitLowPlyShare` | 712 | same | 2052 | quiet update's low-ply share |
| `LitContShare` | 750 | same | 2054 | continuation share |
| `LitPawnShareUp` | 1104 | same | 2057 | pawn history share, bonus |
| `LitPawnShareDown` | 459 | same | 2057 | malus |
| `LitPawnShareThreshold` | −4 | same | 2057 | its sign threshold |
| `LitQuietBonusOffset` | −81 | same | 1973 | best-move bonus offset |
| `LitQuietBonusTt` | 364 | same | 1973 | bonus when best is the TT move |
| `LitQuietBonusStatDen` | 28 | same | 1973 | parent's stat score's divisor |
| `LitMalusOffset` | −235 | same | 1974 | malus offset |
| `LitBonusCountDen` | 256 | same | 1979 | bonus growth per move searched |
| `LitQuietBonusShare` | 899 | same | 1983 | best quiet's share |
| `LitMalusShare` | 1159 | same | 1986 | first quiet malus's share |
| `LitCaptBonusShare` | 1427 | same | 1997 | best capture's share |
| `LitContMalusShare` | 713 | same | 2003 | parent's early quiet, malus |
| `LitCaptMalusShare` | 1489 | same | 2010 | earlier captures, malus |
| `LitFlCapture` | 892 | same, after a fail low | 1608 | parent's capture bonus |
| `LitFlBase` | −241 | same | 1581 | scale's base |
| `LitFlStatDiv` | 98 | same | 1582 | parent's stat score divisor |
| `LitFlDepthSlope` | 59 | same | 1583 | per depth |
| `LitFlDepthCap` | 420 | same | 1583 | cap |
| `LitFlLateMove` | 186 | same | 1584 | late parent move |
| `LitFlLateCount` | 9 | same | 1584 | how late |
| `LitFlBelowOwn` | 142 | same | 1585 | below own eval |
| `LitFlOwnMargin` | 30 | same | 1585 (106 × 0.282) | by how much |
| `LitFlBelowParent` | 159 | same | 1586 | below the parent's |
| `LitFlParentMargin` | 19 | same | 1586 (68 × 0.282) | by how much |
| `LitFlSizeSlope` | 150 | same | 1592 | bonus size per depth |
| `LitFlSizeOffset` | 85 | same | 1592 | offset |
| `LitFlSizeCap` | 1337 | same | 1592 | cap |
| `LitFlContNum` | 263 | same | 1595 | continuation share (over 16384) |
| `LitFlMainShare` | 215 | same | 1597 | main share (over 32768) |
| `LitFlPawnShare` | 324 | same | 1600 | pawn share (over 8192) |
| `LitBoundMain` | 7183 | `history_core.h` | `history.h` 128 | main history's bound |
| `LitBoundLowPly` | 7183 | same | `history.h` 132 | low-ply |
| `LitBoundCapture` | 10692 | same | `history.h` 135 | capture |
| `LitBoundCont` | 30000 | same | `history.h` 138 | continuation (read to 32767 above: an entry is 16-bit) |
| `LitBoundPawn` | 8192 | same | `history.h` 146 | pawn |
| `LitBoundTtMove` | 8192 | same | `history.h` 196 | TT-move |
| `LitBoundCorr` | 1024 | same | `history.h` 41 | pawn, minor and non-pawn correction |
| `LitBoundContCorr` | 1024 | same | `history.h` 41 | continuation correction |
| `LitLowPlyRefill` | 102 | `history_core.cpp` | 326 | low-ply history's value at each search |
| `LitEmaMin` | 12 | `search_b5.h` | 1450 | root record's least weight (of 32) |
| `LitEmaMax` | 24 | same | 1451 | greatest |
| `LitEmaMsqMax` | 16 | same | 1456 | mean square's greatest |
| `LitEmaChi` | 3 | same | 1448 | chi numerator (over 2) |
| `LitAspMinDepth` | 3 | `search_worker.cpp`, both loops | *ours* | iterations searched with a full window |

## Listed, not screened

| Literal | Where | Donor | Why not |
|---|---|---|---|
| LMR window 577, non-unit path | kernel `lmr_reduction` | 1887 | `B5LmrWindowUnits` 0 only; the head runs 1 |
| quiescence delta margin 200 | kernel quiescence | none | `B4QsDelta` 0 at the head |
| NMP verification depth 10 | kernel NMP | none | `B3NmpVerify` 2 only; the head runs 1 |
| IIR stale depth 3 | `search_b3.h` | none | `B3IirStale` 0 at the head |
| singular entry depth 2 | kernel singular | none | `B3SingTtDepth` 1 only |
| history fills −5, −742, −586, −1338, −5, 5 | `history_core.cpp` | 691–710 | `CoreHistoryFills` 0 at the head |
| legacy aspiration growth ½ and give-up 900 | `search_worker.cpp` | none | `B5Window` 0 only |
| helper window offset 282 per thread | `search_b5.h` | 376 | inert at one thread, which every read and gate plays |
| tablebase store depth + 6 | kernel | 958 | unread with tablebases off, as every read and gate is |
| tie-promotion sampler `nodes & 14` | kernel move loop | 1511 | a pseudo-random sample, no magnitude |
| `cont_hist_score`'s ply-4 half | `search_history_core.cpp` | none | read by the decision trace only, never by the search |
| low-ply plies 5, correction table 16,384, pawn buckets 2¹³ | `history_core.h` | `history.h` | table shapes |
| correction update clamps ±256, multi-cut's ±256 | kernel, `search_b3.h` | 1285, 1636 | the bound over 4, coupled to `LitBoundCorr` |
| root record scale 32, chi denominator 2 | `search_b5.h` | 1447, 1449 | fixed-point units of the weights read above |
| unit conversions 1000/282, correction /131072 | throughout | none | units |
| fixed-point denominators 32, 128, 512, 1024, 4096, 8192, 16384, 32768, 65536 | throughout | as the shares | units of the numerators read above |
| capacities and sentinels: 32 searched, 8 pending and queens, 6 continuations, 256 reductions, 2047 poll mask, 7 stack sentinels, `MAX_PLY − 2` | kernel | 73, 290 | storage |
| ordering keys 10,000,000, 7,000,000, 8,000,000, the rank clamp ±2,000 | kernel, `search_root.cpp` | none | order only |
| `update_all_stats`' parent condition `move_count == 1 + tt_hit` | `search_history_core.cpp` | 2001 | a rule, no magnitude |
| time manager's constants | `search_worker.cpp` | `timeman.cpp` | D.1's: time constants never join a search surface |

## Reads

BAS-S61's relevance read (2026-11-13; `analysis/b67_surface_2026-11-13.md` §3 and its
results): `suite_v2` at depth 14 at each point against the defaults, the largest
movement of a row the null points left eligible. **60 of the 118 join B.6.7's surface**
as their screen options; the rest stay literals. The curvature sweep may still drop a
joiner (the packet's results).

| Option | points | read | largest eligible movement |
|---|---|---|---|
| `LitAspMinDepth` | 2, 4 | stays a literal | -6.4% ply-2 correction updates / interior at 4 |
| `LitBonusCountDen` | 205, 307 | stays a literal | -8.6% continuation pruning / moves tested at 307 |
| `LitBoundCapture` | 8554, 12830 | **joins** | -17.8% continuation pruning / moves tested at 8554 |
| `LitBoundCont` | 24000, 36000 | **joins** | -13.9% continuation pruning / moves tested at 24000 |
| `LitBoundContCorr` | 819, 1229 | stays a literal | -9.1% continuation pruning / moves tested at 1229 |
| `LitBoundCorr` | 819, 1229 | **joins** | -12.8% continuation pruning / moves tested at 819 |
| `LitBoundLowPly` | 5746, 8620 | **joins** | -10.1% null tries / interior at 5746 |
| `LitBoundMain` | 5746, 8620 | **joins** | -12.2% continuation pruning / moves tested at 5746 |
| `LitBoundPawn` | 6554, 9830 | stays a literal | -7.6% null tries / interior at 9830 |
| `LitBoundTtMove` | 6554, 9830 | stays a literal | -4.5% null tries / interior at 6554 |
| `LitCaptBonusShare` | 1142, 1712 | **joins** | -18.6% continuation pruning / moves tested at 1142 |
| `LitCaptMalusShare` | 1191, 1787 | stays a literal | +7.9% capture SEE / moves tested at 1191 |
| `LitContConsistencyScale` | 819, 1229 | **joins** | -25.4% continuation pruning / moves tested at 819 |
| `LitContMalusShare` | 570, 856 | **joins** | -16.9% continuation pruning / moves tested at 570 |
| `LitContNearBonus` | 58, 88 | stays a literal | +9.8% IIR reductions / interior at 88 |
| `LitContShare` | 600, 900 | **joins** | -10.0% continuation pruning / moves tested at 600 |
| `LitContW1` | 416, 624 | stays a literal | -8.9% continuation pruning / moves tested at 416 |
| `LitContW2` | 312, 468 | **joins** | -10.6% continuation pruning / moves tested at 312 |
| `LitContW3` | 116, 174 | stays a literal | +7.3% IIR reductions / interior at 116 |
| `LitContW4` | 201, 301 | **joins** | -14.1% continuation pruning / moves tested at 301 |
| `LitContW5` | 53, 79 | stays a literal | -8.9% continuation pruning / moves tested at 53 |
| `LitContW6` | 167, 251 | **joins** | -12.4% continuation pruning / moves tested at 251 |
| `LitCorrCont2Share` | 104, 156 | stays a literal | +8.5% IIR reductions / interior at 156 |
| `LitCorrCont4Share` | 56, 84 | **joins** | -19.1% continuation pruning / moves tested at 84 |
| `LitCorrContNone` | 51239, 76859 | stays a literal | -6.8% ply-2 correction updates / interior at 76859 |
| `LitCorrMinorShare` | 120, 180 | **joins** | -12.2% continuation pruning / moves tested at 180 |
| `LitCorrMinorW` | 8455, 12683 | **joins** | -10.4% continuation pruning / moves tested at 8455 |
| `LitCorrNoMoveScale` | 819, 1229 | **joins** | -13.6% continuation pruning / moves tested at 819 |
| `LitCorrNonPawnShare` | 149, 223 | **joins** | -20.7% continuation pruning / moves tested at 149 |
| `LitCorrNonPawnW` | 10325, 15487 | **joins** | -14.0% continuation pruning / moves tested at 15487 |
| `LitCorrPawnW` | 12273, 18409 | **joins** | -10.9% continuation pruning / moves tested at 18409 |
| `LitCorrShare` | 849, 1273 | **joins** | -19.7% continuation pruning / moves tested at 849 |
| `LitEmaChi` | 2, 4 | stays a literal | -7.0% continuation pruning / moves tested at 4 |
| `LitEmaMax` | 19, 29 | stays a literal | -5.8% continuation pruning / moves tested at 29 |
| `LitEmaMin` | 10, 14 | stays a literal | -7.3% continuation pruning / moves tested at 14 |
| `LitEmaMsqMax` | 13, 19 | stays a literal | +6.8% IIR reductions / interior at 13 |
| `LitEvalDiffHi` | 155, 233 | stays a literal | -8.2% continuation pruning / moves tested at 155 |
| `LitEvalDiffLo` | -227, -151 | **joins** | +10.4% multi-cuts / singular tries at -227 |
| `LitEvalDiffMain` | 9, 13 | **joins** | -12.5% continuation pruning / moves tested at 13 |
| `LitEvalDiffOffset` | 48, 72 | **joins** | -17.2% continuation pruning / moves tested at 48 |
| `LitEvalDiffPawn` | 10, 16 | stays a literal | -6.7% continuation pruning / moves tested at 10 |
| `LitFhRedBy` | 2 | stays a literal | -6.3% LMR applied / moves tested at 2 |
| `LitFhRedMax` | 10, 14 | **joins** | -11.6% continuation pruning / moves tested at 14 |
| `LitFhRedMin` | 4 | stays a literal | -6.6% null tries / interior at 4 |
| `LitFlBase` | -289, -193 | **joins** | +10.7% IIR reductions / interior at -289 |
| `LitFlBelowOwn` | 114, 170 | **joins** | -15.6% continuation pruning / moves tested at 114 |
| `LitFlBelowParent` | 127, 191 | **joins** | -13.1% null tries / interior at 191 |
| `LitFlCapture` | 714, 1070 | **joins** | -11.8% continuation pruning / moves tested at 1070 |
| `LitFlContNum` | 210, 316 | stays a literal | -8.8% continuation pruning / moves tested at 316 |
| `LitFlDepthCap` | 336, 504 | **joins** | -14.9% continuation pruning / moves tested at 504 |
| `LitFlDepthSlope` | 47, 71 | stays a literal | -9.8% continuation pruning / moves tested at 71 |
| `LitFlLateCount` | 7, 11 | stays a literal | -9.7% continuation pruning / moves tested at 7 |
| `LitFlLateMove` | 149, 223 | **joins** | -14.5% continuation pruning / moves tested at 223 |
| `LitFlMainShare` | 172, 258 | stays a literal | -9.7% continuation pruning / moves tested at 172 |
| `LitFlOwnMargin` | 24, 36 | stays a literal | -9.9% continuation pruning / moves tested at 36 |
| `LitFlParentMargin` | 15, 23 | **joins** | -14.5% continuation pruning / moves tested at 15 |
| `LitFlPawnShare` | 259, 389 | stays a literal | +9.7% IIR reductions / interior at 259 |
| `LitFlSizeCap` | 1070, 1604 | stays a literal | -8.9% continuation pruning / moves tested at 1604 |
| `LitFlSizeOffset` | 68, 102 | stays a literal | -9.2% continuation pruning / moves tested at 68 |
| `LitFlSizeSlope` | 120, 180 | **joins** | -16.1% continuation pruning / moves tested at 180 |
| `LitFlStatDiv` | 78, 118 | **joins** | +11.0% IIR reductions / interior at 78 |
| `LitHindDownDepth` | 3 | **joins** | -25.0% hindsight shallowings / interior at 3 |
| `LitHindDownEval` | 38, 56 | **joins** | -13.7% hindsight shallowings / interior at 56 |
| `LitHindDownReduction` | 1, 3 | **joins** | +21.1% hindsight shallowings / interior at 1 |
| `LitHindUpReduction` | 2, 4 | **joins** | +46.4% hindsight deepenings / interior at 2 |
| `LitLmrAllOffset` | 214, 322 | **joins** | -10.4% continuation pruning / moves tested at 214 |
| `LitLmrAllSlope` | 205, 307 | **joins** | -16.9% continuation pruning / moves tested at 205 |
| `LitLmrAlphaHi` | 77, 115 | **joins** | -11.5% continuation pruning / moves tested at 115 |
| `LitLmrAlphaLo` | -77, -51 | **joins** | -12.5% continuation pruning / moves tested at -77 |
| `LitLmrBase` | 558, 836 | **joins** | -13.0% continuation pruning / moves tested at 836 |
| `LitLmrBias` | 1743, 2615 | stays a literal | +7.4% IIR reductions / interior at 1743 |
| `LitLmrCorrCap` | 2156, 3234 | stays a literal | +7.0% IIR reductions / interior at 3234 |
| `LitLmrCorrDiv` | 5935, 8903 | stays a literal | -8.7% continuation pruning / moves tested at 8903 |
| `LitLmrHistDivScale` | 819, 1229 | **joins** | +22.8% IIR reductions / interior at 819 |
| `LitLmrMinDepth` | 1, 3 | **joins** | -16.6% stand-pat cuts / quiescence nodes at 1 |
| `LitLmrMoveCount` | 52, 78 | stays a literal | -7.2% hindsight deepenings / interior at 78 |
| `LitLmrNoTtStep` | 902, 1352 | stays a literal | -8.4% null tries / interior at 902 |
| `LitLmrStep1` | 4187, 6281 | **joins** | +21.6% razoring cuts / interior at 6281 |
| `LitLmrStep2` | 4390, 6584 | **joins** | -13.3% continuation pruning / moves tested at 4390 |
| `LitLmrStep2Depth` | 1, 3 | **joins** | +11.4% IIR reductions / interior at 1 |
| `LitLmrTtPvCut` | 752, 1128 | stays a literal | +2.5% mean fail-high index at 752 |
| `LitLmrWindowNull` | 130, 196 | stays a literal | +5.7% IIR reductions / interior at 196 |
| `LitLmrWindowPv` | 462, 692 | stays a literal | +8.3% IIR reductions / interior at 692 |
| `LitLowPlyRefill` | 82, 122 | **joins** | -12.1% continuation pruning / moves tested at 82 |
| `LitLowPlyShare` | 570, 854 | stays a literal | -7.9% continuation pruning / moves tested at 570 |
| `LitMalusOffset` | -282, -188 | stays a literal | +7.0% IIR reductions / interior at -188 |
| `LitMalusShare` | 927, 1391 | **joins** | -16.4% continuation pruning / moves tested at 927 |
| `LitNmpMinPlyNum` | 2, 4 | stays a literal | +2.0% null tries / interior at 4 |
| `LitPawnShareDown` | 367, 551 | **joins** | -12.9% continuation pruning / moves tested at 367 |
| `LitPawnShareThreshold` | -5, -3 | stays a literal | +2.2% multi-cuts / singular tries at -3 |
| `LitPawnShareUp` | 883, 1325 | stays a literal | -7.9% continuation pruning / moves tested at 1325 |
| `LitPruneMainHist` | 55, 83 | **joins** | -14.3% quiet futility / moves tested at 55 |
| `LitPvTtMoveDepth` | 0, 2 | stays a literal | -7.2% continuation pruning / moves tested at 2 |
| `LitQuietBonusOffset` | -97, -65 | stays a literal | +5.1% singular doubles / singular tries at -97 |
| `LitQuietBonusShare` | 719, 1079 | **joins** | -13.7% continuation pruning / moves tested at 1079 |
| `LitQuietBonusStatDen` | 22, 34 | stays a literal | -8.4% null tries / interior at 22 |
| `LitQuietBonusTt` | 291, 437 | **joins** | -14.0% continuation pruning / moves tested at 437 |
| `LitResearchContBonus` | 1067, 1601 | **joins** | -13.4% continuation pruning / moves tested at 1601 |
| `LitRootUnderpromo` | 160, 240 | stays a literal | +0.0% LMR applied / moves tested at 160 |
| `LitShuffleNullPlies` | 5, 7 | stays a literal | +0.0% LMR applied / moves tested at 5 |
| `LitShufflePly` | 16, 24 | stays a literal | +2.0% null tries / interior at 16 |
| `LitShuffleRule50` | 8, 12 | stays a literal | +2.2% hindsight deepenings / interior at 8 |
| `LitSingBetaDen` | 50, 76 | **joins** | -15.2% singular doubles / singular tries at 50 |
| `LitSingTmhDen` | 91342, 137014 | stays a literal | -4.0% quiescence futility / quiescence moves at 137014 |
| `LitSingTtDepth` | 2, 4 | **joins** | -15.5% continuation pruning / moves tested at 2 |
| `LitStatCapture` | 698, 1048 | stays a literal | +6.7% razoring cuts / interior at 1048 |
| `LitStatCont1` | 901, 1351 | stays a literal | -9.4% continuation pruning / moves tested at 901 |
| `LitStatCont2` | 874, 1312 | **joins** | -15.2% continuation pruning / moves tested at 1312 |
| `LitStatMain` | 1802, 2702 | **joins** | -16.6% continuation pruning / moves tested at 1802 |
| `LitTiePromoPlies` | 1, 3 | stays a literal | +9.8% IIR reductions / interior at 3 |
| `LitTtGraphDepth` | 6, 8 | stays a literal | -6.7% continuation pruning / moves tested at 6 |
| `LitTtMoveBonus` | 734, 1102 | stays a literal | +5.7% IIR reductions / interior at 734 |
| `LitTtMoveMalus` | -896, -598 | stays a literal | +5.3% quiescence fail highs / quiescence nodes at -896 |
| `LitTtNodeTypedDepth` | 3, 5 | stays a literal | +8.6% singular doubles / singular tries at 3 |
| `LitTtPenaltyDepth` | 4, 6 | stays a literal | -8.8% continuation pruning / moves tested at 6 |
| `LitTtPrevMalus` | -2652, -1768 | **joins** | -10.8% continuation pruning / moves tested at -1768 |
| `LitTtPrevMoveCount` | 4, 6 | **joins** | -16.4% continuation pruning / moves tested at 4 |
| `LitTtRule50` | 77, 115 | stays a literal | +0.0% LMR applied / moves tested at 77 |
