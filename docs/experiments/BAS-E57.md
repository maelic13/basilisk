# BAS-E57

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Cluster 5.8 closing summary -->

**BAS-E57 - 15.0.b harness reserve sweep: `Move Overhead` 40 against 10 on
the same binary - RUN 2026-09-10, REJECTED at -64.81 Elo** (registered 2026-09-09).

- Binary: `basilisk-15.0a-cand-pext-pgo.exe` (bench 14,978,465, SHA-256
  `1F7877B2...`) on both sides; arm A `option.Move Overhead=40`, arm B the
  default 10. `tools/sprt.ps1 -Mode fixed -Games 10000`, `3+0.03`, 1T, Hash
  64, paired UHO, concurrency 14, natural termination. Runs third in Rarog's
  `tools/results/night-20260909/run_night.ps1`.
- Research question: does a 30 ms wider reserve remove the residual time
  forfeits (4 in 2,927 on the 15.0.a gate; 3 in 24,989 and 1 in 21,994
  earlier), and what does it cost?
- Mechanism: Basilisk already counts `go` dispatch latency (Step 5.4) and polls
  the clock every 2,048 nodes; Rarog's reconstruction of seven forfeits shows
  50-500 ms host stalls invisible to any engine mid-search. Only the reserve
  can absorb part of that.

PRE-REGISTERED PREDICTION, frozen before exposure:

- Arm B (10 ms): 4 to 12 forfeits in 10,000; arm A (40 ms): 0 to 4.
- Paired Elo of arm A: -2 to +1; probability the wider overhead at least
  halves the rate: 0.6. Confidence low at these rates.
- Registered rule: adopt 40 in the harness profiles only if arm A forfeits at
  most a quarter of arm B AND the paired interval excludes -3; otherwise keep
  10 and record the rate as the harness floor. Symmetric in every gate, so no
  verdict changes either way. No engine source changes.

RESULT (2026-09-10): `sprt_e56overhead40_vs_e56overhead10_20260910_021630`,
10,000 games, **0 forfeits in either arm**, arm A (40 ms) **-64.81 +/- 4.45
Elo**, nElo -101.50, Ptnml [450, 1811, 1965, 681, 93].

PREDICTION CALIBRATION: sign wrong by an order of magnitude, and the miss is
in mechanism. `Move Overhead` is not a low-clock reserve: the SF-shaped
budget subtracts `overhead * (2 + movestogo)` from the planning clock, so 40 ms
over a 50-move horizon discards about 2.1 s of a 3 s clock. Rarog's RAR-R12
measured -80.85 for the same change. The forfeit rate was undecidable: the
idle night host produced no forfeit in either arm, so the daytime rate is
host interference. Disposition: `Move Overhead` stays 10 in every profile; no
engine change; 15.0.b closes.
