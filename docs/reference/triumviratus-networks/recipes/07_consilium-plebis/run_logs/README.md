# Consilium Plebis: run logs (21-28/10/2026)

Light files copied off the training VM. The final network is published as `nn-consilium-plebis.nnue` in the release
[`consilium-plebis`](https://github.com/Tors3/Triumviratus-Networks/releases/tag/consilium-plebis) of this repository,
with its float weights; the other networks and the checkpoints are kept by the author in the archives described
below. Their md5 sums are listed here.

## metrics/

Lightning `metrics.csv` of every run version that logged (the odd versions are the second DDP rank, which logs
nothing). A resume repeats the epoch label of its checkpoint while the global step goes on, so the charts number the
F epochs by step: step-epoch = global step / 3814 - 1.

| file | epoch labels | global steps | content |
|---|---|---|---|
| `P_version_0.csv` | 0-7 | 3,814-30,512 | P, first start (21/10) |
| `P_version_2.csv` | 7-19 | 34,326-80,094 | P after the first restart |
| `P_version_4.csv` | 19-470 | 83,908-1,804,022 | P, main stretch |
| `P_version_6.csv` | 470-496 | 1,807,836-1,907,000 | P from the epoch-470 checkpoint to the stop after epoch 496 |
| `F_version_0.csv` | 0-32 | 3,814-125,862 | F from P's epoch 496, to the cut of the flat stretch (27/10 08:22 UTC) |
| `F_version_2.csv` | 32-58 | 129,676-228,840 | F with the flat stretch cut to 10 epochs, to its end (27/10 15:20 UTC) |
| `F_version_4.csv` | from 58 | from 232,654 | second cycle (copied while it runs; replaced at its end) |

## vm/

- `train_c2.log`: the driver log of the whole run (data check, P, F and its second cycle), with the progress lines of
  the trainer. Copied while the second cycle runs; replaced at its end.
- `export.log`: every periodic export, checkpoint to `.nnue`, with the weight histograms of the serializer.
- `setup_c2.log`, `download_c2.log`, `download_missing.log`, `autostart_missing_1.log`, `autostart_missing_2.log`,
  `guard_c2.log`: VM setup and data download of 21/10.

## helpers/

Logs of the scripts in `../training/vm_helpers/`: `cut_flat.log` (flat stretch cut, 27/10 08:22 UTC), `after_F.log`
(end of F, start of the second cycle, t91 and permuted copies; the last lines come from `finish_F_t91.sh`),
`after_cycle2.log`, `perm_latest.log`, `export_latest.log`, `get_next.log`, and the serializer output of the two
final networks (`F_final_step228840.ser.log`, `P_last_ep496.ser.log`).

## Final networks and checkpoints

| phase | network | md5 | checkpoint | md5 |
|---|---|---|---|---|
| P, stopped after epoch 496 (step 1,907,000) | `P_last_ep496.nnue` | `5b5465a2b5935432002cd51b923fdbda` | `P_ep496_last.ckpt` | `b04cc1f9c6baf0e05de84fca5fc89a3c` |
| P, weights F started from | | | `P_final.pt` | `a14f8b3da7b196e57ef7b439969bec30` |
| F, end of the first cycle (step 228,840) | `F_final_step228840.nnue` | `ebfd47fbfe8f0f6a77aa30a35f4fe222` | `F_final.ckpt` | `44f8058cf71c96a75ae3d3765b673686` |
| F, float weights of its end | | | `F_final.pt` | `263b29e923f8d3f4d87cf91236a5f4e7` |
| continuation of F, last epoch (step 240,282) | `FR3_final_step240282.nnue` | `1d3d71430c8950f7131aee4d74ada728` | `FR3_ep62_last.ckpt` | `7f5267974f657a5631c012846a69f9d9` |
| continuation, average of its last 2 epochs | `FR3_avg2_step240282.nnue` | `8845b60195587c48be46356d7a2758c9` | | |
| **final network**: average of the continuation's 3 epochs | `FR3_avg3_step240282.nnue` | `e82ee48fbe35fbfbf57680b7ba7eed33` | `FR3_avg3.pt` | `f2e1f264b3ef78d841c40901b0bc10bf` |

The second cycle of F and the restart at 0.5% were deleted on the author's order (only their metrics are kept).

FT-permuted copies for the engine (same function, rows reordered for the sparse first layer; same bench as the
plain file):

| network | permuted copy | md5 | permutation data |
|---|---|---|---|
| P, epoch 486 (the default network of the 8.1 development builds until the final one) | `P_last_ep486_perm.nnue` | `808b0e1a352910d047a5e00da4835c6e` | t91 sample |
| P, epoch 496 | `P_last_ep496_perm.nnue` | `9dffc5b1de3b99100ff74503dd2e092b` | t91 sample |
| F, end of the first cycle | `F_final_step228840_perm.nnue` | `3c25e3a8197d9909e4f935c2c623ad4f` | t91 sample |
| final network | `FR3_avg3_step240282_perm2m.nnue` | `156fbe8b01fff176a9c1a2126828f6f9` | 2,000,000 BT4 positions (`vm_helpers/perm2m.sh`): zero blocks 52.7% -> 61.7%; 1M t91 positions gave 52.9% -> 61.9% |

## Archives (28/10/2026)

Before the VM was released, every checkpoint, float `.pt` and network above was archived at maximum ratio
(`vm_helpers/compress_all.sh`, `compress_big.sh`, `compress_ckpt.sh`): the float files split into byte planes by
`vm_helpers/bytesplit.py` (sign and exponent bytes together) and then `xz -9e` (68% of the original for the full
checkpoints, 69-70% for the `.pt` files, against 68-79% for plain xz and 71-79% for zstd -22 on the test slices of
`ctest.sh`), the networks `xz -9e` (66-67%). Every archive was restored and its md5 compared with the original's;
the list is [`archive_MANIFEST_2026-10-28.md5`](archive_MANIFEST_2026-10-28.md5) (original md5, archive md5, sizes).
The author keeps the archives; only the final network stays uncompressed (permuted `.nnue` and float `.pt`). The
published permuted file `nn-consilium-plebis.nnue` is `FR3_avg3_step240282_perm2m.nnue` (md5
`156fbe8b01fff176a9c1a2126828f6f9`).
