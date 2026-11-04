# Brief: the next Triumviratus network (written 21/10/2026 for a new chat)

## Goal

The strongest network Triumviratus has had: input buckets on the first layer (our Consilium, "experts" loosely),
chosen in a smarter way than piece-count bands fixed by hand, with better knowledge of pawn structure and passed
pawns, with **no loss of speed** and with **int8 first-layer weights without loss of strength**. A small speed loss
is accepted only if it is greatly compensated by Elo gain (measured: SPRT at time control, not at fixed nodes).

First deliverable of the new chat: research and design, not training. A document that collects what we know about
our networks and the recent NNUE developments, compares the options (expected strength, inference speed, refresh
cost, training cost, risk) and proposes a staged plan where every stage has a measurable gate.

## Standing rules (from the author; do not re-discuss)

- Training may start in October 2026 (author, 21/10: the earlier "no retrain in October" rule is lifted); it starts
  only when the author decides, after the design.
- Lambda: 0.75 is the reference, but different lambdas are allowed if the design needs them (author, 21/10).
- The SPRT decides, not the training loss (keep trainings running, check that they learn, let the SPRT judge).
- Speed claims need xperf on deterministic PGO builds (tools/misure_xperf, run_misura.ps1); MinGW non-PGO numbers are
  provisional. Comparisons with other engines use release builds (the dev build is 10% slower).
- The MoE / Consilium idea is the author's own (from LLMs); call it input buckets on the first layer in public text.
- Trainer and training-repo additions stay publishable (READMEs in English). Chat in Italian, docs in English.
- Using Leela or other public data is fine (it is what they are published for).
- Compute: renting is expensive (vast.ai dropped for routine work); full runs are rented (4x5090 class), fine-tunes may
  run on the MI50 desktop (see memory mi50-feasibility). Plan compute explicitly.

## What we measured (20-21/10/2026)

- Per node, against Stockfish 19 (release builds, socket 0, xperf): the network costs +6.6% cycles in the middlegame
  and +3.9% in endgames, almost all in apply_tile / refresh_tile / kingstep_tile, i.e. waiting for memory on the
  HalfKA rows. The evaluation itself (instructions) is level with SF's.
- Our HalfKA rows: 4 experts x 32 king buckets x 704 = 90,112 rows in **int16** (about 190 MB in memory; with threats
  about 255 MB per process). SF19 ~100 MB; SF dev (SFNNv18) 45,056 rows in int8 (~46 MB).
- Under load (40 engines at once, like a match) SF19 is 11.4% faster than our release (single engine 7-9.5%).
  Against SF19 at 40+0.4: -22.9 +- 16.0 over 440 games, while at 100k fixed nodes we are +3.6 +- 10.0: per node the
  network + search are level, the gap is speed (about half) and time management (about half).
- Widening int8 blocks to int16 at load was measured on 17/10: +60% L3 misses, -10% speed in the middlegame. Memory
  footprint is the lever; int8 HalfKA was noted as +7.8% speed in the 8.1 speed work.
- The experts really differ: the same position evaluated by the right expert and by another one differs by sd 46 cp
  one band away (corr 0.963), 78 cp two bands away (0.904), 89 cp three bands away (0.878). Using the root's expert for
  the whole search (ExpertFix) was 5% faster but lost -27.7 +- 18.2 Elo. Expert changes inside the tree cost about 5%
  of cycles (hybrid updates about 5x a normal update at a band change).
- Output buckets: (pieces - 1) / 4, 8 stacks; expert boundaries fall inside output buckets; no systematic offset at
  output-bucket jumps (Q4 study).

## Recent NNUE developments to study (see IDEE_RETE_QUEEN_BUCKETS_2026-10-20.md)

- SF SFNNv18 (13/10/2026): K32Q2 inputs (own king 32 buckets x opponent queen present), int8 feature transformer,
  32 layer stacks (8 piece buckets x 4 queen states); about +4 STC / +3.6 LTC; single changes did not pass alone.
  K32Q4 tried and closed. Trainer: nnue-pytorch #542 (int8 with per-iteration clipping, decay of virtual weights).
- SFNNv17: PSQT accumulator removed. SF19: threat inputs, pawn-pair features (SFNNv16).
- Other engines (bullet trainer community): factorised king buckets, threat inputs, output buckets; check Stormphrax,
  PlentyChess, Viridithas, Integral, Reckless (sources in _reference/) for their current architectures.

## Directions the author wants explored

1. Smarter bucket selection than hand-picked piece-count bands: learned or data-driven assignment (clustering of
   positions by what the network does, a cheap router learned with the net but hard at inference), buckets by
   structure (pawn structure classes, queen presence, open/closed centre), perspective-dependent buckets,
   overlapping bands at training time. Constraint: a bucket must be cheap to compute and change rarely along a line
   (each change is a refresh-like update; the Finny cache is keyed by bucket).
2. Better pawn-structure and passed-pawn knowledge: today PassedPawns v1 and PawnPair features plus threats. Grafts
   of new pawn blocks onto a trained net failed (PassedRel, PassedState, outposts H0 -6.4; removed 10/10/2026); trained
   from scratch they were never tried. Pawn-structure keyed small sub-networks (pawn hash cache) are an option.
3. int8 HalfKA without loss of strength (the prerequisite to afford more buckets at the same memory).
4. No speed loss: every architecture option needs a speed estimate (bytes touched per update and refresh, refresh
   frequency) before any training. A small speed loss is accepted if it is greatly compensated by Elo gain; the
   trade is judged by an SPRT at time control (fixed-node tests hide the speed cost).

## Where things are

- Engine network module: Triumviratus_8.0/consilium/ (cn.h dimensions: L1 1024, L2 32, L3 32, 8 stacks, HalfKA int16,
  threats/pawn-pair/passed int8 rows), consilium_bridge.*, search/06_nndirty.inc (per-move changes, band per piece
  count). Block 6 report: B6_SPEED_REPORT.md; spec docs/audit_8.1/B6_SPECIFICA_CONSILIUM.md.
- Network docs: docs/rete/ (this brief, IDEE_RETE_QUEEN_BUCKETS_2026-10-20.md, NETWORKS_7.0_architettura.md,
  PROSPETTIVE_RETE_7.0.md); public repo Documents/GitHub/Triumviratus/docs/networks (NETWORKS.md,
  moe_experimental_grafts.md, architecture figures).
- Training repository (01_rubicon, 02_rubicon-alea, 03_legio-septima, 04_consilium
  with LOG_RUN.md, finetune_esperti, graft_*), local <project>/Training_NNUE (Training80, TrainingMoE1024,
  finetune_*, residual_labels, bucket_occupancy.py).
- Networks repo (public): Documents/GitHub/Triumviratus-Networks (nets, recipes, trainer, tools).
- Profiling 20/10: docs/audit_8.1/R5_PROFILAZIONE_SF19_2026-10-20.md. State of the project: STATO_PROGETTO.md.
- Memory notes worth reading first: future-proposals-2026-10-09 (P1-P6: residual feature discovery, net clustering,
  Consilium Plebis, overlapping bands, hard-example mining), neuron-study-2026-10-09 and idea-net-neuron-analysis (dead,
  saturated and duplicate L1 neurons), expert-finetune-2026-10-07 (FT1/FT2 = 0 Elo), state-2026-10-10-grafts,
  training-moe1024-repo (loader ratio lesson), mi50-feasibility-2026-10-19, feedback-* rules above.
