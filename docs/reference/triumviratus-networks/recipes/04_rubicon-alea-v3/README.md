# 04 — `rubicon-alea-v3` (Triumviratus 6.0, July 2026)

`rubicon-alea-v2` plus a second grafted block, **`PassedPawns`** — original to this project.

| | |
|---|---|
| File | [`nets/nn-rubicon-alea-v3.nnue`](../../nets/nn-rubicon-alea-v3.nnue), SHA-256 `83e30b0a…7afe081a6d` (FT-permuted) |
| Architecture | `Full_Threats + HalfKAv2_hm^ + PawnPair + PassedPawns`, L1 1024, L2 31, L3 32 |
| New block | 96 features: 48 oriented squares × {own, enemy} passed pawn; hash `0x50535344` ("PSSD"); ≈ 99 k parameters |
| Trainer | same as v2: `nnue-pytorch` at `89d5725` + [`trainer/alea-fork/`](../../trainer/alea-fork/) |
| Training | **frozen base, ≈ 4 epochs** of the new block only |
| Result | **+6.96 ± 6.56 Elo** over v2, network isolated, 2,596 games at 15+0.15 (LOS 98.1 %) |

Script: [`screen_v3.sh`](screen_v3.sh). Graft and check scripts are shared with v2: [`../03_rubicon-alea-v2/graft/`](../03_rubicon-alea-v2/graft/).

## The feature

A pawn is **passed** when no enemy pawn stands on the same or an adjacent file ahead of it, and no own pawn stands
directly ahead on the same file. Deliberately square-only: "blocked", "king-supported" and "connected" flags were
considered and rejected. The first two are already learnable through the pairwise-multiplied L1 against the HalfKA
and PawnPair blocks, and "blocked" would have broken the pawn-event-only incremental update in the engine.

## Recipe

```bash
python graft/graft_passedpawns.py v2final_plain.nnue graft/alea_v3_grafted.pt       # graft on the PLAIN v2
python graft/verify_passedpawns.py v2final_plain.nnue graft/alea_v3_grafted.pt      # [A] + [B]
python graft/check_loader_passedpawns.py                                            # [C]
bash screen_v3.sh blackwell /home/USER/data
```
which runs:
```bash
python train.py <binpacks as v2> --features "Full_Threats+HalfKAv2_hm^+PawnPair+PassedPawns" \
  --resume-from-model graft/alea_v3_grafted.pt --max-epochs 30 --epoch-size 100000000 --batch-size 65536 \
  --lr 0 --passedpawns-lr 1e-3 --gamma 0.99 --start-lambda 0.75 --end-lambda 0.75 \
  --validation-size 1000000 --network-save-period 5 --save-top-k -1
```
`--lr 0` freezes the base (Adam's step is proportional to lr). At start-up the trainer must print
`[configure_optimizers] PassedPawns block lr=0.001 (base lr=0.0)`.

Graft on the **plain** (non-permuted) v2 and apply the permutation only when serializing the final net, as for v2.

## What happened

It was meant as a cheap go/no-go probe before a full fine-tune. **It was the whole training**: the block saturates in
about four epochs.

| checkpoint | against | games | Elo |
|---|---|---|---|
| ep 4 | v2 | 2,596 | **+6.96 ± 6.56** (three independent reads, LOS 94 → 91 → 98 %) |
| ep 9 | ep 4 | — | −0.32 ± 10.29 |
| ep 14 | ep 4 | — | −8.09 ± 11.05 |

Epoch 4 ships. A data-enrichment variant (`--min-passed-pawns 1`, which streams only positions containing a passed
pawn) works and costs about 3 % throughput, but did not improve on epoch 4: the block had already converged.

## Lesson

**Adding information the network cannot infer beat adding training on information it already had.** v2 spent 400
flat epochs at its data ceiling; v3 gained 7 Elo in 4 epochs with 96 new features. A frozen-base screening is the
first test for any new feature: it costs a few hours and may already be the final training.
