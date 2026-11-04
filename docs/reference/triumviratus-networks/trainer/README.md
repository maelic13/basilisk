# Trainer

Every network is trained with Stockfish's [`nnue-pytorch`](https://github.com/official-stockfish/nnue-pytorch)
(GPLv3). Our changes are kept as patches against a pinned upstream commit.

| networks | upstream commit | our changes |
|---|---|---|
| `rubicon-v1` | `2db3787` (last L1-2560 SFNNv8 layout) | none |
| `rubicon-alea-v1` | master of June 2026 | none |
| `rubicon-alea-v2`, `v3`, experiments | `89d5725` ("Towards Perfect Cross Eval (#477)") | [`alea-fork/`](alea-fork/) |
| `legio-septima` | `9f72946` (2026-07-26) | [`../recipes/05_legio-septima/triumviratus_passedpawns.patch`](../recipes/05_legio-septima/triumviratus_passedpawns.patch) |
| Consilium | `9f72946` | [`triumviratus_trainer.patch`](triumviratus_trainer.patch) (superset of the one above) |
| Consilium Plebis | `9f72946` | [`consilium_plebis_trainer.patch`](consilium_plebis_trainer.patch) (the whole trainer of the run) and, for the finetune F, [`consilium_plebis_F_piecewise_lr.patch`](consilium_plebis_F_piecewise_lr.patch) |

## `alea-fork/`

- `alea_fork_vs_89d5725.patch` — changes to tracked files: the `PawnPair`, `PassedPawns` and `CandidatePassers`
  indexing in the C++ loader, the `min_passed_pawns` streaming filter, the feature registry, and the per-block
  learning rates (`--pawnpair-lr`, `--passedpawns-lr`, `--candidatepassers-lr`) with a multi-group optimizer.
- `new_files/` — files that do not exist upstream: the three feature blocks and `cross_check_trium.py`.

```bash
git clone https://github.com/official-stockfish/nnue-pytorch && cd nnue-pytorch
git checkout 89d5725
git apply ../trainer/alea-fork/alea_fork_vs_89d5725.patch
cp ../trainer/alea-fork/new_files/{pawn_pair,passed_pawns,candidate_passers}.py model/modules/features/
cp ../trainer/alea-fork/new_files/cross_check_trium.py .
bash compile_data_loader.sh
```

## `triumviratus_trainer.patch`

```bash
git clone https://github.com/official-stockfish/nnue-pytorch && cd nnue-pytorch
git checkout 9f72946529c4187d3679014036cd22c3be419716
git apply ../trainer/triumviratus_trainer.patch
```
Adds `PassedPawns`, the four-expert `HalfKAv2_hm_P4^` feature set and the `NNUE_BACKWARD_TILE` knob.

**Before any run:** `cross_check_eval.py` (upstream) on a random network, engine against trainer. For the MoE, build
the random net with `make_random_net.py --perturb-phases`: with all per-phase deltas at zero a wrong phase index is
invisible.

## `consilium_plebis_trainer.patch`

```bash
git clone https://github.com/official-stockfish/nnue-pytorch && cd nnue-pytorch
git checkout 9f72946529c4187d3679014036cd22c3be419716
patch -p1 < ../trainer/consilium_plebis_trainer.patch
patch -p1 < ../trainer/consilium_plebis_F_piecewise_lr.patch   # only for the finetune F
```
The whole trainer of the Consilium Plebis run: the `HalfKAv2_hm_X8^` block with 8 experts per perspective and its
routing table (`RTB1` section; the EM rounds of `trainer/router_em.py` moved under 1% of the cells in the run), HalfKA clipped to int8, the
pawn-king subnetwork (`PKN1` section), the gates (`c2_gate_loader.py`, `c2_gate_int.py`) and the fixed-lambda holdout
loss (`c2_val_loss_fixed.py`). The second patch adds `PiecewiseLR`, the LR schedule of F set by `NNUE_LR_PIECES`
(stepped every optimizer step). Recipe and commands: [`../recipes/07_consilium-plebis/`](../recipes/07_consilium-plebis/).
