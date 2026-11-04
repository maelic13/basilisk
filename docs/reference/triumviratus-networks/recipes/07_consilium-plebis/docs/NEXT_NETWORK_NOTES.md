# Notes for the network after Consilium Plebis (24/10/2026)

Consilium Plebis keeps its architecture to the end of P and F (author, 24/10/2026). These are the lessons of its
first 260 epochs and of the speed work on the engine, to use when the next network is designed. Measurements in
docs/rete/C2_SPEED_2026-10-23.md, docs/rete/c2_holdout_loss_2026-10-24*.txt and the memory of the sessions.

## Speed of the network (engine cost per node)

- **Pawn-king subnetwork: a smaller hidden layer.** On game positions only 6.9 of its 256 hidden units are active
  after the clamp (ep246, 49,594 positions), yet every unit is used somewhere (each active in 1-5% of positions, 164
  units cover 90% of the activations; no unit is provably dead). The subnetwork matters: without it the evaluation
  changes by 14% on average, 16% in endgames with up to 6 men and in pawn endgames. Proposal: 64-128 hidden units,
  to be checked against 256 on loss and Elo. Engine-side shortcuts did not help: int8 W1 (no gain) and an
  incremental first layer kept per stack state (-0.6% nps, reverted 24/10).
- **Router: a tighter switch budget.** Consilium Plebis switches expert 0.188 times per evaluation, Consilium's
  piece-count bands 0.098: each switch is an accumulator refresh. It is the largest C2-only cost left. The learned
  routing stayed inactive (< 1% of the cells moved), so the budget acts on the initial table: classes chosen so that
  common captures cross fewer boundaries.
- **Permutation of the L1 neurons at every export** (as for Consilium, ftperm): ep208, zero blocks of the first layer
  from 63.9% to 71.0% on 250k positions, +2.5% nps on the Xeon, evaluations identical. It must be applied to every
  network that is used, the final one above all.
- **int8 HalfKA rows** paid off: the incremental updates read half the bytes (the one part where C2 is cheaper
  than Consilium).
- On a VNNI CPU (Zen 4) the C2 extra is instructions (+7.8% at equal IPC before the speed patch): reducing work
  matters more than reducing memory traffic there.

## Training

- **Lambda ramp.** With lambda falling from 1.0 to 0.75 until ep165 the training loss rose for 100 epochs; that was
  the target moving, not a worse network (Elo at fixed nodes rose in the same epochs). A shorter ramp, or a fixed
  lambda from the start, makes the loss readable earlier.
- **Holdout loss at fixed lambda** (c2_val_loss_fixed.py on t91_sample, runs on the local Windows trainer) is the
  check against memorisation: from ep166 to ep246 it fell 2.8% at lambda 0.75, like the training loss.
- **Data use.** 700 GB of filtered BT4 binpacks, about 100-150 billion positions (to be counted exactly); at ep250 the
  run had trained on about 250 billion positions, each used 1-3 times. More epochs still bring nearly new positions.
- **Snapshots at high LR are noisy** (+-10 Elo between neighbouring exports): test an export every 25-30 epochs, or a
  head-to-head of two exports, rather than every few epochs.
