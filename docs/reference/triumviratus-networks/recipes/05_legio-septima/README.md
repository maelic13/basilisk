# 05 — `legio-septima` (Triumviratus 7.0, July–September 2026)

The first network trained **whole and from scratch** on the new architecture, instead of grafting blocks onto a
frozen base. New architecture, new lineage, new name: Caesar's Seventh Legion, and the 7 of the release.

| | |
|---|---|
| File | [`nets/nn-legio-septima.nnue`](../../nets/nn-legio-septima.nnue), 92,417,127 bytes, SHA-256 `b04e2835f538861bf26b7cc9e27c65911af420974f0ddab818ab0d3adf8e3ecc` — identical to the network embedded in Triumviratus 7.0 |
| Architecture | SFNNv16: `Full_Threats + HalfKAv2_hm^ + PP_3Wide + PassedPawns`, L1 1024, L2 32, L3 32, 8 layer stacks |
| Inputs | `Full_Threats` 59,808 · `HalfKAv2_hm` 22,528 · `PP_3Wide` 4,560 · `PassedPawns` 96 = **86,992** |
| Trainer | official `nnue-pytorch` pinned at **`9f72946`** (2026-07-26) + [`triumviratus_passedpawns.patch`](triumviratus_passedpawns.patch) |
| Hardware | 4× RTX 5060 Ti (DDP) |
| Shipped | stage-2 final, **epoch 799** |
| Result | **+23.41 ± 9.22 Elo** over v3, network isolated, 1,442 games at 15+0.15 (LOS 100 %) |

Scripts: [`setup_vm.sh`](setup_vm.sh), [`download_phase1.sh`](download_phase1.sh),
[`download_phase2.sh`](download_phase2.sh), [`train_phase1.sh`](train_phase1.sh),
[`train_phase2.sh`](train_phase2.sh), [`train_phase3.sh`](train_phase3.sh), [`permuta_rete.sh`](permuta_rete.sh)
(feature-transformer permutation of the final net), [`gpu_ceiling.py`](gpu_ceiling.py) and
[`loader_ceiling.py`](loader_ceiling.py) (throughput probes).

## Why these changes

- **Upstream trainer instead of our fork:** about +28 % throughput (fused FT kernel, tiled FT backward, fused
  RangerLite, DDP tuning) and the new architecture. Our only addition is `PassedPawns`.
- **Our `PawnPair` and Stockfish's `PP_3Wide` are the same feature** (same pairs, same 4,560 inputs with the same
  3,024 never-active rows, int8). Nothing to port.
- **Fewer threat inputs** (60,720 → 59,808): SFNNv16 drops pawn→pawn threats and pawn-pusher inputs, already covered
  by the pawn-pair block.
- **Full training:** grafting can only add what a new block expresses, never relearn what the base believes.

**Gate before any training:** `cross_check_eval.py` (upstream) compares trainer-side and engine-side evaluation on the
same positions, with a random net. It catches index order, weight layout and quantization mismatches in one go.

## Stage 1 — Stockfish self-play, re-labelled with BT4

