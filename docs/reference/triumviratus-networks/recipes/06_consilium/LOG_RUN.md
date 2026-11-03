# Consilium (MoE-1024): run log

Times in UTC. What was measured before and during the pretraining P, on the machine that trained the network.

## Machine (28 September 2026)

- Rented instance (vast.ai): 4× RTX 5090 32 GB, PCIe x16, all on the same NUMA node; driver 595.91.07, CUDA 13.2.
- 192 threads, 251 GB RAM, 1.4 TB disk.
- Python 3.12.14, torch 2.11.0+cu128, cupy-cuda12x 13.6.0, numpy 1.26.4, numba 0.67.0, scipy 1.17.1. Installing the
  requirements with `pip` stalled in dependency resolution (`numpy<2.0`); `uv pip install ... "cupy-cuda12x<14"` took
  about two minutes.

## Data

`download_bt4.sh /data/bt4`: **701.1 GB**, not the ~790 GB first estimated. Checked against the Hugging Face API: the
month `test80-2022-10-oct` of `linrock_relabel_1` exists both whole and split in two parts (69.1 GB, downloaded once),
and `xushawn/test80-bt4-relabel` is 17.7 GB.

| repository | files | GB |
|---|---|---|
| vondele/master-binpacks_relabel | 5 | 129.4 |
| vondele/linrock_relabel_1 | 13 | 203.9 |
| vondele/linrock_relabel_2 | 12 | 141.8 |
| vondele/from_kaggle_2_relabel | 5 | 110.6 |
| vondele/from_kaggle_1_relabel | 5 | 97.6 |
| xushawn/test80-bt4-relabel | 2 | 17.7 |

About 115-120 MB/s with 3 or 8 parallel downloads: the link (~1 Gbit/s) is the limit. The `xushawn` files are named
`*.relabel.binpack`, so the training script takes every `*.binpack` in the folder.

## Checks before the run

- Feature set `HalfKAv2_hm_P4^`: **154,576 inputs** in the engine, 162,768 in training (with the factorised features).
- Engine built with `-DTRIUMV_L1=1024 -DTRIUMV_PSQ_PHASES=4`.
- **Engine/trainer gate (`cross_check_eval`) passed** on 4,096 positions of `test80-2023-11-nov`, with a random network
  whose per-phase deltas are also random (`make_random_net.py --perturb-phases`):

  | | R² | mean error | 3σ |
  |---|---|---|---|
  | float | 0.999999 | 1.70 | 5.09 |
  | quantised | 1.000000 | 0.64 | 1.84 |

## Throughput

- **GPU ceiling per card** (synthetic batches, loader excluded, MoE features): 313k pos/s at batch 32,768, 348k at
  65,536, 369k at 131,072 (7.3-8.7 GB VRAM). With the legio-septima features 418k at 131,072: the experts cost 12% of
  GPU throughput.
- **DDP on 4 GPUs** (`bench_ddp.sh`, steady rate, compilation excluded): 1.31M pos/s at a global batch of 131,072,
  ~1.40M at 262,144, 1.50M at 524,288. Four times the per-card ceiling: the all-reduce over PCIe 5.0 costs almost
  nothing. (The Lightning progress bar averages from the start, compilation included, and under-reports the rate.)
- `NNUE_BACKWARD_TILE` 4 / 8 / 16 / 32: 369k / 358k / 352k / 352k; it stays at 4.
- A profile on synthetic batches put 71% of a step in the feature-transformer backward, which suggested rewriting it as
  a gather. **On real batches it did not hold**: only 52 features per perspective are active, the upstream backward
  takes 14.7 ms per 65,536 positions and a segmented version (`ft_backward_segmented.py`, correct to 1.4e-6) is slower
  (18.0 ms). The real limit was the loader: with `NNUE_WORKER_THREAD_RATIO=0.05` (the best value for legio-septima)
  each rank has one feature thread, about 350k pos/s with threats and experts. One card on real training: 381k pos/s
  at 0.05, 1.907M at 0.25.
