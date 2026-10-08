# BAS-M09

<!-- part 1 of 1: from docs/EXPERIMENTS.md, 2. Measurement, harness and tuning -->

| Field | Value |
|---|---|
| ID | BAS-M09 |
| Experiment and conditions | **A.7.2 4T Colosseum null, withdrawn before exposure by maintainer decision 2026-10-01.** The prepared fixed-N 10,000-game identical-binary run used Colosseum CLI 0.2.0 SHA-256 `5A4ECEDBF34A69ECA4D5D1F1E110D163D7C2015B7548D34E4CDA622EE5ABC408`, the same released binary and pin as the Rarog snapshot. Its accepted dry run resolved concurrency 1, shared allocation, four physical cores per arm on the same non-ponder CPU set, one headroom core and no asymmetry. |
| Result / disposition | **No games run; no null result exists.** The maintainer explicitly accepted Colosseum's Rarog verification and directed Basilisk to continue. The pinned Rarog record says the harness is qualified in its own repository and consumers repeat no qualification; its corrected symmetry run reads −0.0 ± 2.0 and its independent fastchess parity runs agree inside their intervals. This waives Basilisk's duplicate null only; it transfers no Rarog engine verdict. |
| Conditional lesson and retry trigger | Repeat qualification after a runner, scheduler, topology or adjudication change. The unrun recipe remains historical provenance, not an active experiment. |
| Source | `docs/reference/rarog/PLAN.md` B.2.6.1; maintainer decision 2026-10-01; PLAN A.7.2 |
