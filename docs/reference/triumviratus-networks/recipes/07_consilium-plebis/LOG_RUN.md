# Consilium Plebis: run log

Times in UTC. One epoch = 1 G positions.

## 20-21/10/2026 night: design, trainer, gates

- Design and decisions: `docs/DESIGN_NEXT_NETWORK_2026-10-21.md`; frozen specification
  `docs/ARCHITECTURE_2026-10-21.md`.
- Trainer written on top of Consilium's (`patches/consilium2_trainer_vs_consilium.patch`): `HalfKAv2_hm_X8` (loader and
  model), routing table and `RouterEM`, pawn-king subnetwork, aux block in the batch, RTB1/PKN1 sections.
- Gates on the PC and again on the VM: loader against python-chess (0 mismatches on 7,025 positions), integer
  evaluator against the trainer (R2 0.9999998 random, 0.9999971 trained smoke network), GPU smoke run with an EM round
  and a resume. The first EM round on the VM stopped on a numpy bool subtraction in the assignment (fixed, unit test
  `c2_test_assign.py`); the budget went from 0.15 to 0.20 changes per capture so the rounds do not give up loss.

## Machines tried before the run

| machine | outcome |
|---|---|
| 4x RTX PRO 6000 | probe: batch 1M 4.3 M pos/s (0.37 $/G) |
| 8x RTX 5090 (PCIe 4) | batch 1M 3.5 M pos/s (communication bound), 2M 5.2-5.6 M pos/s (0.38 $/G); 4M out of memory (32 GB); one crash with the filesystem lost, restarted by the author |
| 2x RTX 5090 | first instance without internet (destroyed); copies between instances too slow (~35 MB/s) |
| 2x RTX 4090, 24 GB, EPYC 7B13, 61 CPUs (cgroup), 944 GB RAM, 1 TB disk, 1 $/h | **the run** |

Cost decided the machine: the 4090s give the lowest price per position (0.29 $/G), at the price of a run of about a
week.

## 21/10/2026: run on 2x RTX 4090

- 01:45 setup (`training/setup_c2.sh`, torch 2.11.0+cu128) and download (`training/download_c2.sh /data/bt4 12`,
  `HF_XET_HIGH_PERFORMANCE=1`, 0.4-1.2 GB/s).
- 01:57-02:06: 11 of the largest files failed with Hugging Face CAS errors ("error decoding response body"); the
  whole partial download of those files was lost. `autostart_c2.sh` fetched them again with `download_missing.sh`
  (02:06-02:21, no failure). Lesson: with 12 parallel xet downloads the big files fail near the end; the check of
  sizes against Hugging Face before the start is what saved the run.
- Author's decisions before the start: no speed probe; random skipping 2 (as Consilium); soft early skip softer than
  Consilium's (ply 0/6/10/18 keep 0.25/0.40/0.60/0.90); more endgames than Consilium through `pc-y` (unchanged from
  the design).
- 02:20:39 data complete (43 binpacks, 654 GiB). 02:20:43 P started:
  `2 GPU, 43 binpacks, batch 262144, workers/rank 28, P 550 ep (2161025 steps, lr 5.6569e-04)`, lambda schedule
  648,307 steps (30%), EM rounds after epochs 8, 16, 24, 32, 48, 64, budget 0.20.
- Loader: 28 threads per rank (21 readers, 7 feature workers, `NNUE_WORKER_THREAD_RATIO=0.25`); about 11 cores busy
  per rank: the GPUs are the limit (92-100% utilisation, 180-200 W of 450).

### Throughput and loss

| point | steps | it/s (between progress marks) | positions/s | train loss |
|---|---|---|---|---|
| epoch 0, 20% | 763 | 2.74 (includes torch.compile) | | 0.01814 |
| epoch 0, 40% | 1,526 | 3.63 | 0.95 M | 0.01384 |
| epoch 0, 80% | 3,052 | 3.74 | 0.98 M | 0.01263 |
| epoch 1, 80% | 6,866 | 3.77 | 0.99 M | 0.01047 |

One epoch takes about 17 minutes; P (550 epochs) about 160 hours and 160 $.

Consilium's P at the same number of positions (its batch was 524k): 0.01638 at 0.4 G, 0.01331 at 0.8 G
(`04_consilium/run_logs/logs_export.tgz`, `metrics_P_version_0.csv`). The two numbers are not a strength comparison:
batch, data mix and sampling differ.