- **Loader and skipping, 4 GPUs** (batch 524,288, 46 workers per rank):

  | feature-thread ratio | skip | pos/s |
  |---|---|---|
  | 0.25 | 10 | 3.40M |
  | 0.25 | 3 | 4.62M |
  | 0.15 | 3 | 4.70M |
  | **0.1** | 3 | **4.79M** |
  | 0.1 | 0 | 4.93M |

  Beyond skip 3 the GPUs are the limit (97-99%, ~300 W).
- **Cost:** about 0.17 $ per billion positions at ~3 $/h and ~5M pos/s. An 8× 3090 machine with a slower CPU was
  rejected: its loader would have been the limit.

## Smoke test

The whole `train_moe1024.sh` in small (P of 2 epochs of 52M positions, conversion to `.pt`, F with
`--resume-from-model`, export to `.nnue`) ran clean: checkpoints of 3.9 GB, LEB128 networks of 161 MB.

## Launch (28 September, 13:39:48 UTC)

```
BATCH=524288 ROOT=/root/moe/run P_EPOCHS=450 F_EPOCHS=22 nohup train_moe1024.sh > train_moe1024.log 2>&1 &
```

- **P:** 450 epochs of 1 G positions, batch 524,288, lr 8e-4, one-cycle over 858,306 steps (warm-up 5%, final divisor
  1000). P was later stopped after epoch 373 and followed by the fine-tunes F3 and F4 (see the recipe README).
- **λ:** 1.0 with a −0.3 cycle (warm-up 25%) and jitter (0.0035 per sample, 0.0070 per batch, decay 0.999).
- **Piece-count distribution** `pc-y`: −0.20 / 0.45 / 1.0 / 0.95 / 0.75.
- **Skipping:** random 2; no hard skip of openings, soft 20 (openings are needed by the ≥ 24-piece expert).
- **Loader:** ratio 0.1, 46 workers per rank, 40 binpacks (the two 3 GB `test60` files left out, for uniform sampling
  per file).
- **Exports** every 45 epochs (`export_nets.sh`); the current checkpoint on demand with `serialize_last.sh`, which
  serialises on the CPU without touching the GPUs.
- Steady rate: 201 s per epoch, **4.97M pos/s**, GPUs at 96-98%.

## First hours (28 September, 13:40-21:15 UTC)

- Speed fell by 3% (9.25 to 8.95 it/s): boost clock about 40 MHz lower and the software power limit, at 63-68 °C and
  265-310 W of 575.
- Learning rate: cosine one-cycle, peak 8e-4 near epoch 22; above 90% of the peak until about epoch 110, 84% at 135,
  54% at 225, 23% at 315.
- Loss per epoch: 0.0081 (7), 0.0047 (20), 0.0041 (27), 0.0038 (42), 0.0042 (80), 0.0044 (110); it rises as the λ cycle
  moves weight onto the game result.
- **Strength against legio-septima** (same 8.0 search on both sides, 1 thread, 64 MB, UHO 2024):

  | epoch | TC | games | Elo |
  |---|---|---|---|
  | 30 | 20+0.2 | 589 | −121 ± 21 |
  | 35 | 20+0.2 | 2,000 | −113 ± 11 |
  | 44 | 20+0.2 | 2,000 | −86 ± 11 |
  | 53 | 30+0.3 | 1,774 | −76 ± 11 |
  | 64 | 30+0.3 | 1,001 | −56 ± 14 |
  | 71 | 30+0.3 | 1,012 | −70 ± 14 |
  | 80 | 30+0.3 | 2,000 | −56 ± 11 |
  | 99 | 30+0.3 | 1,159 | −44 ± 14 |
  | 110 | 30+0.3 | 2,000 | −58 ± 11 |

  Strength rises quickly up to about epoch 60, then plateaus while the learning rate is at its peak. A logarithmic
  forecast (−32 at epoch 71) failed: the result was −70. The whole curve to the end of training is in
  [`elo_trajectory.png`](elo_trajectory.png).
- Criteria fixed in advance: at least −30 at epoch 225 (otherwise the run is examined), near parity at epoch 315, the
  verdict on the final network.
