# BAS-P54

**BAS-P54 — the one-engine cohort read's null (B.8.10)**

- **Registration.** 2026-11-16, B.8.10, before the read. Agent-run on the idle host.
- **Question.** Does `tools/cohort_read.py run --engines 1` read two pools of one
  source as level? PROCESS *NPS* lets each form of the cohort read decide only after
  its own null; BAS-P37 admitted the loaded form, and this form guards B.8.10's arms.
- **Instrument.** `cohort_read.py run --engines 1` (head `e2b32b9`'s tool plus the
  guard mode): one engine at a time on the last physical core (CPU 30), each a fresh
  process, `ucinewgame` once, the first 200 positions of `game_cohort_v1.epd` at
  `go nodes 150000`, Hash 64; a cycle reads every build of both arms once, the order
  rotating; six cycles; per-build medians, `nps_read.py`'s Welch interval. Its
  disturbance rules are BAS-P37's.
- **Arms.** `b84-on` and `b84-on-null`, two independent four-build final-PGO pools of
  `60db617` (BAS-P35, BAS-P36), `bench 13` 4,688,537.
- **Prediction.** +0.0%, half-width about 0.35% (BAS-P40's level pair read −0.02%
  [−0.41, +0.37]).
- **Verdict rule (frozen, BAS-P37's admission).** Admitted when the delta is within
  ±0.5%, the interval holds 0, its half-width is 1.0% or less, and no repeat. If not
  admitted, B.8.10 stops: the guard cannot decide.

#### RESULT

Idle host, 2026-11-16, CPU 30, foreign load 0.16 to 0.19 logical CPUs, no cycle
disturbed (`tools/results/b810-null-single`).

| Arm | Mean of build medians | Cycle levels |
|---|---:|---|
| `b84-on` | 2.3932M | 2.388M to 2.398M |
| `b84-on-null` | 2.3928M | 2.385M to 2.397M |

- `b84-on-null` over `b84-on`: −0.02% [−0.24%, +0.20%], half-width 0.22%.
- **Disposition: ADMITTED.** The one-engine form guards B.8.10's arms.