### Fixed-lambda reference (holdout `t91_sample.binpack`, not in the training data)

`c2_val_loss_fixed.py`, x1e-3; Consilium Plebis networks are added as their exports come out (every 10 epochs).

| network | positions | lambda 1.0 | lambda 0.75 | lambda 0.0 |
|---|---|---|---|---|
| nn-consilium (8.0 release) | 20 x 8192 (smoke test) | 4.276 | 5.205 | 19.13 |
| nn-consilium (8.0 release) | 200 x 8192 | 4.317 +- 0.016 | 5.240 +- 0.017 | 19.14 +- 0.04 |
| Consilium Plebis ep9 (10 epochs) | 200 x 8192 | 6.814 +- 0.022 | 8.180 +- 0.023 | 23.97 +- 0.05 |
| Consilium Plebis ep19 (20 epochs) | 200 x 8192 | 5.157 +- 0.019 | 6.384 +- 0.020 | 21.70 +- 0.05 |

The script reproduces September's numbers for Consilium (ep321: 5.24 at lambda 0.75). Consilium's own trajectory
at fixed lambda starts at ep110 (`04_consilium`: lambda 1.0 4.78 / 4.68 / 4.53 / 4.34 / 4.23 at ep110 / 145 / 200 /
254 / 285), so the like-for-like comparison comes with the exports at those epochs.

On Windows the loader's stream creation fails now and then with an access violation (never on the Linux VMs), so
this comparison runs on the VM, CPU only, at the lowest priority.

### Crash at the first EM round and resume

- 04:43:47: the run stopped at the end of epoch 7, in the first routing round (`router_em.py`, `assign`): the numpy
  bool subtraction already fixed on the first VM. The trainer tarball of the package had been built before that fix
  and was never rebuilt, so this VM got the old code. Only `trainer/router_em.py` differed (checked file by file:
  the loader and the rest of the trainer were identical).
- The watcher did not see it: it looked for the training process with a command-line pattern that its own ssh
  command also contained, so it always found a match. Liveness is now read from `nvidia-smi --query-compute-apps`.
- 06:23:23: fix uploaded, `c2_test_assign.py` run on the VM, P resumed from `last.ckpt` (end of epoch 7, saved at
  04:42:58). The resume redoes epoch 7. Cost of the stop: about 1 h 40 min of idle GPUs plus the repeated epoch,
  about 2 $.
- The package tarball is now rebuilt from the current trainer (with the fix and the unit test).

### Evaluation scale of ep19 against Consilium (first data point; calibration postponed)

Matches of the other chat (same merged 8.1 search, release PGO): ep19 against Consilium -96.7 +- 15.8 at 100k nodes,
-118.5 +- 18.0 at 20+0.2; network cost 1-3% of nodes per second. Scale measured with `analysis/scale/scale_c2.py` and
`analysis/scale/parts.py` (`evalbatch` of both match binaries on 49,594 positions of those games plus the gate set): C2's psqt
matches Consilium's (0.94-1.03), its positional part is 0.61-0.88 of Consilium's, so the search eval is at scale in the
middlegame (0.94-1.05) and compressed in endgames (0.61 up to 8 pieces, 0.76 at 9-12). The author: too early to
calibrate (a young network shrinks its evals); measure again on later exports before adding a C2 output scale.

### Second stop: CUDA out of memory after the first validation

- 10:19 end of epoch 19, first validation inside the run (`--check-val-every-n-epoch 20`): val loss 0.00386. At the
  first training step of epoch 20 both ranks failed with CUDA out of memory (1 GB short: the 4090s have 22 GiB usable,
  training holds ~19.3 GB, validation left memory allocated). Idle 10:20-10:58 (the watcher of the night had ended at
  09:21 and had not been restarted).
- Fix: no validation inside the run (`--check-val-every-n-epoch 100000`; the fixed-lambda loss on t91 is measured on
  the exports, on CPU) and `PYTORCH_CUDA_ALLOC_CONF=expandable_segments:True`. 10:58 resumed from `last.ckpt` (end of
  epoch 19), GPUs back at 96-100%, 18.7 GB. A 24-hour liveness watcher (nvidia-smi compute apps) is running.
- Second routing round, end of epoch 16: 4 cells moved, crossing 0.1995 -> 0.1965, loss change -2.5e-7.

### First routing round and first export

