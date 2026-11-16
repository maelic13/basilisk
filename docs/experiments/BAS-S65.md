# BAS-S65

**BAS-S65 — B.8's delivery gate: the cleanup and speed pass against B.7's head**

- **Registration.** 2026-11-16, before any game, at the maintainer's request when B.8
  is delivered as one pull request. The maintainer's run.
- **Question.** Does B.8 as delivered play stronger than the head before it, as its
  speed reads say it should, with nothing broken on the way? Every B.8 step is at an
  identical tree except B.8.7's node count, gated alone (BAS-S64, −0.8 ± 4.5); this
  reads the pass whole.
- **Arms.** A: the head, `b852dc5` (`bench 13` 4,688,537), `basilisk-b811-head-pext-pgo.exe`,
  SHA-256 0CDF89B0…EBA82. B: `master`, `55e3010` (B.7's head, `bench 13` 4,247,965),
  `basilisk-b811-master-pext-pgo.exe`, SHA-256 28B7D5EA…325D5. Both final-PGO PEXT
  release builds by `build_test.ps1`, clang 23.1.5, clean manifests.
- **Conditions.** Colosseum (`tools/run_b8_gate.py`, `sprt-default.toml`): SPRT on the
  default bracket `[0,3]` nElo, α = β = 0.05, 3+0.03, 1 thread, Hash 64, paired reversed
  UHO openings (`UHO_Lichess_4852_v1.epd`), no adjudication, tablebases off, seed 88010,
  15 games at once, cap 4,000 pairs.
- **Why this bracket and cap.** A strength claim (agents/measurement.md *Gating*). The
  expected gain from the speed reads is about +16 to +20 Elo (bench about +8%, the
  game's load about +10%, by BAS-P01's 2 Elo per 1%). From BAS-S64's pairs (pair-score
  SD 0.21, 87 games a minute) the drift model expects about 2,300 pairs at +16 Elo
  (about 55 minutes) and 1,850 at +20; the cap of 4,000 pairs (about 90 minutes)
  holds a gain down to about +10 Elo.
- **Prediction (80%).** +17 Elo [+9, +25]; H1 accepted before the cap, P = 0.9.
- **Verdict rule (frozen).** H1 accepted: B.8 delivered as measured. H0 accepted, or the
  cap without a decision: the pass does not show its speed in games, and B.8 is not
  merged until the cause is found. Any engine fault or time loss past the harness's
  fault rule stops the gate and is a finding. The estimate at the stop is a
  first-crossing estimate, not the effect size.

#### RESULT

The maintainer's run, 2026-11-16, 39 minutes (`tools/results/b8-gate`, result `h1`;
the wrapper's PGN recount agrees).

| Games | W/D/L (head first) | Pentanomial | Elo | nElo | LLR |
|---:|---|---|---:|---:|---:|
| 3,938 | 1,123 / 1,908 / 907 | [38, 417, 878, 563, 73] | +19.1 ± 6.5 | +32.0 ± 10.9 | +2.96 |

- **Disposition: H1 ACCEPTED.** B.8 is delivered as measured. Zero time losses and
  zero other faults (19 allowed); 32 post-terminal games kept as evidence, outside the
  sample.
- The estimate is a first-crossing estimate, not the effect size. Prediction +17
  [+9, +25]: inside, and the gate stopped at 3,938 games against about 4,700
  expected at +16.
