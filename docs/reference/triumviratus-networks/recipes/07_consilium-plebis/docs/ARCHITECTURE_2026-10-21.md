# Consilium Plebis: the architecture chosen for the next network (21/10/2026)

Decided by the author on 21/10/2026, in the chat that wrote `DESIGN_NEXT_NETWORK_2026-10-21.md` (design and
measurements). This sheet is the reference for the trainer, the run and the engine. Status: **trainer being written,
training planned for the evening of 21/10/2026; the engine side is written after the launch.**

**Name (author, 22/10/2026): Consilium Plebis**, "the council of the plebs", after the Concilium Plebis of the Roman
Republic. The plebs are the pawns: in this network they have a subnetwork of their own, with the two kings. Until
22/10 the documents called it Consilium-2; file names (`nn-consilium2.nnue`, `consilium2_*`), the build flag
`TRIUMV_C2` and the `C2*` options keep the technical short form.

## 1. Decisions (author, 21/10)

- One full new run, no proxies, with every innovation in it; more than 520 G positions.
- PassedPawns v1 stays.
- **No queen buckets** (neither on the input nor in the layer stacks).
- Pawn-king subnetwork and learned routing are in the run.
- int8 first-layer weights.
- λ 0.75 is the reference.
- The engine work starts after the launch.
- A correction network with an uncertainty gate (idea with Madushan, 16/10) is a later, switchable module (UCI option),
  trained on the new network's errors, not part of this run.

## 2. Network

| part | Consilium (8.0) | **Consilium Plebis** |
|---|---|---|
| feature set (trainer name) | `Full_Threats+HalfKAv2_hm_P4^+PP_3Wide+PassedPawns` | **`Full_Threats+HalfKAv2_hm_X8^+PP_3Wide+PassedPawns`** |
| king-relative block | 4 experts by total piece count (pawns included), same for both perspectives | **8 experts per perspective**, chosen by a **learned routing table** over the non-pawn material signature |
| HalfKA weights | int16 (190 MB) | **int8**, merged weights clamped to +-127/256 in training (196 MB) |
| threats, pawn pairs, passed pawns | int8 | int8 (unchanged) |
| L1 / L2 / L3 | 1024 / 32 / 32 | 1024 / 32 / 32 |
| layer stacks, PSQT buckets | 8, (pieces - 1) / 4 | 8, (pieces - 1) / 4 (unchanged) |
| PSQT | yes | yes |
| pawn-king subnetwork | no | **yes**, 256 hidden, output added to the l1 outputs of the stack |

### 2.1 Experts (`HalfKAv2_hm_X8^`)

- signature(side) = min(N,2)·18 + min(B,2)·6 + min(R,2)·2 + min(Q,1) (54 values);
  cell(perspective) = signature(own side)·54 + signature(opponent) (2,916 cells).
- expert(perspective) = router[cell] (0..7); HalfKA index = HalfKA index + expert · 24,576 (training) / 22,528
  (export, 11 piece types).
- Pawn captures never change a cell; a non-pawn capture or a promotion changes an expert only where the table says.
- Weights in training: per-expert delta + base per king bucket shared by the experts + virtual per plane.
- Initial table: own non-pawn material in 2 classes (<= 13, > 13) x opponent's in 4 classes (<= 5, 6-13, 14-22, >= 23).
- Feature hash `0x7F234CB8 ^ 0x58384331`.

### 2.2 Learned routing (hard EM, `trainer/router_em.py`)

- Rounds at the end of chosen epochs (planned: after epochs 8, 16, 24, 32, 48, 64; then the table is frozen).
- E-step: a sample of training positions (64 x 16,384 per rank); for each position and perspective, the loss with
  that perspective moved to each of the 8 experts; differences summed per (cell, expert), all-reduced over ranks.
- M-step: reassign cells to minimise the summed loss under a **budget on expert changes per capture**. The budget is
  0.12, summed over both perspectives; today's bands give 0.262, and the initial table is measured before the run.
  Each expert keeps >= 4% of positions, at most 15% of positions move per round, and only cells with >= 2,000
  samples move. Capture transitions between cells come from game positions (`c2_transitions.npz`).
- The table is stored in the network (checkpoint buffer and .nnue section) and set in the data loader.

### 2.3 Pawn-king subnetwork

- Inputs from the side to move's view (board flipped vertically when Black moves, no mirror): 0-63 own pawns, 64-127
  enemy pawns, 128-191 own king, 192-255 enemy king; at most 18 active.
- h = clamp(b1 + Σ W1 rows, 0, 127/128) (256 values); out = b2 + W2·h (32 values), added to the stack's l1 outputs
  before the skip and the activations.
- Integer form (exact, the trainer keeps the weights on the grid): W1, b1 int16 x128; h uint8 0..127; W2 int8 x128;
  b2 int32 x128x128 = units of the l1 accumulators. W2 starts at zero.
- Engine: per-thread cache keyed by pawns, kings and side to move; computed on a miss.

### 2.4 .nnue file

Same layout as Consilium (header, feature transformer with HalfKA segment in int8 + PSQT int32, 8 stacks), then:

- router section: `RTB1`, cells (2,916), experts (8), uint8 table;
- pawn-king section: `PKN1`, hidden, outputs, int16 b1[H], int16 W1[256][H], int32 b2[32], int8 W2[32][H].

## 3. Training

- Data: the BT4-relabelled corpus of Consilium (701 GB, list in `recipes/06_consilium/README.md`),
  plus the additions being researched on 21/10. The additions stay with BT4 labels and include Fischer random/DFRC
  data.
- Positions: >= 1,000 G (author: more than 520 G). The exact epochs are set in the run script.
- λ: 0.75 reference. Pretraining descends to 0.75 early and stays there; the finetune runs at 0.75 fixed with a long
  tail at a low learning rate.
- Piece-count sampling: pc-y0 -0.20 -> 0.0 and pc-y1 0.45 -> 0.60, so the endgame experts get more data. Other values
  as Consilium: 1.0 / 0.95 / 0.75.
- int8: merged HalfKA weights clamped at the end of each epoch (as the threat weights).
- Machine: 8 GPUs of 4090/5090 class with >= 192 recent CPU threads (the loader is CPU-bound); the throughput is
  measured in the first 30 minutes (`bench_ddp.sh`).

## 4. Gates

Before the launch (trainer side, the engine comes later):

1. A Python integer evaluator reads the exported .nnue and must match the trainer's evaluation.
2. Loader indices (cells, experts, pawn-king) must match an independent python-chess computation.
3. A CPU smoke run: the loss falls, the HalfKA weights stay inside int8, one EM round runs, and the run resumes
   from a checkpoint.

After the launch, before any match: the engine must match the Python evaluator on the exported nets (bench,
nnueverify across expert changes, nnperft). Decision: SPRT at time control against Consilium, with xperf on
deterministic PGO builds.

## 5. Where the code is

- Trainer working copy: scratchpad of the chat of 21/10, folder `c2/nnue-pytorch`; it goes into
  `recipes/07_consilium-plebis/` with a patch and the run scripts.
- Design and measurements: `docs/DESIGN_NEXT_NETWORK_2026-10-21.md`, `docs/measurements_2026-10-21/`.