- 06:45, end of epoch 8 (`router_em.jsonl`): 4.19 M sampled positions, mean loss 0.006416, gain 1.6e-6, 17 cells
  moved (1.3% of the positions), crossing rate 0.209 -> 0.199. Expert shares 8.8 / 7.5 / 12.2 / 2.8 / 13.3 / 12.0 /
  22.2 / 21.3%: the two experts with the most material take 22% each (the softer early skip adds openings). A small
  gain is expected this early: the experts start equal and have had 8 epochs to separate.
- 07:24, first export `P_epoch=9-step=41954.nnue` (10 epochs). Integer evaluator against the trainer: mean |d| 0.748,
  R2 0.9999981; engine `evalbatch` equal to the reference on 2,004 of 2,004 positions with the table changed by the
  round.

### Engine cost, provisional (21/10, Xeon busy with an SPSA)

`nnspeed 4` on the first 20 positions of `fens.txt` (middlegame) and `eg64.txt` (endgames), engine pinned, 2 rounds,
time per evaluation of the C2 build against Consilium (same tree):

| build | network | middlegame | endgames |
|---|---|---|---|
| PGO, all 51 + 64 positions, 4 rounds | random | +13.3% | +26.8% |
| PGO (profile trained with the random network) | ep9, EvalFile | +5.7% | +14.2% |
| MinGW, C2 | ep9 | +1.9% | +9.7% |
| MinGW, C2 without pawn-king (`-DCN_C2_PK_OFF`) | ep9 | -3.0% | +3.2% |
| MinGW, C2 with one expert (`-DCN_C2_ONE_EXPERT`) | ep9 | +4.1% | +4.7% |
| MinGW, one expert and no pawn-king | ep9 | +1.0% | +1.8% |

The random network overstated the cost (its L1 has far more non-zero inputs than a trained one). With the trained
network the middlegame cost is small and the endgame cost comes mostly from the pawn-king subnetwork (king moves miss
its cache and recompute it with plain loops): it is the first target of the engine's speed work. Rounds differ by up
to 5 points: the numbers wait for an idle machine and a PGO build with the trained network embedded (being built).

### Engine: where the cost is, and the first optimizations (21/10, morning)

Phase profile (`-DCN_PROF`, cycles of the network module, same `nnspeed` tree, ep13 network, machine busy):
middlegame +5.2% against Consilium, endgames +11.7% before the optimizations. Incremental updates (`apply`) are
slightly cheaper with int8 HalfKA; refreshes are fewer in the middlegame (pawn captures never change the expert) and
cost about 15% more each; the pawn-king subnetwork took ~3% (cache 91-94% hits, ~2,900 cycles per miss with plain
loops) plus ~1.4% for its cache lookup; the forward was +8% (middlegame) and +30% (endgames).

The forward follows the density of the young network: non-zero groups of 4 l1 inputs per evaluation, out of 256:

| network | middlegame | endgames |
|---|---|---|
| Consilium (final) | 84.7 | 91.8 |
| Consilium Plebis ep9 | 138.7 | 185.7 |
| Consilium Plebis ep13 | 90.0 | 134.1 |

