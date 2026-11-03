# 06 — Consilium (Triumviratus 8.0, trained 28–29 September 2026)

`legio-septima` with its king-relative block split into **four experts by material phase**. The full narrative — why,
the design, every measurement — is in the engine's
[`NETWORKS.md`](https://github.com/Tors3/Triumviratus/blob/main/NETWORKS.md). This folder holds what is needed to
rerun it.

| | |
|---|---|
| File | [`nets/nn-consilium.nnue`](../../nets/nn-consilium.nnue) (Git LFS, SHA-256 `8fc004b0…`) |
| Status | finished: pretraining P (374 epochs × 1 G positions), fine-tunes F3 (80) and F4 (60); the file is the average of F4 epochs 55–59, feature transformer permuted |
| Result | Triumviratus 8.0 release build against the official 7.0: **+27.3 ± 8.3 Elo** at 15+0.15 (2,000 games) |
| Architecture | SFNNv16 with `HalfKAv2_hm_P4^`: `Full_Threats + HalfKAv2_hm_P4^ + PP_3Wide + PassedPawns`, L1 1024 |
| Inputs | 154,576 (162,768 in training, with the factorised features) |
| Trainer | official `nnue-pytorch` at **`9f72946`** + [`trainer/triumviratus_trainer.patch`](../../trainer/triumviratus_trainer.patch) (PassedPawns, `HalfKAv2_hm_P4`, `NNUE_BACKWARD_TILE`) |
| Engine | Triumviratus 8.0 built with `-DTRIUMV_PSQ_PHASES=4` |
| Hardware | 4× RTX 5090 (vast.ai), 192 threads, 251 GB RAM; ≈ 4.97 M positions/s, ≈ 3 $/h; 105 $ for the whole run |

## Strength across training

![Consilium against legio-septima across training](elo_trajectory.png)

Each point is an exported epoch played against `legio-septima` with the same 8.0 search, so only the network changes.
Early points at 20+0.2, then 30+0.3; from epoch 157 the eval scale is calibrated per phase. The trend of the 30+0.3
points is +3.8 Elo every 10 epochs and reaches parity near epoch 240; after the λ descent (epochs 292–332) the curve is
flat around zero, and the F3 fine-tune from epoch 373 brings the gain (+14.5 at 90+0.9 over 720 games, with the SPSA
vector of the search). Vector version:
[`elo_trajectory.svg`](elo_trajectory.svg).

## Files

| file | what it does |
|---|---|
| [`LOG_RUN.md`](LOG_RUN.md) | the run log: machine, data, gate, throughput measurements, launch and first hours |
| [`setup_moe1024.sh`](setup_moe1024.sh) | environment, patched trainer, engine build with 4 phases |
| [`download_bt4.sh`](download_bt4.sh) | the 42 BT4-relabelled binpacks, 701 GB |
| [`make_random_net.py`](make_random_net.py) | random network with `--perturb-phases` for the engine/trainer gate |
| [`eqfreq.py`](eqfreq.py), [`bucket_occupancy.py`](bucket_occupancy.py), [`pp_active.py`](pp_active.py) | measurements behind the phase cut points and feature counts |
| [`bench_ddp.sh`](bench_ddp.sh) | real DDP throughput at a given batch, workers and skip (steady rate, compile excluded) |
| [`verify_ft_backward.py`](verify_ft_backward.py) | feature-transformer backward timed on real batches |
| [`train_moe1024.sh`](train_moe1024.sh) | pretraining P (with the lambda fix of epoch 292) and periodic exports |
| [`train_moe1024_F2.sh`](train_moe1024_F2.sh) | the gentle fine-tune planned first (30 epochs, 6.5e-5); superseded by F3, never run |
| [`train_moe1024_F3.sh`](train_moe1024_F3.sh) | F3: 80 epochs from P's epoch 373, peak 1.6e-4, 1 % warmup, λ 0.75 |
| [`train_moe1024_F4.sh`](train_moe1024_F4.sh) | F4: 60 epochs from F3's end, peak 6e-5, new seed, λ 0.75 |
| [`avg_perm_F4.sh`](avg_perm_F4.sh) | the final file: snapshots of F4 epochs 55–59, weight average, `.nnue`, feature-transformer permutation |
| [`export_nets.sh`](export_nets.sh), [`serialize_last.sh`](serialize_last.sh) | `.nnue` from saved checkpoints / from the live checkpoint, on CPU |

## Launch

```bash
bash setup_moe1024.sh $HOME/moe
bash download_bt4.sh /data/bt4 850 3
# gate first: engine vs trainer on a random net with random per-phase deltas
BATCH=524288 ROOT=/root/moe/run P_EPOCHS=450 nohup ./train_moe1024.sh > train_moe1024.log 2>&1 &
# P was stopped after epoch 373; then, one after the other:
F3_FROM=<P checkpoint> nohup ./train_moe1024_F3.sh >> train_moe1024.log 2>&1 &
WAIT_PID=<F3 pid> nohup ./train_moe1024_F4.sh >> train_moe1024.log 2>&1 &
WAIT_PID=<F4 pid> nohup ./avg_perm_F4.sh >> avg_perm_F4.log 2>&1 &
```

Key settings of P, in `train_moe1024.sh`:
- batch **524,288**, lr **8e-4** (4e-4 at 131,072 × √4), one-cycle, 5 % warmup, final divisor 1000;
- λ: 1.0 with a −0.3 cycle to epoch 292, then a linear descent to **0.75** by epoch 332, then fixed;
- jitter 0.0035 / 0.0070, decay 0.999;
- `pc-y` −0.20 / 0.45 / 1.0 / 0.95 / 0.75;
- `random-fen-skipping 2`, `soft-early 20`, no hard early skip (the ≥ 24-piece expert needs openings);
- `NNUE_WORKER_THREAD_RATIO=0.1`, 46 workers per GPU (0.05 starves the GPUs: one feature thread per rank);
- binpacks under 5 GB excluded (40 of 42 used).

Check after `avg_perm_F4.sh`: the plain and the permuted `.nnue` must give the same `bench` on the same engine binary
(206269 on the 8.0 source before its SPSA was baked).

## Following the run

From a local machine: [`tools/fetch_net.ps1`](../../tools/fetch_net.ps1) downloads the exports (and with `-Last`
serializes the live checkpoint first); [`tools/match_moe.ps1`](../../tools/match_moe.ps1) plays a checkpoint against
`legio-septima` with the same 8.0 search on both sides.
