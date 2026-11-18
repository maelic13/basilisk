# BAS-O07

**BAS-O07 — B.11.1 G(0) at the bridge's measured time odds (RAR-O08's conditions)**

- Date / owner / calibration category: registered 2026-11-17, before the
  price read and any game; maintainer-run, agent-read; measurement.
- Baseline / candidate: as BAS-O06, the head (`4c0bc0e`, bench 4,777,412,
  `31BC229A…`) against the same playing oracle (`8ADEE9AE…`).
- Research question: G(0) with the bridge's cost handed back to the oracle,
  the only read that compares with Rarog's RAR-O08 and later oracle reads
  (Rarog found its RAR-O03/O04 oracle at 0.56 of its own speed through a FEN
  round trip, Basilisk's at 0.83).
- Instrument:
  - *Price oracle.* `tools/test_engines/oracle-b111-4d2742f-price.exe`
    (`DF6FD7D3…`), the same build with `tools/oracle/bridge-repeat.patch`,
    which builds the board `BASILISK_HCE_BRIDGE_REPEAT` times per evaluation
    and changes nothing else: its bench is the playing oracle's 875,469 nodes
    and conformance 0 mismatches. Only the playing oracle plays.
  - *Price.* `tools/diag/bridge_price.py`, Rarog's method from
    `tools/oracle-hybrids/bridge_price.py` at `89b4a92`: `bench 16 1 16
    default depth` (Rarog's command for the classical oracle), three cycles
    at repeat 1 and 3, identical node counts required, medians; the share is
    half the repeat-3 excess over the repeat-1 time per node, the odds
    t1 / (t1 − bridge), the clock 3000 and 30 ms times the odds, rounded to
    the millisecond. Unit tests reproduce RAR-O08's classical medians (share
    0.0847, odds 1.0926).
  - *Wire live.* A one-cycle smoke at `bench 16 1 12` on the busy host
    (2026-11-17, not this read): repeat 3 took 932 ms against 888, equal
    nodes (2,495,841), share 2.5%; the tool refuses the unpatched oracle.
  - *Harness.* Colosseum's per-arm clock exists only in a two-arm mode, so
    this read is a fixed match with the oracle as `-ExternalB` (pinned by
    SHA-256, no Basilisk manifest), added to `tools/colosseum.ps1` at
    `177c502`; guard cases 52/52, the four new ones included.
- Interacting mechanisms and confounds: as BAS-O06. The odds equalise the
  bridge only; the oracle keeps its non-PGO speed otherwise. The match form
  draws its openings from its own seed, so the two G(0) reads are not
  paired; their difference carries both reads' noise.

#### PRE-REGISTERED PREDICTION — freeze before exposure

- Bridge share: **3.0%**, 80% band 1.5% to 6.0% (odds ×1.015 to ×1.064);
  basis: the smoke's 2.5% and the bridge's direct field writes, far cheaper
  than Rarog's 8.5%.
- G(0) at odds: **−35**, 80% band **−105 to +35**; the oracle gains about
  +5 from its extra time (two to four per cent of a doubling).
- Most likely failure mode: as BAS-O06.
- Falsifiers: a share above 10% says the bridge is not what this oracle pays
  (check the patch's placement); a G(0) more than 40 from BAS-O06's in either
  direction is beyond what a few per cent of time can buy and is checked as
  noise or instrument before it is read.
- Registered design and stop rule: the price is read once, on an idle host,
  and then the match plays once at the price's clock; no extension, no
  re-run. `run_b111_reads.py` refuses a missing price or one off this
  protocol (other engine, command, cycles under three, base clock).
- Full conditions:
  - Price: `python tools/diag/bridge_price.py --engine
    tools/test_engines/oracle-b111-4d2742f-price.exe --out
    tools/results/b111-checkpoint/bridge_price.json`, idle host.
  - Match: `python tools/run_b111_reads.py --read g0-odds`; Colosseum CLI
    0.5.0, fixed match, **3,000 games** in 1,500 colour-reversed pairs, UHO
    random order, seed `111102`, head `3+0.03`, oracle at the price's clock,
    margin 20 ms, 1T, Hash 64, `Use Basilisk HCE=true`, concurrency 14, no
    adjudication, no tablebases, zero faults (`match-fixed.toml`);
    **G(0) is minus the head's Elo**. Dry run clean at `62e7643` with
    Rarog's 3279+33 standing in for the unread price.

#### RESULT — append after exposure

- **Price.** Read 2026-11-17 (finished 13:49) by the maintainer on an idle
  host (`tools/results/b111-checkpoint/bridge_price.json`).
  - Three cycles of `bench 16 1 16 default depth`, 15,239,377 nodes at
    both repeats.
  - Median 5,263 ms at repeat 1 and 5,481 ms at repeat 3; every cycle is
    slower at 3, so the wire is live.
  - Bridge 7.15 ns a node, share **2.07%**, odds **×1.0211**, oracle clock
    **3063+31 ms**.
  - Inside the 80% band (1.5% to 6.0%), below the 3.0% point. This is a
    quarter of Rarog's 8.5%, as the direct field writes predicted.
- **Match.** Run 14:26–15:00, at `769ade8`
  (`tools/results/b111-g0-odds/`), 3,000 games, no faults, no time
  losses; the pentanomial recount agrees.
  - The head 1025-1408-567, 57.6%, Elo **+53.5 ± 7.6**; pentanomial
    [20, 215, 649, 519, 97], nElo +89.1 ± 12.4. **G(0) at odds = −53.5**.
  - Inside the 80% band (−105 to +35), 18.5 beyond the point toward the
    head.
  - The oracle's average move took 68.2 ms here and 67.0 ms in BAS-O06,
    1.8% more, as the odds give.
- **Against BAS-O06.** The odds cost the head 17.2 Elo, with a difference
  interval of about ±10.8.
  - About 2% more time should buy the oracle +3 to +5, so the excess is
    about 1.2 standard errors. The two runs draw different opening samples
    (seeds 111101 and 111102). It is read as noise, not as a price of the
    bridge.
  - The falsifier (more than 40 from BAS-O06) did not fire.
- Rarog's sign: the head over the oracle is +53.5 at RAR-O08's conditions.
  Rarog's O08 read +59.8 for its own head on its evaluation, so the two
  engines stand at about the same distance past the classical search.
- Disposition: COMPLETE. G(0) at odds is the figure that compares with
  Rarog's reads.