Optimizations (all exact: `evalbatch` 2,004/2,004 against `c2_gate_int.py`, `nnperft` 0 bad, AVX-512 and AVX2):
1. pawn-king subnetwork in SIMD (hidden layer in registers, packus with the lanes put back in order, output with the
   forward's 4-input broadcast against a regrouped `pkW2g`, independent chains): ~2,900 -> ~1,250 cycles per miss;
   what is left is fetching its 17 KB of weights, evicted by the main network between misses;
2. pawn-king cache entries aligned to three cache lines, and the entry prefetched at the start of the evaluation.

After them (depth 3, busy machine): middlegame +5.2% (pawn-king 1.8 points, its lookup and the rest 1.9, forward 0.9,
updates 1.0, refreshes -0.4), endgames +6.9% (mostly the forward). The final number waits for an idle Xeon and PGO
builds of the optimized code.

### Fixes made during the night

- `c2_gate_int.py`: the loader must get the network's routing table before building the batch (`set_router_table`),
  as `train.py` does. Without it the trainer and the integer evaluator agreed with each other on the default table,
  and the engine (which reads the table from the file) differed on 2.6% of the positional value. Found with the
  engine gate; after the fix the engine matches on 2,004 of 2,004 positions.
- `train_c2.sh`, F phase: `--router-em-epochs 0`. After a resume from a Lightning checkpoint the model is restored only
  after the loaders start, so without the callback F would route with the default table.

## Engine side (21/10/2026)

- `patches/consilium2_engine_inference.patch`, build `-DTRIUMV_C2`: int8 HalfKA rows, expert per perspective from the
  network's table (written by the make on captures and promotions), pawn-king subnetwork with a per-thread cache,
  RTB1/PKN1 sections.
- `evalbatch` equal to `c2_gate_int.py --dump` on 2,004 of 2,004 positions; `nnperft`: 1,020,822 incremental
  evaluations equal to a full refresh.
- Cost measurement: `nnspeed D <fens>` (nnperft's tree without the checks: the work does not depend on the network)
  on two PGO builds of the same source, C2 on and off (`analysis/engine_speed/measure_nnspeed.py`). The first C2 build embeds a random
  network; the real measurement needs a trained export (its L1 sparsity differs) and an idle machine.

## 22-26/10/2026: P from epoch 25 to epoch 470

- **Exports.** Every export taken for tests was copied off the VM and checked: md5 against the VM, `nnperft` (205,573
  incremental evaluations, 0 bad). From 23/10 most were also permuted for sparsity (`serialize.py --ft-optimize`, 500k
  positions of `t91_sample`, `training/vm_helpers/perm_only.sh`): the permuted file evaluates byte for byte like the
  plain one (`evalbatch` on 49,594 positions and the bench equal), with 66-68% zero blocks against 58-64% before. On
  the GPU the permutation ran out of memory next to the training (about 3.4 GB free per card), so it runs on the CPU.
  From 26/10 the author takes plain exports again.
- **Training loss.** Lambda fixed at 0.75 from epoch 168: 0.003528 (ep168), 0.003134 (ep435), 0.003118 (ep470), a
  nearly constant slope of -0.0011 to -0.0019e-3 per epoch until epoch 435, about -0.0005 from 435 to 460 as the LR
  fell below 12% of its peak (charts in `charts/`).
- **Evaluation scale** against Consilium, 41 exports: [`docs/C2_SCALE_2026-10-22.md`](docs/C2_SCALE_2026-10-22.md).
  Neither a shared eval factor (C2EvalExp 64, 23/10) nor a side-to-move offset (C2StmOffset 6 on ep435 at 60k nodes:
  -1.2 +- 7.6 over 2,245 games, 26/10) gave anything measurable.
- **Holdout t91**, paired against Consilium on the same positions, by material and by expert:
  [`docs/C2_HOLDOUT_T91_2026-10-26.md`](docs/C2_HOLDOUT_T91_2026-10-26.md). Consilium Plebis passed Consilium near
  epoch 240; ep435 is 2.9% below at lambda 0.75, and the gain grows faster as the LR falls.
- **26/10 16:55 UTC: instance stopped** (vast.ai balance empty) at epoch 471, 80%. The author topped up; the instance
  came back at 17:12 with the disk intact and nothing running (the instance's start script is vast.ai's own). Resumed
  at 17:14 with `P_EPOCHS=550 nohup bash training/train_c2.sh >> /root/train_c2.log` from the epoch-470 `last.ckpt`:
  same 2,161,025 OneCycle steps, router table restored from the checkpoint by `RouterEM.on_train_start`. The resume
  redoes epoch 470 and logs to `lightning_logs/version_6`. A copy of the epoch-470 checkpoint is kept off the VM.

## 26/10/2026: lambda calibration (no lambda per expert)

Question: should F train each expert or piece band at its own lambda? On the VM, over the 43 BT4 binpacks of P
(`analysis/lambda_calibration/`, 3.2 M positions with more than 5 pieces), the trainer's mapping
`pf = 0.5 * (1 + sig((s - 270) / 380) - sig((-s - 270) / 380))` against the game result:

- E[result | pf] is off the diagonal in every group: the results are more decisive than pf (pf 0.33 -> 0.15,
  0.67 -> 0.84; slope 1.16 overall, 1.65 near 0.5), with a mean gap of 0.017 (6-9 pieces) to 0.066 (24-32 pieces)
  (`lambda_bt4.txt`; the t91 sample goes the same way with smaller gaps, 0.043 overall against 0.052,
  `lambda_t91.txt`).
- The out_scaling that best fits the results is 65-80 in every piece band and every expert (380 in the trainer;
  `scaling_fit_bt4.txt`): one global mismatch between the BT4 score scale and the mapping, not a bias of some phase.

So F keeps one lambda, 0.75, for every expert.

## 27/10/2026: P stopped after epoch 496, F with the author's LR schedule

- **Why.** Linear fits of the training loss over 40 epochs: -1.93e-6 per epoch at a mean LR of 78% of the peak
  (epochs 175-214), -1.32e-6 at 44%, -1.06e-6 at 23%, -0.74e-6 at 14%, -0.43e-6 at 7% (epochs 455-494). The descent
  followed the LR, so the network was still learning, and the one-cycle tail to epoch 549 would have added little.
  Consilium's F3 was the same restart (P stopped at 8.7% of its peak, F3 at 20%), and gave +14.5 Elo at 90+0.9.
- **Stop.** 27/10 00:14 UTC, right after the epoch-496 checkpoint (training loss 0.00311, LR 2.112e-5 = 3.73% of the
  peak, global step 1,907,000): the training processes stopped by PID. The checkpoint is kept on the VM
  (`P_ep496_last.ckpt`). The P exports up to epoch 486 (plain and permuted) were copied off the VM.
- **F.** Started 00:15 UTC with `training/vm_helpers/start_F.sh`: `STOP_P=1 F_EPOCHS=70 F_LR_FRAC=0.25` and
  `NNUE_LR_PIECES="7628:0.1494:1:lin;87722:1:0.6:cos;76280:0.6:0.6:lin;95350:0.6:0.00016:cos"` (fractions of
  `--lr` = 1.4142e-4, 25% of P's peak; 3,814 steps per epoch). In % of P's peak: 2 epochs linear from 3.7 to 25,
  23 epochs half-cosine to 15, 20 epochs flat at 15, 25 epochs half-cosine to 0.004 (P's final LR). Weights from
  `P_final.pt` (serialized from the epoch-496 checkpoint), fresh optimizer, lambda 0.75, routing table frozen,
  same data. The scheduler was checked on the CPU before the launch (boundaries, two parameter groups, a
  `state_dict` round trip) and both ranks log `NNUE_LR_PIECES` at start. The trainer tarball on the VM
  (`nnue-pytorch-c2.tar.gz`) carries the new scheduler. Planned end around 27/10 18:00 UTC.
- **Reading F.** The training loss first rises with the LR (3.7% -> 25%); the flat stretch at 15% (epochs 25-44)
  shows whether it keeps falling at a non-zero LR, and the final decay gives the network to test. The exports of
  epochs 9-49 come from a high LR (15-25% of the peak) and are not meant for Elo tests.
- **Flat stretch cut to 10 epochs (author, 27/10 08:22 UTC).** Over epochs 25-31 at 15% the loss was flat (mean
  0.003141, slope +0.04e-6 +- 0.72e-6 per epoch). Against P at the same LR, F was 6e-6 lower after about 100 more
  epochs: about 0.06e-6 per epoch of learning at a fixed LR, some 1e-6 over 20 flat epochs, below the epoch-to-epoch
  spread of 2.8e-6. The loss level follows the LR (3.9e-6 per point of LR between 15% and 25% of the peak; P's
  -0.78e-6 per epoch over epochs 400-460 is the LR falling 0.2 points per epoch). F was stopped after the checkpoint of
  epoch 32 (global step 125,862; copy kept on the VM) and resumed from it with `training/vm_helpers/cut_flat.sh` and
  `start_F2.sh`: the same pieces with the flat segment at 38,140 steps (10 epochs), 60 epochs of steps in all.
  The scheduler restores its step count from the checkpoint and takes the new pieces from the environment (the
  lambda is not stored in the checkpoint; checked on the VM CPU before the stop: same LR at the resume step, new
  segment boundaries after it). A resume repeats the epoch label of its checkpoint while the step count goes on,
  so the resumed run has `--max-epochs 59` and its epoch labels are one below the step-epochs used here
  (step-epoch = global step / 3814 - 1). Planned end around 27/10 15:10 UTC. Scripts of the projection and the P+F
  chart: `analysis/loss/proj_F.py` (output `proj_F_2026-10-27.txt`), `plot_c2_pf.py`.
- **Projection of the final training loss** (least squares on P from LR 30% and F after its rise, L = a + f(LR) +
  g x epoch, f linear or quadratic in LR; g = -0.06e-6 per epoch): F's last five epochs at 0.003089 (linear) to
  0.003096 (quadratic), against 0.003105 at P's epoch 496 (0.003109 over its last five). The LR range below 3.7%
  has no data from P, so the two forms differ there. The same fit puts P, had it followed its one-cycle to epoch 549,
  at the same level as F's end: most of the gap to P's epoch 496 is the cooling P did not have.
- **The descent (epochs 35-49).** The loss followed the projection to epoch 47 and then fell below it: 0.003106 at
  epoch 48 (LR 6.6% of the peak) and 0.003102 at epoch 49 (LR 5.6%), 6-7e-6 under the projection and 11-12e-6 under P
  at the same LR (P's own points, no extrapolation), of which the extra epochs explain about 4e-6. F passed P's
  epoch-496 loss with its LR still above P's last LR.
- **A second cycle, started automatically at the end of F (author's schedule and decision, 27/10).** From F's final
  checkpoint, in % of P's peak: up to 10% in 2 epochs (linear), half-cosine to 4% in 3, flat 4% for 5, half-cosine to
  0.004% in 15 (25 epochs, about 6.5 hours): `training/vm_helpers/start_cycle2.sh` (the eight pieces, F's four in
  front; checked on the VM CPU with `test_cycle2_sched.py`: 10.000%, 4.000% and 0.004% at the segment ends).
  `training/vm_helpers/after_F_auto.sh` runs at the end of F: copy and export of the final checkpoint (the run's
  export skips `last.ckpt`, and after the resume the final epoch label is not a periodic checkpoint) and of P's epoch
  496; once the F run has exited and the GPUs are free, the second cycle; then, below the training's priority, the
  paired t91 (300 batches, the 26/10 method; the VM stream is deterministic, two processes read identical batches) of
  ten networks (Consilium, P epochs 399-496, F epochs 19-59) and FT-permuted copies of the final F and of P's epoch
  496. `after_cycle2.sh` does the same at the end of the second cycle (its final network against the final F).
- **End of F (27/10 15:20 UTC).** Training loss of the last epoch 0.0030822 (step-epoch 59, LR falling to 0.004% of
  P's peak), 0.74% below P's epoch 496 (0.0031051). `after_F_auto.sh` exported the final network
  (`F_final_step228840.nnue`) and P's epoch 496 and started the second cycle at 15:20 UTC. It had started the cycle
  as its own background child, so its `wait` for the t91 processes also waited for the second cycle: it was stopped
  by PID at 15:52 UTC, with the t91 dumps complete (the training processes kept running), and `finish_F_t91.sh` wrote
  the paired results and started the permuted copies. The repository copy of `after_F_auto.sh` starts the cycle in a
  session of its own and waits only for the t91 processes. Logs, metrics and md5 sums of the finals: `run_logs/`.
- **Holdout t91 at the end of F** (paired, 300 x 8192 positions, reference P's epoch 496, which gives 4.1575, 5.0749
  and 18.9774 x1e-3 at lambda 1.0, 0.75 and 0.0; `analysis/holdout_t91/results/2026-10-27_F_end/`). F epochs are
  step-epochs (file labels 19, 39, 49):

  | network | lambda 1.0 | lambda 0.75 | lambda 0.0 |
  |---|---|---|---|
  | Consilium | +3.92% | +3.39% | +1.06% |
  | P epoch 399 | +1.66% | +1.28% | +0.18% |
  | P epoch 439 | +2.23% | +1.67% | +0.20% |
  | P epoch 479 | +0.76% | +0.65% | +0.23% |
  | P epoch 486 | +0.41% | +0.27% | -0.04% |
  | F epoch 19 | +0.32% | +0.52% | +0.49% |
  | F epoch 40 | +0.78% | +0.76% | +0.36% |
  | F epoch 50 | +0.90% | +0.63% | +0.02% |
  | F final (59) | +0.33% | +0.14% | -0.14% |

  Paired standard errors 0.01-0.04% (Consilium 0.12-0.14%); they cover the positions, not the spread between
  snapshots of the same run, which is larger while the LR is high (P's epoch 439, at 12.7% of the peak, is above its
  epoch 399 at 21.4%). At the training lambda 0.75 the final F, cooled to 0.004% of the peak, is 0.14% above P's
  epoch 496 (LR 3.7%) on t91 (+0.33% on the score term, -0.14% on the game-result term), while on the training stream
  it is 0.74% below. P's last exports fall steadily with its LR (epoch 479 at 5.8%: +0.65%; epoch 486 at 4.9%:
  +0.27%); F's epoch 50 at 4.3% is +0.63%, above P at a similar LR, and F's epoch 40 at 13.0% is +0.76% against
  +1.67% for P's epoch 439 at 12.7%. The training-loss gain of F did not carry over to t91 (positions from a different
  Lc0 run than the training data); the Elo test decides between the two networks.

- **Tail and average of five epochs (author, 27/10).** The final network is the average of the last five epochs, at
  a near-zero, stable LR: after the second cycle, 5 epochs at 0.004% of P's peak (2.26e-8; the value the cooling ends
  at, and P's own one-cycle floor), one checkpoint per epoch kept. `training/train_c2_tail.sh` is train_c2.sh with the
  checkpoint period and the export script taken from the environment (the running driver is not edited);
  `export_c2_keep.sh` exports without removing the checkpoints (`export_c2.sh` keeps only every 100th epoch);
  `vm_helpers/start_tail.sh` resumes from the second cycle's end (schedule checked on the VM CPU with
  `test_tail_sched.py`: 0.0040% of the peak at every tail step); `vm_helpers/avg_ckpt.py` averages the floating-point
  model tensors (the optimizer state, the lambda buffers and the routing table come from the last checkpoint; the
  table must be identical in all), tested on two F checkpoints and serialized; `vm_helpers/after_tail.sh` starts the
  tail once the second cycle has exited, averages, exports, and runs t91 and the permuted copy. At this LR the
  weights move by about 1e-6 per epoch, well below the quantization step, so the average is expected to serialize
  almost exactly as the tail's last epoch.

- **Average of the cooling's last five epochs, as Consilium (27/10 evening).** Consilium's final network was the
  weight average of epochs 55-59 of F4, the last five of its own one-cycle (LR from about 1.7% of F4's peak to
  0.1%), with no extra epochs (`04_consilium/avg_perm_F4.sh`). The same here is step-epochs 80-84 of the second cycle
  (LR from 1.0% to 0.004% of P's peak), whose checkpoints the run does not keep: `vm_helpers/snap_c2_last5.sh` copies
  `last.ckpt` at the end of each of them (global steps 308,934 to 324,190), and `vm_helpers/avg_c2_last5.sh` averages
  them with `avg_ckpt.py`, exports `F2_avg5_step324190.nnue`, runs the paired t91 against the second cycle's end and
  P's epoch 496, and makes the permuted copy. Armed at 18:57 UTC, before the end of epoch 80.
- **t91 of the second cycle at P's LR** (export label 69, step-epoch 71, LR 3.83% of the peak; reference P's epoch
  496 at 3.73%): +0.36% at lambda 1.0, +0.15% at 0.75, -0.15% at 0.0, the same as the final F of the first cycle
  (+0.33%, +0.14%, -0.14%) and well below the first cycle's epoch 50 at 4.3% (+0.63% at 0.75). Elo of the final F
  against P's epoch 496 (author's match, 27/10): +0.99 +- 4.41 over 7,358 games at 30k nodes, level.

- **Second cycle stopped and discarded (author, 27/10 20:42 UTC).** Elo at 30k nodes against P's epoch 496: epoch 75
  (LR 2.8%) +0.36 +- 4.36 over 7,654 games, epoch 78 (LR 1.6%) -3.46 +- 7.86 over 2,408 (stopped early); the
  training loss followed the first cycle's curve at the same LR. The run was stopped by PID after epoch 79 with its
  watchers; no network of the second cycle is kept as a candidate, and its checkpoints and networks were deleted on
  the author's order (only P and F are kept); its metrics stay in `run_logs/metrics/F_version_4.csv`. The best network so far is the final F of the first
  cycle (+0.99 +- 4.41 against P's epoch 496).
- **Restart from F's end (author's schedule, 27/10 20:47 UTC).** From F's final checkpoint: one epoch of linear rise
  to 0.5% of P's peak (the LR of F's epoch 57, when the loss was still falling), then 10 epochs of half-cosine back to
  0.004% (`vm_helpers/start_fr.sh`, checked with `test_fr_sched.py`), in a run folder of its own so that the trainer
  resumes from F's end, one checkpoint per epoch kept. `vm_helpers/after_fr.sh` averages the last 3 and the last 5
  epochs, runs t91 against F's end and P's epoch 496, and makes the permuted copies. End around 23:40 UTC.

- **The restart at 0.5% discarded and deleted (author, 27/10 22:30 UTC).** Its training loss stayed around 0.00309
  (epochs 60-65, LR 0.5% to 0.29%), and the paired t91 of its epochs 60-64 against F's end gave +0.67%, +0.01%,
  +0.16%, +0.01%, +0.24% at lambda 0.75 (snapshot noise around F's end, never below it). It was stopped after
  epoch 65 and its checkpoints, networks, t91 and charts deleted on the author's order. A restart at 0.14% of P's peak
  (`start_fr2.sh`) was stopped before its first epoch: the author wanted no rise above F's final LR.
- **Continuation of F without rise (author, 27/10 22:39 UTC).** From F's final checkpoint, 3 epochs of half-cosine
  from F's final LR (0.004% of P's peak) to zero (`vm_helpers/start_fr3.sh`, checked with `test_fr3_sched.py`: 0.004%,
  0.003%, 0.001%, 0 at the epoch ends), one checkpoint per epoch kept; `vm_helpers/after_fr3.sh` averages the three
  epochs, runs t91 against F's end and P's epoch 496 and makes the permuted copies. At this LR the weights move far
  less than the quantization step, so the networks are expected to match F's end closely.
- **End of the continuation (27/10 23:30 UTC).** Training loss 0.0030843, 0.0030805, 0.0030810 for epochs 60-62
  (F's last epoch 0.0030822: equal within noise). On the author's request only the last epoch was exported and
  measured (`vm_helpers/after_fr3b.sh`, replacing `after_fr3.sh`): paired t91 against F's end +0.10% at lambda 0.75
  (+0.0051 +- 0.0003 x1e-3; +0.10% at lambda 1, +0.05% at lambda 0), so slightly worse than F's end.
  `vm_helpers/avg_fr3.sh` then averaged the last 2 and the last 3 epochs (`avg_ckpt.py`); the averages differ from
  the last epoch by at most 1e-6 per weight.
- **Five more epochs started and stopped (author, 27/10 23:27-23:47 UTC).** From the continuation's end, 2 epochs
  flat at 0.0003% of P's peak and 3 of half-cosine to 0.00001% (`vm_helpers/start_fr4.sh`, `chain_fr4.sh`, checked
  with `test_fr4_sched.py`). Stopped by the author during its first epoch: at an LR 13 times below the continuation's
  the network cannot move away from the continuation's end.
- **Elo of the average of the last 3 epochs against P's epoch 496 (author, 27/10 23:55 UTC).** SPRT at 30k nodes on
  the VM's CPU (`vm_helpers/launch_sprt_avg3.sh` with the Linux twin of the Windows test script, same book and
  adjudication; benches 245702 and 135026). The container's CPU quota is 61 CPUs, so 53 games run at once.
  Stopped by the author at 10,974 games (28/10 00:20 UTC): **+0.57 +- 3.65** (pentanomial [110, 1302, 2626, 1358, 91],
  LLR -0.13). Together with the final F's test (+0.99 +- 4.41 / 7,358), weighted by their variances: **+0.7 +- 2.8**
  over 18,332 games. The final network is level with P's epoch 496 or slightly above; the author takes the 3-epoch
  average as the final network.
- **t91 of the averages (27/10 23:55 UTC).** Against F's end at lambda 0.75: average of 2 +0.09%, average of 3
  +0.21%, last epoch +0.10%. The three differ by at most 1e-6 per weight, so t91 gaps of 0.1-0.2% between networks
  this close come from the rounding of the export, not from what they learnt; the same holds for F against P's epoch
  496 (+0.14%).

## Next

- Permuted copy of the final network on 2 million positions; archives of the checkpoints and networks.
- (done) SPRT of the continuation's 3-epoch average against P's epoch 496 (30k nodes, VM).
- (superseded) End of the continuation around 27/10 23:30 UTC, then its average and t91.
- (superseded) End of the restart around 27/10 23:40 UTC, then its averages and t91 (about 00:15 UTC); Elo at 30k nodes against
  P's epoch 496 and F's end on the author's order.
- (superseded) End of the second cycle around 27/10 21:50 UTC: `after_cycle2.sh` exports its final network, runs t91 against the
  final F and P's epoch 496 and the permuted copy; the local watcher copies them off the VM. Then the tail (about
  23:15 UTC) and its average, t91 and permuted copy.
- Elo at 80k nodes, final F against P's epoch 496 (author's test, started 27/10 16:06 UTC).
- Elo of the final F against P's epoch 496 and against Consilium (author's tests). t91 is not used as training data
  until its label quality is checked.
