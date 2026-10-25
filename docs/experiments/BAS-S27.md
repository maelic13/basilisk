# BAS-S27

**BAS-S27 — three direction reads of the head against Basilisk 1.10.2, before B.3 builds on it**

- **Registration.** Registered 2026-10-24, before any game; maintainer-run;
  programme checkpoint, Elo layer, direction reads (PLAN A.9.2). They gate
  nothing: they look for a regression at the conditions no gate has read.
- **Engines** (both Release PEXT final PGO from `tools/build_test.ps1`,
  clang 23.1.3, clean manifests):
  - **The head:** `tools/test_engines/basilisk-head-4fa68b4-pext-pgo.exe`,
    revision `4fa68b4` (master: the selectivity core, fitted, and the
    piece-square repair); `bench 13` 8,437,148; SHA-256
    `FCC39B1E1F66F943046D44EDA7962B8291C156ADC5BD22A7FA3B5E5E3B992669`.
  - **Basilisk 1.10.2**, rebuilt from its tag (`fac5064`) with the same
    compiler, so both sides share one toolchain:
    `tools/test_engines/basilisk-v1102-clang23-pext-pgo.exe`; `bench 13`
    14,978,465, equal to the shipped 1.10.2; SHA-256
    `2282DC135A80192EE25EC2565C10061269989103DFDF5DFF48FFD0D3DA5C3E0E`.
- **Why now.** Every gate since 1.10.2 read `3+0.03` on one thread only:
  - B.2.4's gates: +136.2 ± 33.6 and +58.7 ± 20.9;
  - B.10: +18.3 ± 12.9;
  - BAS-S25: +188.3 ± 17.5 for the block-1 state against 1.10.2.

  Nothing has read the head at a longer control, at four threads, or with
  tablebases. B.2's new kernel has its own tablebase probe code, while the
  tablebase repair (BAS-S19, +31.7) was gated on the legacy kernel.
- **Question.** Does the head keep a clear lead over 1.10.2 at `10+0.1`, at
  four threads, and with tablebases configured?
