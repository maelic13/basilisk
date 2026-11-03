# 02 — `rubicon-alea-v1` (Triumviratus 5.0, June 2026)

The second own-lineage network, and the first on Stockfish's threat-aware architecture.
`rubicon` is the own-lineage family; `alea` marks the threats generation ("alea iacta est", said at the Rubicon).

| | |
|---|---|
| File | [`nets/nn-rubicon-alea-v1.nnue`](../../nets/nn-rubicon-alea-v1.nnue), SHA-256 `984a27dc…5a2872` |
| Architecture | SFNNv13: `Full_Threats + HalfKAv2_hm^`, **L1 1024, L2 31, L3 32**, 8 layer stacks |
| Trainer | official `nnue-pytorch` master of June 2026 (native `Full_Threats`), unpatched |
| Recipe | Stockfish's [`vondele/nettest`](https://github.com/vondele/nettest) `threats.yaml`, scaled down |
| Hardware | Google Cloud `g4-standard-48`: 1× RTX PRO 6000 Blackwell, spot |
| Speed | ≈ 4.5 it/s at batch 65,536, ≈ 5.7 min per epoch; stage 1 ≈ 24 h |
| Result | **−40.4 ± 14.5 Elo** against the strongest Stockfish network of the time (`nn-71d6d32cb962`), 640 games at 20+0.2 |

Script: [`stage1_stage2_runbook.sh`](stage1_stage2_runbook.sh) — the runbook actually used, block by block: VM,
drivers, environment, data download, both stages.

## Stage 1 — Stockfish self-play

Data: the five public Kaggle datasets by Joost VandeVondele (≈ 136 GB):
`nodes5000pv2-u-uho`, `dfrc-u-n5000`, `multinet-u-pv-2-u-diff-100-u-nodes-5000`,
`data-u-pv-2-u-diff-100-u-nodes-5000`, `wrongisright-u-nodes5000pv2`
(`kaggle datasets download joostvandevondele/<slug>`).

```bash
TORCHDYNAMO_DISABLE=1 python train.py data/*.binpack \
  --gpus 0 --l1 1024 --l2 31 --l3 32 --features "Full_Threats+HalfKAv2_hm^" \
  --start-lambda 1.0 --end-lambda 0.75 --lr 1.5e-3 --one-cycle-steps 381500 \
  --batch-size 65536 --num-workers "$(nproc)" --epoch-size 100000000 --max-epochs 250 \
  --factorized-weight-decay 0.001 --early-fen-skipping 18 --random-fen-skipping 10 \
  --network-save-period 25 --default-root-dir runs/stage1
```
Optimizer `rangerlite`, one-cycle schedule (trainer defaults of the recipe). `TORCHDYNAMO_DISABLE=1` works around a
crash of `torch.compile` on the trainer's logging with torch 2.11 + cu128.

## Stage 2 — Leela data

Data: Leela `test80` from linrock (2023 and 2024 months, `.min-v2.v6` binpacks; see the runbook for the list).
Resumed from the stage-1 weights (`serialize.py last.ckpt stage1.pt`, then `--resume-from-model stage1.pt`):
`--lr 1.3e-3`, **λ 0.74** fixed, planned 950 epochs, closed at ≈ 600.

## Measurements

All net against net, same engine, only the `.nnue` swapped; 1 thread, 64 MB, UHO 2024 openings.

| checkpoint | against | TC | games | Elo |
|---|---|---|---|---|
| stage 1, ep 119 | Stockfish net | 8+0.08 | 120 | −55.5 ± 34.5 |
| stage 1, ep 217 | Stockfish net | 8+0.08 | 320 | −27.2 ± 23.6 |
| stage 1, ep 250 (final) | Stockfish net | **20+0.2** | 186 | **−83.8 ± 29.4** |
| stage 2, ep 415 | Stockfish net | 20+0.2 | 242 | −40.4 ± 25.8 |
| stage 2, ep 462 | stage 2, ep 415 | 20+0.2 | 600 | +11.6 ± 13.3 |
| stage 2, ep 529 | stage 2, ep 462 | 20+0.2 | 318 | +6.6 ± 19.9 |
| **final** | Stockfish net | 20+0.2 | 640 | **−40.4 ± 14.5** |

## Lessons that carried over

- **Measure networks at 20+0.2 or longer.** The same stage-1 network read −27 at 8+0.08 and −84 at 20+0.2: a
  57-Elo swing from the time control alone. Every later gate uses a long time control.
- **Anchor progress on a direct match against a fixed opponent.** Consecutive checkpoint-vs-checkpoint deltas summed
  to +71 where the direct gap closed by +28: checkpoints beat each other more than they beat a third net.
- The Stockfish recipe's staging (Stockfish data first, Leela data second) is what closed half the gap.
