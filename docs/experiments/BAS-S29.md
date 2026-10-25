# BAS-S29

**BAS-S29 — A.11.2: a lone excluded move fails the singular search by one**

- **Registration.** Registered 2026-10-25, before any game; maintainer-run;
  correctness repair from the Rarog audit (PLAN A.11.2; audit B-4,
  `analysis/rarog_audit_2026-10-24.md`).
- **Defect.** When the singular search excludes a node's only legal move,
  the node has no move left and scored checkmate (in check) or stalemate.
  That sits far below the singular beta, so a forced move took the double
  extension off the PV, and with the check extension on top a forced
  evasion could take three plies; doubles are capped only by
  `DoubleExtMax`. In Rarog the chain left bench unfinished once IIR
  changed; its fix (B.3.1 amendment 4, part a1) is this one.
- **Candidate.** `basilisk-a112-fix-pext-pgo.exe`, revision `8802a85`: an
  exclusion search with no move left returns `alpha`, one below the
  singular beta, as Stockfish does (`no_move_score`, `src/search_types.h`),
  so a forced move extends by one ply. Nothing else changes.
  - Release PEXT final PGO, B2_CORE on, clang 23.1.3, clean manifest,
    built from a temporary worktree at its revision.
  - `bench 13` 8,753,339; SHA-256
    `777BA298BCFE12E65B46EE3858D28B6A658F6C2C12832D42B64684AD8D179F44`.
- **Baseline.** `basilisk-a112-base-pext-pgo.exe`, revision `7f19c7e`
  (`master`, A.11.1 merged), built the same way with the same compiler;
  `bench 13` 9,260,429; SHA-256
  `0CB79CCD71D770F3429ED0974BD386E9C82E1E11D4E4C7500770D3776D75BE60`.
- **Mechanism.** A forced move is not evidence that the node's other moves
  are much worse than the TT move: there are none. One ply of extension
  keeps the forced line's horizon where the single-reply case wants it;
  the double and triple extensions spent depth on forced lines and could
  chain.
- **Interactions.**
  - The check extension (`did_check_ext`) composes with the singular
    extension by measured choice (BAS-D11); a forced evasion now takes two
    plies, not three.
  - `DoubleExtMax` and `ss->double_exts` cap only the doubles; the repair
    removes the lone-move doubles before the cap is reached.
  - `singular_quiet_lmr` still relaxes the siblings' reductions after a
    singular result; a lone move has no siblings.
  - IIR at its rail (8) may have hidden the chain; B.3's IIR-off arm now
    starts from the repaired search.
  - `SingularDoubleMargin` at its lower bound 0 would still double a lone
    move; the default is 4.
- **Competing explanations for a neutral reading:** lone-move nodes are
  rare in play, and the old double extension mostly bought depth on forced
  lines that the search resolved anyway.

#### PRE-REGISTERED PREDICTION — freeze before exposure

- **Expected:** about **0 to +3 Elo**; most likely the cap without a
  decision. Confidence that the result is not H0: about 85%.
- **Most likely failure mode:** a reading too small for the bracket.
- **Verdict rule:**
  - H1: the repair stays, and the fingerprint record updates to its
    `bench 13`;
  - the cap without a decision: the repair stays as a correctness fix
    with no measured harm, as BAS-S20 and BAS-S28; report the LLR and the
    estimate, and the maintainer confirms;
  - H0: revert the engine commit and return the leaf to `RESEARCH` with
    the result.
- **Correctness floors, read before the result is acted on:** CTest on the
  candidate, the conversion floors and the WAC floor included.

#### CONDITIONS

- Colosseum SPRT, `sprt-repair.toml`: normalized `[-5,5]`, α = β = 0.05.
- `3+0.03`, 1T, Hash 64, paired reversed UHO openings, no adjudication,
  tablebases off; seed 29001; concurrency 15 as the wrapper resolves it.
- **Cap: 4,000 pairs**, as BAS-S19, BAS-S26 and BAS-S28 on this bracket.
- Launcher `tools/run_a112_gate.ps1`; results in `tools/results/a112-gate/`.
- **Dry run accepted 2026-10-25**
  (`tools/results/colosseum_sprt_a112-gate_20261025_190729.*`, dry run
  `d801ebf8…831c`, manifest `fa6d05ac…d247`).

#### QUALIFICATION (before the gate)

- On `8802a85`, Release PEXT without PGO:
  - `bench 13`: ON 8,753,339 (from 9,260,429, -5.5%); the legacy OFF arm
    15,887,213, unchanged;
  - CTest: ON 23/23, OFF 20/20; `test_search` 536/536.
- The new `test_search` sections take Rarog's forced `Kxg2`: the
  exclusion search returns `alpha`, which is below the singular beta and
  above the double margin, and a node with nothing excluded still scores
  mate or stalemate. On the old code the same node scored
  `-(MATE_SCORE - ply)`.
- **The counter is live.** A Diag build over `tools/diag/suite_v1.epd` at
  depth 10 counts 2,719 lone-move returns against 32,596 singular
  extensions fired (14,281 doubles) on the 106 legal positions.

#### RESULT — append after exposure

- **Cap without a decision (maintainer-run 2026-10-25, 1h18m):**
  **+1.7 ± 4.8 Elo** (nElo +2.8 ± 7.6), LLR +1.84 in [-2.94, 2.94] at the
  4,000-pair cap (8,000 games); W/D/L 2156/3728/2116, pentanomial
  [149, 945, 1770, 989, 147].
  - 0 faults, 0 time losses, no post-terminal pairs; the independent PGN
    recount agrees, and the score recomputed from the pentanomial matches.
  - Both arms played 188k nodes and 70.0 ms per move at the same 2.69M
    NPS, mean depth 17.17 on both: the repair costs no speed.
  - Terminations: checkmate 4,272, threefold 2,879, insufficient material
    763, fifty-move 65, stalemate 21.
  - `colosseum_sprt_a112-gate_20261025_190841.*`, manifest
    `14e6bafa…`, run record `1b43fa2e…`, PGN `fecb3daa…`.
- **Verdict (registered rule):** the cap without a decision, so the
  repair stays as a correctness fix with no measured harm, on the
  maintainer's confirmation; the fingerprint record then updates to
  `bench 13` 8,753,339.
- Disposition: **cap, +1.7 ± 4.8; the repair is kept (maintainer
  confirmed 2026-10-25). CLOSED 2026-10-25.**

#### PREDICTION CALIBRATION — preserve the original prediction above

- **Prediction:** about 0 to +3 Elo, most likely the cap; not H0 at about
  85%.
- **Reading:** the cap came, centred at +1.7 inside the predicted range,
  with the LLR leaning toward H1 (+1.84). Hit.
- **What it teaches:** removing the lone-move doubles costs nothing in
  play; its value is the bounded extension chain that B.3's IIR-off arm
  needs.
