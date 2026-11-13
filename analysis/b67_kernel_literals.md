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

Appended after the screen (B.6.7's registration).
