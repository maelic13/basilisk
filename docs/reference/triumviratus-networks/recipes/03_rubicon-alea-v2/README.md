# 03 — `rubicon-alea-v2` (Triumviratus 6.0, July 2026)

A fine-tune of `rubicon-alea-v1` with a new input block, **`PawnPair`**, grafted at zero, trained partly on our own
self-play.

| | |
|---|---|
| File | [`nets/nn-rubicon-alea-v2.nnue`](../../nets/nn-rubicon-alea-v2.nnue), SHA-256 `7316cec0…ec52c65` (FT-permuted) |
| Architecture | `Full_Threats + HalfKAv2_hm^ + PawnPair`, L1 1024, L2 31, L3 32, 8 layer stacks |
| New block | `PawnPair`: 4,560 features, int8, hash `0x50414952` — the idea of Jonathan Hallström (Pawnocchio), later adopted by Stockfish as `PP_3Wide` |
| Trainer | `nnue-pytorch` at `89d5725` + [`trainer/alea-fork/`](../../trainer/alea-fork/) |
| Hardware | Google Cloud `g4`: 1× RTX PRO 6000 Blackwell, spot |
| Result | **+18.27 ± 9.94 Elo** over v1, network isolated, 1,104 games at 20+0.2 (LOS 99.98 %) |

Scripts: [`train_gcp.sh`](train_gcp.sh) (main run), [`finetune_anneal.sh`](finetune_anneal.sh) (closing anneal),
[`setup_remote.sh`](setup_remote.sh), and [`graft/`](graft/) (graft, averaging and verification).

## 1. Graft

```bash
python graft/graft_pawnpair.py nn-rubicon-alea-v1.nnue graft/alea_v2_grafted.pt
python graft/verify_graft.py nn-rubicon-alea-v1.nnue graft/alea_v2_grafted.pt   # [A] indices, [B] zero identity
python graft/check_loader_indices.py                                            # [C] C++ loader == Python reference
```
The grafted model is **bit-identical to v1** at the start (same engine bench, zero evaluation mismatch), so training
starts from v1's strength and adds the new signal instead of relearning.

## 2. Main run: two learning rates

| | |
|---|---|
| Data | Leela `test80` **all of 2022** (unseen by v1) + part of 2023, `.min` binpacks, ≈ 327 GB, **plus our own self-play**: ≈ 67 M positions (2.3 GB) from 5.1 / v1 games, listed 10 times (`OWN_REPEAT=10`) ≈ 6.5 % of the mix |
| lr | base weights **1e-4** (gentle refine), `PawnPair` block **1e-3** (`--pawnpair-lr`, it starts from nothing) |
| Schedule | `--gamma 0.997`, λ **0.75** fixed, batch 65,536, epoch size 100 M, 1 M validation positions |
| Length | 800 epochs planned, **stopped at ≈ 470** |

```bash
bash train_gcp.sh blackwell /home/USER/data     # data/t80/**/*.binpack + data/own/*.binpack
```

**What happened.** Training and validation loss went flat at ≈ 0.0036 from epoch 80 and stayed there to 473, with the
learning rate still at 25 % of its start. `train ≈ val` means no overfit: the limit was the label noise of the data,
not the schedule. Gates between checkpoints 299…459 were all within noise, so the run was stopped at ≈ 470 instead of
burning 340 flat epochs (≈ 2.3 days).

## 3. Closing: anneal and tail average

- **Anneal** from epoch 459 with a fresh, steeper decay: base 2.5e-5 / block 2.5e-4 (the rates the main run had
  reached), γ 0.95, ≈ 50 epochs ([`finetune_anneal.sh`](finetune_anneal.sh)). Validation floor down by about one
  sigma, not the jump hoped for.
- **`over_last`**: the shipped weights are the **average of the last 3 anneal checkpoints**:
  ```bash
  python graft/overlast.py runs/alea_v2_anneal anneal_overlast.pt --n 3
  ```
- Gate, average against the single epoch 459 ([`nets/intermediate/rubicon-alea-v2_ep459.nnue`](../../nets/intermediate/)):
  **+3.81 ± 7.67** over 2,004 games (15+0.15) — parity with a positive lean, so the lower-variance average ships.

## 4. Serialization (feature-transformer permutation)

```bash
python serialize.py anneal_overlast.pt nn-rubicon-alea-v2.nnue --features "Full_Threats+HalfKAv2_hm^+PawnPair" \
  --ft_optimize --ft_optimize_data own/selfplay_own.binpack --ft_optimize_count 10000 --no-cupy
```
`--ft_optimize` reorders the L1 neurons so zeros cluster for the sparse path. Evaluation is bit-identical to the plain
net (same bench). The permutation must cover every input block, grafted ones included; permuting only the stock
columns would make the net wrong, not just slow.

## Measurements during training (10+0.1, against v1)

| checkpoint | Elo |
|---|---|
| ep 199 | +13.3 |
| ep 219 | +16.1 |
| **final (anneal average), 20+0.2** | **+18.27 ± 9.94** (1,104 games) |

Release comparison, Triumviratus 6.0 + v2 against 5.1 + v1 at 20+0.2: **+30.9 ± 9.6** (network plus search changes).

## Lessons that carried over

1. A graft converges in about 80–100 epochs, not 800. Size `--max-epochs` and the decay to the real length.
2. γ must fit the real length: 0.997 over 800 epochs left the rate at 25 % halfway through.
3. Gates below ≈ 1,500 games cannot resolve differences under 10 Elo; fix the stopping criterion before looking at
   the numbers.
4. Tail averaging (`over_last`) is a free variance reduction at the end of a run.
