# BAS-M31

**BAS-M31 — C.5.1.1: the frontier pair on the head, at equal time and at time odds**

- Date / owner / category: registered 2026-11-27, before any game;
  maintainer-run, agent-read; measurement.
- Arms:
  - Head: `tools/test_engines/basilisk-c511-head-pext-pgo.exe`, SHA-256
    31E63F9A59A2FA2ED3E9235E4123B7609C6198CBE6CE5E0320EF995E240EE989,
    a clean final PGO PEXT build of `b5b8817`, `bench 13` 3,525,148.
  - Stockfish 16 HCE:
    `D:\chess\engines\stockfish-16-x64-bmi2-classical-pgo.exe`, SHA-256
    041396A4B14EC25BAE0B57F90E917A17E7FCEB1FBBAB80859EE2EC3774D25442
    (BAS-M18's), `Use NNUE=false`, otherwise at its defaults.
- Research question: how far is the head from the pre-NNUE target after
  the refit BAS-E66 kept, and how much of that distance is nodes a
  second?
- Registration, hypotheses, rules and limits:
  `analysis/c511_residual_2026-11-27.md` §3 to §6 and §9.
- Two reads, 3,000 games each, one seed (`511002`) and so the same
  openings in the same order:
  - `frontier`, equal time: both at `3+0.03`. BAS-M24's protocol, the
    pair owed after an accepted gate.
  - `odds`: the head at `3+0.03` and Stockfish at `2.161+0.022`. The
    head's clock is 1.388 times Stockfish's, the ratio of nodes a second
    in BAS-M27 (2.79M against 2.01M). The wrapper gives only the second
    arm a clock of its own, so the odds shorten Stockfish's.
- Registered design and stop rule: no gate; one fixed match per read,
  never extended or replaced.
- Full conditions: `python tools/run_c511_reads.py --read frontier` and
  `--read odds`; Colosseum CLI, a match with an external second arm, UHO
  `UHO_Lichess_4852_v1.epd` in random order, margin 20 ms, 1T, Hash 64,
  concurrency 14, no adjudication, no tablebases, the wrapper's 1% fault
  allowance for the foreign engine.
- Limits: `odds` prices 39% more nodes a move for Basilisk against this
  opponent. Whether the two engines count the same thing, and how much
  of the gap a speed pass can recover, is C.5.3's question.
- Dry runs accepted 2026-11-27; an eight-game smoke of `odds` had no
  fault and no time loss, the head 160,224 nodes a move and Stockfish
  161,090.

#### PRE-REGISTERED PREDICTION — frozen 2026-11-27

Elo, the head less Stockfish 16 HCE, with an 80% band:

- `frontier`: −30, −44 to −16.
- `odds`: +10, −12 to +30.
- `odds` less `frontier`: +40, +22 to +60.
- `odds`' arms agree in nodes a move within 7%: 0.8.
- The gain is at least two thirds of the deficit, which reads "speed
  leads" under the packet's rule 1: 0.55.
- Basis: BAS-M27 read −39.8 before the refit, and BAS-E66's
  first-crossing +14.3 is taken as about +10. 1.388 times the nodes is
  0.47 of a doubling: 28 Elo by the textbook rule and 69 by PROCESS's,
  and less across engines than in self-play.
- Falsifiers:
  - `frontier` worse than −48.0, BAS-M27's lower bound: BAS-E66's gain
    would be worth nothing against the target.
  - `odds` below `frontier`: more relative time cannot cost, so the
    clocks or the pairing are then wrong and neither read is used.
- Flag: `odds` with the arms' nodes a move more than 7% apart decides
  nothing alone; the gain is then read per percent actually equalised.

#### RESULT — append after exposure

#### PREDICTION CALIBRATION — preserve the original prediction above
