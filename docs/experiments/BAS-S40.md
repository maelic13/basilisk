# BAS-S40

**BAS-S40 — B.12: quiescence's gain pruning stands down in a known loss**

- **Registration.** Registered 2026-10-30, before any game; maintainer-run;
  correctness repair of PLAN B.12, the false win B.3.0's counter suite
  stopped on (`analysis/b30_cluster2_research_2026-10-30.md` §2.3).
- **Defect.** `7k/8/6KP/8/8/8/8/5B2 w`, an h-pawn with the wrong bishop, is
  a draw; the head read +150.94 at depth 8 and did not finish depth 11 in
  13.5M nodes. With every ablation bit set it still read a known win, so
  the cause sat outside the masked families. Walking the false line, every
  position with the pawn on h7 and the defending king in the corner read
  the known-win class at depth 2, and 0 when searched from its own root.
  The leaf of the false line is an underpromotion beside the king: after
  `h8=B` (or `=N`) the defender's stand-pat is a won KBB-K (KBN-K) for the
  opponent, about −15,000, and quiescence's delta pruning, capture futility
  and alpha-relative SEE threshold all read `Kxh8` as a gain of one minor
  against it. The capture leaves a dead draw; the cut returned the known
  win as a bound.
- **Candidate.** `basilisk-b12-fix-pext-pgo.exe`, revision `d8d30e9`: the
  three prunes run only when `stand_pat + 150 > -KNOWN_WIN`, the oracle's
  `futilityBase > -VALUE_KNOWN_WIN` (classical Stockfish `9587eeeb`,
  `src/search.cpp:1532`) with Basilisk's futility base; below it the SEE
  threshold is 0, the oracle's unconditional `see_ge(move)`. Classical
  Stockfish has no node-level delta prune; Basilisk's keeps its form above
  the band. `KNOWN_WIN` moves to `eval.h` for the search and the test.
  - Release PEXT final PGO, clang 23.1.4, clean manifest.
  - `bench 13` 16,615,757; SHA-256
    `F1C3D0BC7BAD8377B2287B50ADBCE3786F0876B44A0455F60961DC48914ED6E3`.
- **Baseline.** `basilisk-b12-base-pext-pgo.exe`, revision `dd7a3ab`, the
  head before it, built from a temporary worktree; `bench 13` 19,470,325;
  SHA-256 `37B2F3E48B43289CFE94A40DE9F49C9AB4ECAFBDC4376BE2158AE2BE3A193E5B`.
- **Mechanism.** A defender facing a known-win score searches the captures
  that can end the ending; the four wrong-bishop positions of the new test
  read 0 at every depth 8–14 in 6,364–14,373 nodes at depth 14 (the head:
  +355 after 22.5M nodes, +385 after 92M, two never finishing).
- **Interactions.** Only quiescence nodes whose stand-pat is a known loss
  change; no material balance reaches the band, so a middlegame tree is
  unchanged (bench position 4 searched alone: 228,803 nodes in both). The
  bench moves −14.7% because its 40 positions share the table and
  histories: positions 1–2 are identical, the rook ending at position 3
  first meets a bare-king tier in quiescence, and every later position
  inherits that state. The main search's move-loop pruning runs only for
  a side with non-pawn material, as the oracle's does, so a bare defending
  king never meets it. RFP's `eval < tablebaseWinInMaxPly` bound and NMP's
  beta test lack the oracle's `VALUE_KNOWN_WIN` bounds; they are B.3's and
  B.4's (reported, not changed).
- **Measured exposure.** Of the 8,000 games of BAS-S38, 3,183 (39.8%) reach
  a bare king and 5,430 (67.9%) a side with at most two men beside its
  king, from which quiescence can reach one. Nearly all of it is in
  decided positions, where the repair changes only how the defence is
  searched; the positions it can turn are the false wins, which are rarer.
  The exposure is far above 2%, so the cap is the full one.
- **Competing explanation for a neutral reading:** the false wins arise
  only where the stronger side can still promote beside the defending king,
  and in a decided ending the change moves the defender's search, not the
  result.

#### PRE-REGISTERED PREDICTION — freeze before exposure

- **Expected:** about **0 to +2 Elo**; most likely the cap without a
  decision. Confidence that the result is not H0: about 85%.
- **Verdict rule:**
  - H1: the repair stays, and the fingerprint record updates to its
    `bench 13`;
  - the cap without a decision: the repair stays as a correctness fix with
    no measured harm, as BAS-S35, BAS-S37 and BAS-S38; report the LLR and
    the estimate, and the maintainer confirms;
  - H0: revert the engine commit `d8d30e9` and return the leaf to
    `RESEARCH`.
- **Correctness floors, read before the result is acted on:** CTest on the
  candidate, the wrong-bishop test and the conversion floors included.

#### CONDITIONS

- Colosseum SPRT, `sprt-repair.toml`: normalized `[-5,5]`, α = β = 0.05.
- `3+0.03`, 1T, Hash 64, paired reversed UHO openings, no adjudication,
  tablebases off; seed 40001; concurrency as the wrapper resolves it.
- **Cap: 4,000 pairs.**
- Launcher `tools/run_b12_gate.ps1`; results in `tools/results/b12-gate/`.
- **Dry run accepted 2026-10-30**
  (`tools/results/colosseum_sprt_b12-gate_20261030_131742.*`, dry run
  `2a0f697a…`, manifest `879f87e1…`).

#### QUALIFICATION (before the gate)

- On `d8d30e9`, Release PEXT without PGO: `bench 13` 16,615,757 (from
  19,470,325, −14.7%), the same as the PGO build; CTest 24/24.
- Test (`test_endgames`, "KBP-K wrong bishop reads a draw at depths
  8-14"): four positions — the suite's, the a-file mirror, the colours
  reversed, and the near-stalemate position on the false line — each a
  cold search at every depth 8–14 under a 200,000-node cap, which must
  finish the depth and read within the draw tolerance. With only the
  quiescence change reverted it fails 47 of its expectations.
- Conversion floors at the packet's five budgets (30k/45k/60k/90k/120k
  nodes per move; `tools/results/b12-20261030/conversion/`): KQ-K, KR-K
  and KBB-K 24/24 at each; KBN-K 22/23/24/23/23, 115/120 against the head's
  113/120, no change under the packet's rule (a sum moving by more than 3).
  At 120k KQ-K's mean, 10.8 plies against the head's 11.3, trips the
  stale-ceiling check, which is calibrated at the default 60k; at 60k
  every check passes.

#### RESULT — append after exposure

#### PREDICTION CALIBRATION — preserve the original prediction above
