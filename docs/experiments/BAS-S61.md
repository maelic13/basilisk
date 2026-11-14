# BAS-S61

**BAS-S61 — B.6.7: the literal screen and the relevance read, the curvature sweep, then the joint search SPSA**

- **Registration.** Created 2026-11-13 at B.6.7's opening, before any relevance read,
  sweep point or game; agent-run reads and sweep, maintainer-run SPSA; parameter fit.
  The frozen rules are `analysis/b67_surface_2026-11-13.md`; the literal list with
  each literal's donor line is `analysis/b67_kernel_literals.md`.
- **Subject.** The B.6.6 head (no donor form adopted), `bench 13` 3,783,135, with the
  kernel's literals exposed by the screen build (CMake `LITERAL_SCREEN` on a Tune
  build). The release build's code is instruction-identical to the head's.
- **Research question.** Which of the head's 74 search coordinates and 118 kernel
  literals change what the search does at ±20%, which of those carry curvature, and
  what theta one night of rule 7c SPSA reaches from the head over them.
- **Hypothesis.** Four clusters were fitted one at a time, each with the earlier
  ones held, and about a hundred of the donor's literals were never fitted at all in
  this engine's units and tree; a joint fit over what is live moves some of them.
- **Competing explanations.** The surface is mostly flat at STC after the cluster
  fits and the joint fit is noise (Rarog's joint tune after fitted clusters bought
  +4.5 for 120,000 games, B-106); the gate reads it.

#### CONDITIONS

- The packet's §3 to §7: the wire, the relevance read with its null points and
  eligible rows, the surface (literals join as their screen options; release keeps
  them as constants), rails and boundaries, the WAC curvature sweep, and the tune.
- **The tune:** rule 7c, the screen build at the surface commit with final PGO,
  `3+0.03`, 1T, Hash 64, UHO paired, no adjudication, tablebases off, 30 games an
  iteration on 15 slots, `r_end` 0.00155, seed 67001, one night to rule 7c's first
  checkpoint at 2,000 iterations, whose read goes to the maintainer. Amendment 1
  (packet §7, before any game; it follows from the wrapper and no read): the production schedule for a ceiling of 6,000,
  held at 2,000 by `spsa_colosseum.ps1 -PauseAt 2000`.
- No subject, point, row, threshold or ceiling changes after a read is seen.
- **Amendment 2** (packet, 2026-11-13, before any game, by a reviewing session at
  the maintainer's request): the relevance read's verdicts no longer choose the
  surface, because its rows are too chaotic to separate live from inert (81 of its
  102 passes rest on one row whose spread over all 381 points is 6.6%, and 73 on
  that row and one other alone). The surface is every wired subject that is a
  magnitude, resolvable by a tune and not redundant: **170 coordinates, 69 X lines
  and 101 literals**, among them ten picker literals the screen had not listed
  (`425adcc`). The tune's binary is the screen build at `425adcc`
  (`basilisk-b67-screen2-pext-tune-pgo.exe`, SHA-256 `2DFA7B95…`); the game
  conditions, the schedule and the seed are unchanged. Its predictions A1 to A4
  are in the packet.

#### PRE-REGISTERED PREDICTION — freeze before exposure

The packet's §8: R1 at least 60 of 74 coordinates pass (70%); R2 30 to 80 of 118
literals pass (60%); R3 at least three rows ineligible (60%); R4 a coordinate fails the
wire (30%); R5 a third of the surface flat on WAC (60%); R6 most pruning margins'
WAC maxima on the less-pruning side (55%).

#### RESULT — append after exposure

- **The tune** (maintainer-run 2026-11-13 22:23 to 2026-11-14 10:36 UTC,
  `tools/results/b67-spsa/`): **STOP_QUIET at 2,000**, 60,000 games, no engine, time
  or infrastructure fault, no rail, no arm clamped. The window 1,000–2,000: largest
  |z| 2.82 (line 3.61), Σz² − n +32.1 (+1.7 SD). The window 0–1,000 read live
  (+3.8 SD; `B3SingMargin` z −3.82). The tune's own gain read +22.9 Elo, null SD
  11.6, not a gate. Theta: the 2,000 checkpoint's rounded centres
  (`theta-2000.toml` 6c240b98…).
- **Predictions.** Amendment 2's: A1 misses (quiet, not live); A2 misses (4.2% of the
  read's fails at |z| ≥ 2 against 11.2% of its passes, 1.7 standard errors, short of
  the falsifier's two); A3 misses; A4 hits. The registration's R1–R6 are scored in
  the packet.
- **The fitted-constant analysis** (`analysis/b67_constants_2026-11-14.md`): eight
  findings; none blocks the bake. One is a defect of the screen the bake found: a
  scale literal could not be baked (`0f30bf1`).
- **The bake** (`25c570f`): 61 X lines and 93 literals; `bench 13` **4,278,307**,
  equal to theta's replay; release and sanitizer CTest 30/30; the floors pass.
- **Disposition:** baked, pending B.6's SPRT `[0,3]` against the B.6.6 head,
  registered on its own.
