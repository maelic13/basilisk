# Triumviratus — Networks

Every NNUE network trained for the [Triumviratus](https://github.com/Tors3/Triumviratus) chess engine, from the first
one the project trained itself, with what is needed to reproduce each of them: the data (every binpack and where it
comes from), the trainer and our changes to it, the exact commands and hyper-parameters, the scripts, and the
measurements that decided what shipped.

The idea is the one of Stockfish's own networks and test-configuration repositories: a network is not only a file,
it is a recipe and a record.

## The networks

| # | Network | Release | Architecture | Inputs | Size | Recipe |
|---|---|---|---|---|---|---|
| 1 | [`nn-rubicon-v1.nnue`](nets/nn-rubicon-v1.nnue) | 4.2 | `HalfKAv2_hm^`, L1 2560 (SFNNv8) | 22,528 | 60.4 MB | [01](recipes/01_rubicon-v1/) |
| 2 | [`nn-rubicon-alea-v1.nnue`](nets/nn-rubicon-alea-v1.nnue) | 5.0 | SFNNv13: `Full_Threats + HalfKAv2_hm^`, L1 1024 | 83,248 | 89.6 MB | [02](recipes/02_rubicon-alea-v1/) |
| 3 | [`nn-rubicon-alea-v2.nnue`](nets/nn-rubicon-alea-v2.nnue) | 6.0 | + `PawnPair` | 87,808 | 94.3 MB | [03](recipes/03_rubicon-alea-v2/) |
| 4 | [`nn-rubicon-alea-v3.nnue`](nets/nn-rubicon-alea-v3.nnue) | 6.0 | + `PassedPawns` | 87,904 | 94.4 MB | [04](recipes/04_rubicon-alea-v3/) |
| 5 | [`nn-legio-septima.nnue`](nets/nn-legio-septima.nnue) | 7.0 | SFNNv16: `Full_Threats + HalfKAv2_hm^ + PP_3Wide + PassedPawns`, L1 1024 | 86,992 | 92.4 MB | [05](recipes/05_legio-septima/) |
| 6 | [`nn-consilium.nnue`](nets/nn-consilium.nnue) | 8.0 | SFNNv16 with the king-relative block split into 4 experts (`HalfKAv2_hm_P4^`), L1 1024 | 154,576 | 170.3 MB | [06](recipes/06_consilium/) |
| 7 | [`nn-consilium-plebis.nnue`](https://github.com/Tors3/Triumviratus-Networks/releases/download/consilium-plebis/nn-consilium-plebis.nnue) | 8.1 | SFNNv16 with 8 king-relative experts per perspective, chosen by the material each side faces (`HalfKAv2_hm_X8^`, int8), plus a pawn-king subnetwork, L1 1024 | 244,688 (+ 256 pawn-king) | 255.7 MB | [07](recipes/07_consilium-plebis/) |

SHA-256 of every file: [`nets/SHA256SUMS`](nets/SHA256SUMS). The `.nnue` of `legio-septima` is byte-identical to the
one embedded in the Triumviratus 7.0 release. `nn-consilium.nnue` is above GitHub's 100 MB file limit and is stored
with **Git LFS** (`git lfs install` before cloning, or download it from its file page). `nn-consilium-plebis.nnue` is
attached to the release [`consilium-plebis`](https://github.com/Tors3/Triumviratus-Networks/releases/tag/consilium-plebis)
of this repository, with its float weights (SHA-256 `b069fc089421bdf2dfcc5d7ea4de2345d7af4795cd996373c07d5f5cf67e0670`).


**How they compare**, each measured with the engine it shipped in:

| Network | Result |
|---|---|
| `rubicon-v1` | ≈ −39 Elo against the Stockfish network of the time: the cost of being own-lineage |
| `rubicon-alea-v1` | −40 Elo against the strongest Stockfish network of the time at 20+0.2; Triumviratus 5.0 ≈ +50 over 4.2 |
| `rubicon-alea-v2` | +18.3 ± 9.9 Elo over v1, network isolated (20+0.2) |
| `rubicon-alea-v3` | +7.0 ± 6.6 Elo over v2, network isolated (15+0.15) |
| `legio-septima` | +23.4 ± 9.2 Elo over v3, network isolated (15+0.15) |
| Consilium | +23.4 ± 12.0 Elo over `legio-septima`, network with its eval-scale calibration and first SPSA, same search (end of F3, 12+0.12); the 8.0 release build with the final network: +27.3 ± 8.3 over the official 7.0 (15+0.15) |
| Consilium Plebis | +10.3 ± 4.3 Elo over Consilium, 30k nodes per move, the same search source on both sides built for each network (8,332 games) |

"Network isolated" means the same engine binary on both sides with only the `.nnue` swapped (or a zero-grafted copy of
the older net, which evaluates identically but loads in the same binary).

### Other files

- [`nets/intermediate/`](nets/intermediate/) — checkpoints the measurements refer to:
  - `rubicon-alea-v2_ep459` — the single checkpoint the shipped v2 (an average of the last three) was gated against;
  - `legio-septima_stage2_ep696` — the checkpoint that measured +28.0 over v3, then lost the direct match to epoch 799.
- [`nets/experiments/`](nets/experiments/) — feature blocks that were tried and not shipped. Kept so that a
  negative result can be re-checked. See [`recipes/experiments/`](recipes/experiments/).

### Charts of the Consilium Plebis run

[`charts/consilium-plebis/`](charts/consilium-plebis/): training loss of the pretraining P, the finetune F and its
continuation, loss on held-out positions against Consilium, evaluation scale and Elo of the exports. The same charts
and their scripts are in the recipe, [`recipes/07_consilium-plebis/`](recipes/07_consilium-plebis/).

## Using a network

Triumviratus loads a network with the UCI option `EvalFile`. Each network needs an engine that knows its input
blocks:

| Network | Loads in |
|---|---|
| `rubicon-v1` | Triumviratus 4.2 |
| `rubicon-alea-v1` | Triumviratus 5.0 / 5.1 |
| `rubicon-alea-v2` / `v3` | Triumviratus 6.0 (its reader loads both formats) |
| `legio-septima` | Triumviratus 7.0 and later |
| Consilium | 8.0 built with `TRIUMV_PSQ_PHASES=4` |
| Consilium Plebis | Triumviratus 8.1 (its default network file, `nn-consilium-plebis.nnue` beside the binary) |

## Reproducing a network

Each folder under [`recipes/`](recipes/) has a README with the data, the command lines, the hyper-parameters, the
hardware, the timeline and the measurements, plus the scripts that were actually run.

- **Trainer:** always Stockfish's [`nnue-pytorch`](https://github.com/official-stockfish/nnue-pytorch), with our
  changes as patches in [`trainer/`](trainer/) (which base commit each network used is in its recipe).
- **Data:** every binpack is public — Stockfish self-play published by Joost VandeVondele, Leela Chess Zero data
  published by linrock and others, and their BT4 re-labellings, all on Hugging Face or Kaggle. The one exception is
  a small slice of our own self-play used in `rubicon-alea-v2` (≈ 6.5 % of that mix), which is not in this
  repository.
- **Tools:** [`tools/`](tools/) holds the scripts used to follow a run from a local machine and to play a checkpoint
  against the previous network.

<sub>Some scripts and logs are in Italian, as they were written during the runs. Machine-specific details (cloud
project IDs, hostnames, keys) have been removed; everything else is as it was run.</sub>

## Provenance and license

The network **weights** are trained by the project, from scratch, with no Stockfish network used as a seed or teacher.
The evaluation code, the base architecture and the trainer are Stockfish's (GPLv3), extended with our own input
blocks. The pawn-pair block is Jonathan Hallström's idea (Pawnocchio), now Stockfish's `PP_3Wide`; `PassedPawns` is
original to this project.

Everything here is distributed under the **GNU GPL v3**, like the engine.