All five packs from [`vondele/master-binpacks_relabel`](https://huggingface.co/datasets/vondele/master-binpacks_relabel),
re-labelled with Leela's BT4 network so the whole run shares one label scale:

| binpack | size |
|---|---|
| `nodes5000pv2_UHO.relabel-BT4-tf13tune.binpack` | 41.1 GiB |
| `dfrc_n5000.relabel-BT4-tf13tune.binpack` (Fischer random) | 38.2 GiB |
| `multinet_pv-2_diff-100_nodes-5000.relabel-BT4-tf13tune.binpack` | 28.4 GiB |
| `wrongIsRight_nodes5000pv2.relabel-BT4-tf13tune.binpack` | 7.3 GiB |
| `fishpack32.relabel-BT4-tf13tune.binpack` | 5.5 GiB |
| **total** | **121 GiB**, ≈ 50 G positions |

Recipe: batch **131,072**, lr **2.47e-3**, gamma **0.990**, λ **1.0 → 0.75** across the run,
`random-fen-skipping 3`, epoch size 100 M. Ran to **epoch 479** of 500 (47.9 G positions, about one pass). Batch and
lr are 8× and √8× Stockfish's published 16,384 / 8.75e-4: a bigger batch amortises the fixed DDP all-reduce cost.

## Stage 2 — Leela data, re-labelled with BT4

21 binpacks, 423 GB:

| binpacks | files | size | source |
|---|---|---|---|
| `T60T70wIsRightFarseerT60T74T75T76.split_0…4.relabel-BT4-tf13tune` | 5 | ~105 GB | `vondele/from_kaggle_2_relabel` |
| `leela96-filt-v2.min.split_0…4.relabel-BT4-tf13tune` | 5 | ~95 GB | `vondele/from_kaggle_1_relabel` |
| `test80-2022-{jun,jul,aug,sep,oct,nov}-16tb7p.v6-dd[.min].relabel-BT4-tf13tune` | 6 | ~126 GB | `vondele/linrock_relabel_1` |
| `test78-2022-{01-to-05-jantomay,06-to-09-juntosep}-16tb7p.v6-dd.min.relabel-BT4-tf13tune` | 2 | ~31 GB | `vondele/linrock_relabel_1` |
| `test77-2021-12-dec-16tb7p.v6-dd.min.relabel-BT4-tf13tune` | 1 | ~18 GB | `vondele/linrock_relabel_1` |
| `T91-2026-{May,June}-6p-bp` | 2 | ~31 GB | `jshriver/t91-binpacks` |

T91 is the one set not re-labelled, on purpose: it is the Leela run that produces the BT4 nets. Only its two most
recent months are used. `test60-2021-{nov,dec}` and `test79-2022-{apr,may}` are excluded: the loader picks files
uniformly, not by size, so a 3 GB file would be read ~13 times over.

Recipe: batch 131,072, lr **1.237e-3**, gamma **0.995**, `random-fen-skipping 3`, epoch size 100 M,
**800 epochs**, seeded from the stage-1 weights with a fresh schedule. **λ 0.79 → 0.75 over the first 100 epochs,
then fixed**, so the target stops moving while the learning rate is still high.

⚠️ `val_loss` is not comparable across epochs while lambda moves (the floor rises by construction), so
`--save-top-k` by `val_loss` is unusable in an annealed run.

## Stage 3 — annealing tail (run and closed at zero)

lr 3.36e-5 → 2.0e-6, λ fixed, corpus weighted per position; stopped at epoch 169 of 350. Against its own starting
point: **−0.28 ± 5.21 Elo** over 5,024 games (15+0.15). The shipped network is the stage-2 final.

## Measurements

Against `rubicon-alea-v3`, network isolated, 15+0.15 unless noted:

| stage-2 epoch | games | Elo |
|---|---|---|
| 189 (12+0.12) | 802 | +13.00 ± 12.72 |
| 263 | 1,678 | +13.88 ± 8.58 |
| 370 | 1,180 | +17.09 ± 10.55 |
| 696 | 2,138 | +28.01 ± 7.72 |
| **799 (final)** | 1,442 | **+23.41 ± 9.22** |

696 and 799 overlap; the direct match between them says 799 is the stronger by +5.18 ± 6.03, so 799 ships
(696 is kept in [`nets/intermediate/`](../../nets/intermediate/)). Release binaries, 7.0 against 6.0:
**+25.18 ± 4.28** at 5+0.05 (9,000 games) and **+21.34 ± 6.66** at 25+0.25 (3,000 games).

## After release

The network is treated as **saturated for its size**: no gain between stage-1 epochs 249 and 416, a stage-3 tail at
zero, validation loss never above training loss. The next step was more capacity, not more epochs — see
[06](../06_consilium/).
