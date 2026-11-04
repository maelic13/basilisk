# 01 — `rubicon-v1` (Triumviratus 4.2, June 2026)

The project's first own-lineage network: the first release with no Stockfish network shipped.

| | |
|---|---|
| File | [`nets/nn-rubicon-v1.nnue`](../../nets/nn-rubicon-v1.nnue), SHA-256 `29142e9a…d7e137` |
| Architecture | `HalfKAv2_hm^` (factorised), **L1 2560, L2 15, L3 32**, 8 PSQT buckets / layer stacks — the SFNNv8 generation |
| Trainer | `nnue-pytorch` at **`2db3787`**, the last commit with the L1-2560 SFNNv8 layout the 4.2 engine reads |
| Data | Leela Chess Zero `test80`, 16 months (see below), ≈ 138 GB of binpacks |
| Hardware | Google Cloud `g2-standard-4`: 1× NVIDIA L4, 4 vCPU |
| Speed | ≈ 7.4 it/s × 16,384 = **≈ 120 k positions/s**, GPU at 96 %, ≈ 15 min per epoch |
| Result | ≈ **−39 Elo** against the Stockfish network it replaced (`nn-b1a57edbea57`) |

## Data

Linrock's `test80` datasets on Hugging Face, `.min-v2.v6` versions (already filtered):
- [`linrock/test80-2022`](https://huggingface.co/datasets/linrock/test80-2022): June – September 2022;
- [`linrock/test80-2023`](https://huggingface.co/datasets/linrock/test80-2023): June – December 2023;
- [`linrock/test80-2024`](https://huggingface.co/datasets/linrock/test80-2024): January – June 2024.

Download pattern (one month):
```bash
hf download linrock/test80-2023 test80-2023-06-jun-2tb7p.min-v2.v6.binpack.zst --repo-type dataset --local-dir .
zstd -d --rm test80-2023-06-jun-*.zst
```

## Recipe

The first attempt failed, and the reasons are worth keeping:

| | first run | the run that shipped |
|---|---|---|
| features | `HalfKAv2_hm` (**no factoriser**: "Num virtual features: 0" in the log) | `HalfKAv2_hm^` (768 virtual features) |
| lambda | 1.0 → 1.0 | 1.0, then a short squeeze 1.0 → 0.75 on the best checkpoint |
| length | stopped at epoch 124 on a flat validation loss | ≈ 400 epochs |
| result | **−102 Elo** against the Stockfish net | **≈ −39** |

The shipped run, as documented at the time:
```bash
python train.py data/*.binpack --gpus 1 --batch-size 16384 --num-workers 14 --random-fen-skipping 3 \
  --epoch-size 100000000 --features='HalfKAv2_hm^' --max_epochs 400
# lr ≈ 8.75e-4 (trainer default of that commit), gamma 0.992
```
then the lambda squeeze on the peak checkpoint (`--start-lambda 1.0 --end-lambda 0.75`, resumed from it) and
`serialize.py <ckpt> nn-rubicon-v1.nnue --features='HalfKAv2_hm^'`.

<sub>Two sources of the time disagree on one point. The published record says the main run used λ 1.0 and the
squeeze came after; a working note written when the corrected run was launched gives
`--start-lambda 1.0 --end-lambda 0.75` on the main run itself. The exact command of the finishing squeeze was not
archived.</sub>

## Lessons that carried over

- **A flat validation loss is not a stopping criterion.** At epoch 120 the loss had flattened while the learning rate
  was still 38 % of its initial value; the network kept gaining strength until the rate decayed.
- **Check the recipe against the official one before paying for a run.** Two of the three failures (no factoriser,
  no WDL mix) were visible in the command line.
- **Cloud hygiene:** stopping the VM keeps the disk, deleting it does not. Copy checkpoints off first, with checksums.
