# BAS-M20

**BAS-M20 — B.11.4's 4T pool read: the 1.11.0 candidate against the four targets at four threads, sibling-free**

- **Registration.** Created 2026-11-18, before any game; maintainer-run
  games. PLAN B.11.4: a 4T pool read beside the release SPRT, sibling-free.
  It sees what a head-to-head read cannot: a thread-scaling loss that both
  Basilisk versions share.
- **Candidate.** BAS-S67's (`basilisk-b111-head-pext-pgo.exe`, `31BC229A…`,
  bench 4,777,412).
- **Field.** BAS-M16's four targets, pinned by SHA-256, at their 1T Super
  Rating Tournament ratings, fixed:

  | Target | Rating | Thread option | Other options |
  |---|---|---|---|
  | Houdini 3 | 3287 | `Threads=4` | |
  | Critter 1.6a | 3192 | `Threads=4` | `OwnBook=false`, `Tablebase Usage=Disable` |
  | Fritz 16 | 3173 | `Max CPUs=4` | |
  | Rybka 4.1 | 3111 | `Max CPUs=4` | |

- **Conditions.**
  - A gauntlet of 200 cycles × 2 colour-reversed games: 400 games an
    opponent, 1,600 in all.
  - `3+0.03`, margin 20 ms, 4 threads a side, Hash 512.
  - Sibling-free: three games at a time on explicit cores, one logical CPU
    per physical core, 4 cores a game.
  - UHO in random order, seed 114004.
  - No adjudication, no tablebases.
  - The runner's fault cap is 16, 1% of the foreign games; any Basilisk
    fault is listed and opens a repair leaf.
  - Launcher: `python tools/run_b114_gates.py --run pool4t`; the dry run is
    accepted. About 75 minutes.
- **Reading.**
  - The candidate's 4T performance against the four fixed 1T ratings, by
    maximum likelihood, with a 95% bootstrap interval.
  - Each head-to-head's W-D-L and Elo, with its 95% interval.
  - A target's crash is listed, and the reading is reported with and
    without it, as in BAS-M11.
- **Comparison.**
  - BAS-M11 read 1.10.1 at a 4T performance of 3040 [3026, 3054], but
    unpinned, which BAS-X41 shows understates a four-thread engine.
  - The head's 1T performance against the same field is 3416.3 (BAS-M16).
  - This is not an acceptance gate on its own. It licenses the release
    beside BAS-S67 and BAS-M19, and stands as 1.11.0's 4T baseline.

#### PRE-REGISTERED PREDICTION — freeze before exposure

- 4T performance about **3480**, 80% band 3420 to 3540.
  - Basis: 1T 3416.3, plus a sibling-free four-thread gain over an
    opponent field that also plays at four threads.
  - BAS-M11's unpinned +46 over 1T is a floor.
- Every target gap stays closed at 4T: probability 0.85.
- No Basilisk fault: probability 0.95.
- Falsifier: a performance below 3416 means Basilisk scales worse than the
  targets from 1T to 4T. That goes to D's threads leaf with the per-game
  NPS, not to the release decision, unless a fault caused it.

#### RESULT — append after exposure

- **Withdrawn unplayed** (maintainer, 2026-11-18), before any game.
  - It could not block the release under its own falsifier, so it licensed
    nothing.
  - A regression at 4T is BAS-M19 (b)'s to catch.
  - A shared thread-scaling loss is D.2's self-relative read.
  - A 4T performance against the field is E.2's target gate.
  - PLAN §5 drops the 4T pool read from the classical release rule.
  - The launcher's `pool4t` mode was removed once BAS-M19's runs finished.