- **Competing explanations for a shortfall in one read:**
  - LTC compression of fitted gains (Basilisk's 1.8.0 gains about halved
    at LTC; Rarog's search release held at LTC);
  - lost thread scaling in the new kernel (Rarog's 4T/1T ratio fell after
    its speed work, RAR-M74);
  - a defect in the new kernel's tablebase probes or band handling.

#### PRE-REGISTERED PREDICTION — freeze before exposure

- **The STC chain** puts the head about +208 over 1.10.2 at `3+0.03` 1T:
  - BAS-S25's +188;
  - block 2's +2 (+58.7 against +57.0);
  - B.10's +18.
- **`ltc`, 10+0.1 1T:** **+170 Elo**, 80% interval [+120, +215].
- **`4t`, 3+0.03 4T:** **+190 Elo**, 80% interval [+120, +250].
- **`tb`, 10+0.1 1T with Syzygy 3–6 men:** **+160 Elo**, 80% interval
  [+100, +210].
- **Confidence:** moderate for `ltc` and `tb`; low for `4t`, since the new
  kernel's thread behaviour has never been measured.
- **Reading rule:**
  - A read passes when its 95% interval excludes a loss (lower bound
    above 0).
  - A read below its 80% interval, or any Basilisk fault, time loss or
    illegal move, opens a diagnosis before B.3 starts:
    - `4t`: a thread-scaling read (1T against 4T bench and in-game NPS);
    - `tb`: the tablebase probe paths;
    - `ltc`: the prediction's compression assumption.
  - The reads accept nothing and change no default.

#### CONDITIONS

- Colosseum gauntlet through `tools/run_head_reads.ps1 -Read
  ltc|4t|tb`, 500 paired cycles (1,000 games) each.
- Engine 2 is fixed at 1500, so engine 1's rating minus 1500 is the head's
  Elo over 1.10.2.
- UHO book in random order, no adjudication, zero engine faults allowed.

| Read | Clock | Threads | Hash | Tablebases | Seed | Run file | Concurrency | Results |
|---|---|---|---|---|---|---|---|---|
| `ltc` | 10+0.1 | 1 | 64 | none | 27001 | `gauntlet-ltc.toml` (new) | 15 | `tools/results/head-vs-1102-ltc/` |
| `4t` | 3+0.03 | 4 | 512 | none | 27002 | `gauntlet.toml` | 3 | `tools/results/head-vs-1102-4t/` |
| `tb` | 10+0.1 | 1 | 64 | `SyzygyPath` `D:/chess/tablebases/syzygy3456` on both | 27003 | `gauntlet-ltc.toml` | 15 | `tools/results/head-vs-1102-tb/` |

- **Dry runs accepted 2026-10-24**
  (`tools/results/colosseum_gauntlet_head-vs-1102-{ltc,4t,tb}_20261024_2003*.*`;
  manifests `cc057116…`, `ac5d22fd…`, `fd4e50df…`).
- **Estimated time:** `ltc` about 40 min, `4t` about 50 min, `tb` about
  40 min; about 2.5 hours in all.
- **Runner, per read** (amended 2026-10-24, after `ltc` and before any `4t`
  or `tb` game):
  - `ltc` ran complete on `colosseum-cli` 0.2.0 (SHA-256 `5A4ECEDB…`), with
    fresh engine processes for every game, as 0.2.0's tournament did;
    manifest `a98ae0b6…`.
  - `4t` and `tb` run on 0.4.0 (SHA-256 `F3201288…`, PLAN A.9.1), each
    slot keeping its engine processes between games.
  - The first `4t` launch on 0.2.0 stopped before its first game; its
    empty directory is kept as
    `tools/results/head-vs-1102-4t-interrupted-cli020/`.
  - Each read is its own fixed gauntlet with its own verdict, so no read
    changes runner mid-way. Under 0.4.0 the resolved configuration of
    every read differs from 0.2.0's only in `engine_processes`: schedule,
    openings, seed, clocks, options and placement are unchanged.
  - The wrapper's new opponent-fault rule does not apply here: both
    engines are Basilisk builds, so any fault or time loss by either
    still voids a read, and the runner's fault cap stays 0.
  - Dry runs on 0.4.0 accepted at `38c6e6a`, clean tree
    (`tools/results/colosseum_gauntlet_head-vs-1102-{ltc,4t,tb}_20261024_2124*.*`;
    manifests `fd558a74…`, `5df62be5…`, `92e65ff4…`).

#### RESULT — append after exposure

- **Results (maintainer-run 2026-10-24).** The head's Elo over 1.10.2 is
  the runner's trinomial with its 95% interval; the pentanomial interval
  is over 500 colour pairs (`colosseum-cli stats` 0.4.0, which also pairs
  the 0.2.0 `ltc` journal). An independent recount from W/D/L agrees to
  0.1 Elo.

| Read | Elo [95%] | Pentanomial [95%] | W/D/L | Score | Pentanomial counts |
|---|---|---|---|---|---|
| `ltc` | **+241.4** [+221.9, +262.3] | [+223.6, +260.3] | 660/281/59 | 80.1% | 0, 11, 74, 218, 197 |
| `4t` | **+237.6** [+217.4, +259.2] | [+218.7, +257.7] | 670/254/76 | 79.7% | 1, 14, 85, 190, 210 |
| `tb` | **+221.9** [+202.4, +242.7] | [+204.0, +240.9] | 643/278/79 | 78.2% | 0, 14, 100, 194, 192 |

- **Faults and terminations.** No engine, time-loss, illegal-move or
  infrastructure fault in any read, and no adjudication.
  - Every decisive game ended in checkmate: 719, 746 and 722.
  - Draws by threefold, insufficient material, the 50-move rule and
    stalemate: `ltc` 194/60/25/2, `4t` 183/50/17/4, `tb` 169/82/27/0.
  - Each wrapper run exited 0 (completed), and the independent PGN census
    agrees in each.
- **Tablebases were live in `tb`.** Both engines got `SyzygyPath`.
  - About 7,300 move scores fall in the tablebase band
    (`cp 20000 - plies`), in 596 of the 1,000 games.
  - `ltc` has 111 such scores, in 64 games, all of them decisive. They
    start a median 14 plies before the end (4 to 82).
- **Runs.**
  - `ltc` ran on 0.2.0 at `e71aee7`: 34.9 min, manifest `a98ae0b6…`,
    run record `EB82A620…`, PGN `4ABE7398…`.
  - `4t` ran on 0.4.0. It was stopped once at `80865c0` and resumed from
    its journal at `65e34ad`; the runner and wrapper are identical at both
    revisions. 51.6 min, manifest `c4cc5a91…`, run record `3FC7ADA6…`,
    PGN `6E991E84…`.
  - `tb` ran on 0.4.0 at `65e34ad`: 35.1 min, manifest `950b570c…`, run
    record `8729DE7A…`, PGN `7816385D…`.
- **Reading rule.**
  - All three reads pass: every 95% lower bound is above +200.
  - None is below its 80% band, and there is no Basilisk fault, time loss
    or illegal move.
  - No diagnosis is opened before B.3.
- **Calibration against the frozen predictions.** All three are high:
  - `ltc` +241 against +170 [+120, +215]: 26 above the band.
  - `4t` +238 against +190 [+120, +250]: inside, 48 above the centre.
  - `tb` +222 against +160 [+100, +210]: 12 above the band.
- **What it shows.**
  - The prediction's LTC compression did not happen. `ltc` sits about 33
    above the STC chain's +208, so the fitted search gains held, and
    perhaps grew, at `10+0.1`.
  - Thread scaling is intact: `4t` at `3+0.03` reads the same as `ltc`
    within noise.
  - Tablebases narrow the gap by about 20 (`tb` against `ltc`). That is
    the predicted direction, but within the reads' combined noise, so it
    is no sign of a defect in the new kernel's probes.
  - It is self-play against the engine's own release, so a pool reading
    will compress it. B.11's release reads should predict without LTC
    compression.
- **Side finding (not owned here).** Without tablebases, both engines
  sometimes report a mate-range score as `cp`, for example `cp -31742`
  (more than 128 plies to mate). They also report `cp` values in the
  tablebase band in won endings. 1.10.2 does it too, so
  it predates B.2. It is display only, and it is reported to the
  maintainer for an owner.
- Disposition: COMPLETE. All three pass: `ltc` +241.4, `4t` +237.6, `tb`
  +221.9; zero faults.
