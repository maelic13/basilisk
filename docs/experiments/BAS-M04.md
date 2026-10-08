# BAS-M04

<!-- part 1 of 1: from docs/EXPERIMENTS.md, 2. Measurement, harness and tuning -->

| Field | Value |
|---|---|
| ID | BAS-M04 |
| Experiment and conditions | SPSA schedule audit: iteration/game units, PowerShell `$A`/`$a` collision and perturbation resolution. |
| Result / disposition | Several schedule defects were repaired; the old runs annealed faster than intended. Accepted bakes remain accepted because they passed independent SPRTs. |
| Conditional lesson and retry trigger | A converged SPSA trajectory is not proof that its schedule was well calibrated. Assert every derived emitted value and validate integer perturbations across the whole horizon. |
| Source | legacy plan at `8dc0a24^`; `CHANGELOG.md` 1.9.2 |
