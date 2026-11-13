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
  iteration on 15 slots, `r_end` 0.00155, seed 67001, a ceiling of 2,000 iterations
  (one night); its read there goes to the maintainer.
- No subject, point, row, threshold or ceiling changes after a read is seen.

#### PRE-REGISTERED PREDICTION — freeze before exposure

The packet's §8: R1 at least 60 of 74 coordinates pass (70%); R2 30 to 80 of 118
literals pass (60%); R3 at least three rows ineligible (60%); R4 a coordinate fails the
wire (30%); R5 a third of the surface flat on WAC (60%); R6 most pruning margins'
WAC maxima on the less-pruning side (55%).

#### RESULT — append after exposure
