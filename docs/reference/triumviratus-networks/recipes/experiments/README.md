# Experiments that did not ship

Feature blocks tried on top of `rubicon-alea-v3` (July 2026), with the same graft-and-screen method. Kept so that a
negative result can be re-checked instead of re-discovered.

| file | block | training | outcome |
|---|---|---|---|
| [`rubicon-alea-v4-coadapt-ep3-UNCONFIRMED.nnue`](../../nets/experiments/) | `Outposts` (96 features, the slot after `PassedPawns`) | co-adapted: base **not** frozen, 3 epochs | not confirmed; the block was removed. Lesson written down at the time: on this network, letting the base co-adapt first damages it and then only recovers to parity |
| [`rubicon-alea-v5-candidates-ep7-NEGATIVE.nnue`](../../nets/experiments/) | `CandidatePassers` (96 features: pawns that are not passed yet but have the majority to become passed) | frozen base, 7 epochs | negative against the zero-grafted v3; discarded |

`CandidatePassers` in detail: [`V5_CANDIDATES_PROCEDURE.md`](V5_CANDIDATES_PROCEDURE.md) (definition, index maths,
verification, procedure) and [`train_v5.sh`](train_v5.sh). The trainer side is
[`trainer/alea-fork/new_files/candidate_passers.py`](../../trainer/alea-fork/new_files/candidate_passers.py).

Both load only in a 6.0-era development engine with the fifth input slot, not in a release.
