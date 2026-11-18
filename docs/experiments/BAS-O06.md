# BAS-O06

**BAS-O06 — B.11.1 search deficit meter G(0) on the head, equal time (BAS-O05's protocol)**

- Date / owner / calibration category: registered 2026-11-17, before any game;
  maintainer-run, agent-read; measurement.
- Baseline / candidate: Basilisk's head at `4c0bc0e`, whose `src/` tree
  (`348c90f`) is master's `4d2742f`, against the classical-Stockfish search
  oracle rebuilt around that same evaluation.
- Binaries:
  - Head: `tools/test_engines/basilisk-b111-head-pext-pgo.exe`, Release PEXT
    final PGO, clean, bench **4,777,412**, SHA-256 `31BC229A…`, clang 23.1.5.
  - Oracle: `tools/test_engines/oracle-b111-4d2742f.exe`, built by
    `tools/oracle/build_oracle.ps1 -BasiliskRevision 4d2742f` from tag
    `oracle/hybrid` (`01df815`, Stockfish `9587eeeb`), `src/` byte-identical to
    `4d2742f`, bridge patch `224E6E20…`, clang 23.1.5; SHA-256 `8ADEE9AE…`,
    oracle bench `16 1 10 default depth` = **875,469** nodes (967,078 on
    1.10.1: the B-phase verdict repairs changed the evaluation). Conformance
    471,519 positions, **0 mismatches**. The build first failed its identity
    check: `src/` files the head deleted lingered in the worktree; the script
    now clears `src/` before the checkout.
- Research question: how much of BAS-O05's +312.6 ± 17.8 search deficit did
  the B programme recover, on the same instrument?
- Interacting mechanisms: the oracle carries the head's evaluation, so the
  B-phase evaluation repairs (B.9's verdicts, B.12) move both arms; the oracle
  is not PGO-built (BAS-O05 informational: 2.83M against 3.99M NPS), so it
  plays handicapped on speed, as at A.7.3; time management is each engine's own.
- Competing hypotheses: the cluster gates' self-play gains overstate the gain
  against a different search; the fixed-node depth gap (BAS-D22: 30.41
  against 33.75 plies at 300k nodes, 3.3 plies where it was 12.2) says the
  oracle still searches deeper per node.

#### PRE-REGISTERED PREDICTION — freeze before exposure

- Expected G(0) (oracle over head): **−40**, 80% band **−110 to +30**: the
  head ahead.
- Basis: BAS-M15 put the head after Clusters 1 to 3 at +314 in the pool
  over 1.10.2 and +148 against Critter 1.6a (1.10.0 was −198 there); the
  clusters since add about +55 to +65 in self-play (B.6 +15.6 and +28.2,
  B.7 +1.9, B.8 +19.1, B.9 −0.8), less in transfer. About +350 to +390 over
  1.10.x against a field, against 312.6, gives −40 to −80; the remaining
  fixed-node depth gap and Rarog's own read (its head +59.8 over the same
  search on its evaluation, RAR-O08) pull it toward zero.
- Probability the head scores above 50%: 0.7.
- Confidence: moderate on the sign, low on the magnitude.
- Most likely failure mode: the self-play gains transfer less to a strong
  foreign search than to the field.
- Falsifiers: G(0) above **+100** says the programme recovered under two
  thirds of what its gates and the pool claim, and the instrument is checked
  first (the oracle's option, its evaluation); below **−150** is inconsistent
  with the remaining 3.3-ply fixed-node gap and points at the instrument.
- Registered design and stop rule: no gate; one fixed run, never extended or
  re-run on the same binaries.
- Full conditions: `python tools/run_b111_reads.py --read g0-equal`;
  Colosseum CLI 0.5.0 (`81EA6CBA…`), two-engine gauntlet, 1,500 cycles of one
  opening twice with colours reversed, **3,000 games**, UHO
  `UHO_Lichess_4852_v1.epd` (`7A7F6470…`) in random order, seed `111101`,
  `3+0.03`, margin 20 ms, 1T, Hash 64 both, `Use Basilisk HCE=true`,
  concurrency 14 one core each, no adjudication, no tablebases, **zero
  permitted faults** (`-OpponentFaultRatePercent 0`, as BAS-O05); the oracle
  fixed at 1500, so **G(0) = 1500 minus the head's rating**. Dry run clean at
  `62e7643`.

#### RESULT — append after exposure

- Run 2026-11-17, 13:53–14:26, by the maintainer on an idle host, at
  `769ade8` (`tools/results/b111-g0-equal/`). The run is complete: all
  3,000 games scored, no faults, no time losses; the census agrees and the
  pentanomial recount agrees.
- The head **1082-1438-480**, 60.0%, Elo **+70.7**, 95% interval [+63.0,
  +78.4]; pentanomial [13, 189, 604, 571, 123], nElo +117.2 ± 12.4.
  **G(0) = −70.7**: the head beats the oracle's search on its own
  evaluation. Against BAS-O05's +312.6 on 1.10.1, the programme recovered
  **383 Elo** on this meter.
- Game meters at `3+0.03`:
  - the head: 2.10M nodes per second, 139,949 nodes a move, median depth 15;
  - the oracle: 2.58M nodes per second, 172,691 nodes a move, median
    depth 16.
- Calibration: inside the 80% band (−110 to +30). It is 31 Elo beyond the
  point estimate toward the head, and the head scored above 50% as given
  0.7. The falsifiers did not fire.
- Disposition: COMPLETE. This is the checkpoint's G(0) at equal time.
