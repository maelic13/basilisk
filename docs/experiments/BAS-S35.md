# BAS-S35

**BAS-S35 — A.11.10: the rule-50 damping keeps a bare-king tier's base**

- **Registration.** Registered 2026-10-27, before any game; maintainer-run;
  a correctness repair of A.11.9 found by A.11.11's research (PLAN
  A.11.10), an evaluation change the maintainer admitted ahead of
  fail-soft quiescence.
- **Defect.** A.11.9 put bare-king wins against a queen or a rook in tiers
  (rook 26,100 and up, queen 28,700 and up), and the rule-50 damping scales
  the whole score by `(199 - clock) / 199`, so the rook tier loses about 131
  points a ply. That is more than the king drive KR-K is mated with: over
  the first 24 positions of the conversion-floor generator, 60,000 cold
  nodes a move, `5524bf7` converts 24/24 and `00809cc` 20/24, the losses
  drifting into fifty-move draws. Tier scores left undamped convert 24/24;
  the tier's cap lifted does not (20/24).
- **Candidate.** `basilisk-a1110-fix-pext-pgo.exe`, revision `3d7559a`:
  `damp_rule50` keeps a tier's base and damps only the score above it.
  Every score below the rook tier is damped as before; with the base at 0
  the curve reproduces `bench 13` 7,857,713.
  - Release PEXT final PGO, B2_CORE on, clang 23.1.4, clean manifest,
    built from a temporary worktree at its revision.
  - `bench 13` 8,913,394; SHA-256
    `C8F6296C378A6F6FB2B388C01BDA63DC0C2EA3747FED3BA3FD31318A8BA5A511`.
- **Baseline.** `basilisk-a1110-base-pext-pgo.exe`, revision `050ad18`,
  the head before the repair; `bench 13` 7,857,713; SHA-256
  `6BB7B1CBE29EABC059A7C6092512C7A789C8D1872DE5FF03F5E080139F2B2AF6`. The
  candidate's other commit, `d0e0991`, changes tests only.
- **Mechanism.** The clock's pressure on a bare king against a queen or
  rook is the ordinary score's, as before A.11.9, so the king drive again
  decides between moves; and a tier stays above every lower tier at any
  clock.
- **Interactions.**
  - The damping is applied in the search after the TT read (A.11.1); the
    stored static eval stays undamped, so the TT contract is unchanged.
  - Search trees: tier scores reached in deep capture lines change values
    far outside any window; through the histories bench shares between
    positions, every position from the third on moves.
  - The Texel tuner's linear rule-50 curve does not mirror apply_endgame's
    overrides and is unchanged.
  - The legacy kernel shares the evaluation, so its bench moves too
    (23,696,970 to 22,652,055).
- **Measured exposure.** 3,414 of 11,234 games of the A.11.7 and A.11.9
  gates (30.4%) reach a settled bare king against a queen or rook (the bare
  king's side has just moved), and one of those was drawn, by insufficient
  material. The change acts in a third of the games but almost never on
  their result, so the cap is the small-exposure one.
- **Competing explanation for a neutral reading:** games at 3+0.03
  already convert these endings, so the repair shortens mates without
  changing results.

#### PRE-REGISTERED PREDICTION — freeze before exposure

- **Expected:** about **0 Elo**; most likely the cap without a decision.
  Confidence that the result is not H0: about 85%.
- **Verdict rule:**
  - H1: the repair stays, and the fingerprint record updates to its
    `bench 13`;
  - the cap without a decision: the repair stays as a correctness fix with
    no measured harm, as BAS-S28 and BAS-S32; report the LLR and the
    estimate, and the maintainer confirms;
  - H0: revert the engine commit `3d7559a` and return the leaf to
    `RESEARCH`.
- **Correctness floors, read before the result is acted on:** CTest on the
  candidate, the conversion floors (every family over 24 positions) and the
  WAC floor included.

#### CONDITIONS

- Colosseum SPRT, `sprt-repair.toml`: normalized `[-5,5]`, α = β = 0.05.
- `3+0.03`, 1T, Hash 64, paired reversed UHO openings, no adjudication,
  tablebases off; seed 35002; concurrency 15 as the wrapper resolves it.
- **Cap: 1,500 pairs.**
- Launcher `tools/run_a1110_gate.ps1`; results in
  `tools/results/a1110-gate-r2/`.
- **First run, stopped (void).** Seed 35001, `tools/results/a1110-gate/`:
  the maintainer stopped it at 107 pairs (W/D/L 49/115/50 for the repair,
  no faults), before any decision, to delete the legacy kernel (B.8.1) and
  harden the floors first. It is not a result and is not continued; the
  run above starts from zero with a new seed. The engines are unchanged:
  B.8.1 reproduces `bench 13` 7,857,713 exactly and the floors are tests,
  so the pinned binaries stand.
- **Dry run accepted 2026-10-27**
  (`tools/results/colosseum_sprt_a1110-gate-r2_20261027_125406.*`, dry run
  `ac2de413…`, manifest `23502f6e…`).

#### QUALIFICATION (before the gate)

- On `3d7559a`, Release PEXT without PGO: `bench 13` 8,913,394 and the
  legacy arm 22,652,055; CTest ON 24/24, OFF 21/21.
- Tests failing on the old curve: `test_eval`'s tier order at clock 99 and
  the bound on the rook tier's loss to the clock (3 checks); the KR-K floor
  over 24 positions (21/24 on the old code).
- Conversion over 40 positions, 60,000 cold nodes, Hash 8: KR-K 40/40 at a
  mean 22.1 plies to mate (head 35/40 at 33.5); KQ-K 40/40 at 10.7 (head
  40/40 at 13.3).
- Speed: the PGO arms, four interleaved `bench 13` runs each, 2,826,374
  nps against 2,620,083. The arms search different trees, so this reads
  tree shape, not a cost.

- Disposition: registered, maintainer-run.
