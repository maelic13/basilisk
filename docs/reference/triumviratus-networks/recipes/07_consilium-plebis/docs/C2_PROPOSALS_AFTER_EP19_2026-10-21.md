# Consilium Plebis after epoch 19: measurements and proposals (21/10/2026)

For the chat that works on the Consilium Plebis engine side. Same merged 8.1 search on both sides (TV1 time bake, budget
ramp), release PGO builds with the networks embedded: `c2ep19` (bench 329525, `c2_P_ep19.nnue`) and `t80_tm105r`
(bench 209655, Consilium). Binaries: `<work>\match_c2ep19`.

## Measurements

| test | result | notes |
|---|---|---|
| match at fixed 100k nodes per move | **-96.7 +- 15.8** over 586 games | socket 0 -101.2, socket 1 -90.3; network and search only |
| match at 20+0.2 | **-118.5 +- 18.0** over 332 games | socket 0 -108.9, socket 1 -128.4 |
| xperf, one engine, Xeon idle (cycles per node) | middlegame **+0.69%**, endgames **+2.78%** | instructions +6.6% / +8.5%, IPC 1.52 vs 1.44 |
| NPS under load, 40 instances (one per core) | middlegame **-1.0%**, endgames **-2.3%** | socket 0 -0.6 / -2.0, socket 1 -1.4 / -2.5 (two agents were compiling with -j4 during the run) |

Points file: `docs/rete/consilium2_elo_points.csv`; chart: `docs/rete/consilium2_elo_trajectory.py`.
Scripts: `script\misure\xperf\misura_c2.ps1` (xperf), `script\misure\xperf\carico40_c2.ps1` (under load), results in
`<work>\xp\prof_c2ep19_{mg,eg}\analisi.txt` and `<work>\prof\carico40_c2\risultati.txt`.

## Reading

- Speed is **not** the problem: the network costs 1-3% of nodes per second, idle and under load alike, with no socket
  effect. The hypothesis of a memory-bandwidth penalty under load (larger net, 8 experts) is refuted.
- The gap between the time control and the fixed-node match (-22 +- 24) may be noise. If it is real, the candidates
  are, in order:
  1. **Evaluation scale.** The search margins (RFP, futility, razoring, SEE thresholds in eval units), the
     correction histories and the time verdict (trend factor, TmJumpGuard in printed cp via `units_to_cp`, /400)
     were tuned on Consilium's scale. A network with a different scale shifts all of them at once. Consilium needed
     a per-phase eval-scale calibration at epoch 157 for the same reason.
  2. **Time management.** Fixed nodes bypass it; at 20+0.2 the verdict reads the score (trend, jump guard).
  3. **Depth.** 100k nodes is shallower than a 20 s game; the relative strength of a young network can change with
     depth.

## Proposals, in order

1. **Measure the scale of C2 against Consilium** on the same positions: `evalbatch` of both builds on a fixed set
   (e.g. the gate positions plus middlegame and endgame sets), regression of C2 eval on Consilium eval per phase
   (piece count) and per C2 expert. Report slope and offset.
2. **If the slope differs from 1**, add a per-phase (or per-expert) output scale for C2 in the bridge, default
   identity (bench unchanged), set to the measured slope, then a fixed-node and a 20+0.2 match at the same epoch.
   This is the same calibration that Consilium got at epoch 157 (+ calibrated points in its chart).
3. **Repeat the pair of matches** (fixed 100k nodes and 20+0.2) at each new export (every 10 epochs) against the same
   opponent, so the C2 chart has comparable points; append rows to the CSV.
4. **Speed, only after the above** (worth 1-3% NPS at most, about 1-3 Elo), all tree-identical:
   - pawn-king cache: 8192 entries x 192 bytes = 1.5 MB per thread (40 threads = 60 MB, above the L3 of a socket);
     try 1024-2048 entries, or a compact entry (outputs in int16 if their range allows exactness);
   - refresh cache `kcache[2][64][NEXPERTS]`: with 8 experts about 2.2 MB per thread; keep entries only for the
     experts actually seen (e.g. the last two per perspective and king square);
   - check the endgame path (+2.8% idle): the endgame experts and the pawn-king subnet are hit more often there.
   Measure each with `carico40_c2.ps1` and the xperf kit (`run_misura.ps1`), bench must stay 329525.
