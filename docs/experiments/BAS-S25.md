# BAS-S25

**BAS-S25 — where the programme stands: the current state against Basilisk 1.10.2**

- **Registration.** Registered 2026-10-14, before any game; maintainer-run;
  programme checkpoint, Elo layer. A direct reading the maintainer asked
  for, not a gate: B.2.3's blocks and B.2.4's gates are unchanged.
- **Engines.**
  - **Engine 1, the current state:** the ON Tune build at `b5983c6`
    (engine `3eea3c5`: Cluster 1 with `CoreLmpExemptChecks` 0 and
    Amendment 5; bench 6,096,792; SHA-256
    `A900EFD2AC77CD3F4178A1161A4FA5091A313DA5F4CCCE4EA799043C99CA2AEB`).
    It plays SPSA block 1's 49 rounded centres
    (`tools/results/b23-spsa-block1/tuned-options.txt`, SHA-256
    `5DB77308154B088BE3E7D641F38776DEAA7390DDFB30D5E666A58C1E231282A1`).
  - **Engine 2, Basilisk 1.10.2:**
    `tools/test_engines/basilisk-1.10.2-rc-pext-pgo.exe`, bench 14,978,465,
    SHA-256
    `1234F3F68A6C9C55578DA3CE18086B2FB499C97986A24022CCD8C745626972F1`.
    It is built from `2ec445bb`, which differs from the `v1.10.2` tag
    (`fac50644`) only by a CMake line for the `TEXEL` target, so its engine
    is 1.10.2's.
- **Question.** How many Elo does the current state carry over the released
  1.10.2, measured directly rather than chained?
- **The chain it checks:**
  - B.2.2's paired run, +130.9 ± 11.4;
  - `CoreLmpExemptChecks` 0, +13.6 ± 9.7;
  - Amendment 5, not measured;
  - SPSA block 1, +57.0 ± 9.8 (BAS-S24).
  - The measured steps sum to about +201 (±18 from their intervals
    alone).
- **Competing explanations for a direct reading below the chain:**
  - Amendment 5 cost Elo; it cost 12 WAC positions at 100k nodes.
  - Self-play gains do not add exactly across a chain of neighbours.
  - The Tune flavour's speed against the Release binary.

#### PRE-REGISTERED PREDICTION — freeze before exposure

- **Expected:** **+175 Elo**, 80% interval [+140, +210].
  - Below the chain's +201, for the unmeasured Amendment 5 and for
    non-additivity.
- **Confidence:** moderate.
- **Falsifiers:**
  - below +120: the chain overstates by more than its links' intervals.
    Amendment 5 then becomes the first suspect for B.2.4(a);
  - above +230: the links understate, which no one predicted.
- **Conditions:**
  - Colosseum gauntlet, `tools/run_b23_vs_1102.ps1`;
  - 1,000 paired cycles (2,000 games), seed 25001, engine 2 fixed at
    1500;
  - `3+0.03`, 1T, Hash 64, UHO, no adjudication, tablebases off;
  - concurrency 15 as the wrapper resolves it;
  - results in `tools/results/b23-vs-1102/`.
- **Preparation:** dry run accepted 2026-10-14
  (`tools/results/colosseum_gauntlet_b23-vs-1102_20261014_122243.*`).
  - Engine 1 carries Hash, Threads and the 49 block-1 values; engine 2
    carries Hash and Threads only.
  - Engine 2 is the fixed participant.

#### RESULT — append after exposure

- **Result (maintainer-run 2026-10-14):** **the current state +188.3 ± 17.5
  Elo over 1.10.2** (rating 1688.3 against the fixed 1500).
  - 1,496 of 2,000 points (74.8%); W/D/L 1185/622/193;
  - 0 engine faults, natural termination, the independent PGN census
    agrees;
  - `colosseum_gauntlet_b23-vs-1102_20261014_122508.*`, run record
    `0E95EFC7…506F`, PGN `68D339BF…2A15`.
- **Calibration:** inside the 80% interval [+140, +210], 13 above the
  centre of +175; neither falsifier fires.
- **Against the chain:** the direct reading sits 13 below the measured
  links' sum of about +201.
  - That is well inside the chain's own ±18 and this run's ±17.5, so it
    does not show that Amendment 5 cost Elo, nor that the links are
    non-additive. Amendment 5's own effect stays B.2.4(a)'s to measure.
- **What it shows:** with SPSA block 1 of up to three done, the search
  programme stands about +188 Elo above the released 1.10.2 at `3+0.03`.
  It is self-play against the engine's own release, so a pool reading
  will compress it.
- Disposition: run, +188.3 ± 17.5.
