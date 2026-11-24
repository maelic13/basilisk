# BAS-O09

**BAS-O09 — the evaluation donor: `9587eeeb`'s and `sf_16`'s classical evaluations, each in both searches**

- Date / owner / calibration category: registered 2026-11-18, before any
  game and before the ports were accepted; maintainer-run, agent-read;
  measurement.
- Binaries: `sf16-stock`, `sf16-eval9587`, `sf9587-stock` and
  `sf9587-eval16`, built by `tools/donor/` from Stockfish `sf_16`
  (`68e1e9b`) and `SF_classical` (`9587eeeb`) with committed patches; the
  hashes and the port acceptance are appended here before the first game.
- Research question: which classical evaluation is the donor for Phase C?
- Registration, port acceptance, design, decision rule and prediction
  (`a16` +15, `a95` −10; P(opposite signs) 0.50):
  `analysis/c0_evaluation_programme_2026-11-18.md` §4.3, frozen in the
  commit that adds this file.
- Registered design and stop rule: no gate; two fixed runs of 3,000 games,
  one a host, never extended or re-run on the same binaries.
- Full conditions: `python tools/run_c0_reads.py --read donor-sf16` and
  `--read donor-9587`; Colosseum CLI, two-engine gauntlets, 1,500 cycles
  each, UHO in random order, seeds `120002` and `120003`, `3+0.03`, margin
  20 ms, 1T, Hash 64, concurrency 14, no adjudication, no tablebases, zero
  permitted faults.

#### RESULT — append after exposure

- **Binaries and port acceptance, appended 2026-11-18 before any game.**
  Built by `tools/donor/build_donor.py` from the committed patches, clang
  23.1.5, no PGO, identical flags inside a host:
  - `donor-sf16-stock.exe` `C85B7F8280D56D5A23FD50BEE121F42408F0B94FAC00C2B387AD5EE8325D95E4`,
    bench 3,684,832 (`bench 16 1 13 default depth classical`);
  - `donor-sf16-eval9587.exe` `C870CD697EEC7FD877986C336B2ECE09FA35A1A9B66E7F7271DB62639F88851C`,
    bench 2,360,513;
  - `donor-sf9587-stock.exe` `2F2FE66BCDE3A205EE586E6F802554B3C5E496EC9A5C647C8206129823E635E3`,
    bench 4,746,616, the commit's own signature;
  - `donor-sf9587-eval16.exe` `AF84AAFB437D8F7CBE6A8956332C275BA65136329A79C492481258827B826C26`,
    bench 4,572,257.
  Each hybrid's raw evaluation equals its origin's on about 122,000
  positions with zero mismatches (`tools/donor/verify_donor.py`), and the
  verifier fails on known-bad input. One clause added to the contract
  before any game: `sf_16`'s lazy cut reads the search's root score, so
  `sf9587-eval16` receives it from `9587eeeb`'s root loop (one line of
  `search.cpp`). Details: the packet's §5.1.
- Dry runs accepted 2026-11-18 for both hosts.
- Run 2026-11-18 by the maintainer (`tools/results/c0-donor-sf16`, PGN
  `1ae0a005…`; `tools/results/c0-donor-9587`, PGN `202aff7d…`): 3,000
  games each, no faults, no time losses; the recounts agree.
- `sf_16`'s search: `sf16-eval9587` against `sf16-stock` 784-1464-752,
  +3.7 [−3.5, +10.9], pentanomial [33, 355, 696, 379, 37]. So `a16` =
  −3.7.
- `9587eeeb`'s search: `sf9587-eval16` against `sf9587-stock`
  826-1334-840, −1.6 [−9.4, +6.1], pentanomial [56, 374, 648, 372, 50].
  So `a95` = −1.6.
- `A` = −2.7 [−8.0, +2.6]; `H` = −1.0 [−6.3, +4.3].
- **By the frozen rule `sf_16` stays the donor**: `9587eeeb`'s evaluation
  is ahead in both hosts, but `A`'s interval includes zero. In play the
  two are equal within about 8 Elo in either search, and neither search
  favours its own evaluation.
- One host effect: `sf9587-eval16` drew seven elementary mates by the
  fifty-move rule (KR-K 4, KBN-K 2, KRB-K 1), about 0.8 Elo of `a95`.
  `sf_16` damps specialised endings by the halfmove clock, which takes
  the score under the value below which `9587eeeb`'s search cuts a node
  off on its static score. The port is as registered.
- Disposition: COMPLETE. `sf_16`'s classical evaluation is Phase C's
  donor and `9587eeeb`'s the second seed column.

#### PREDICTION CALIBRATION — preserve the original prediction above

- `a16` +15 (band −10 to +45): −3.7, inside the band, the sign missed.
  `a95` −10 (band −40 to +20): −1.6, held.
- Opposite signs were given 0.50 and did not occur; `9587eeeb` ahead in
  both was given 0.20 and is the nominal outcome; the rule moving the
  donor was given 0.08 and did not.
- What the model had wrong: a host binding of about 12 Elo. None is
  measurable. The two evaluations differ by 1.7% in scale, and a search's
  margins do not see that.
- Conditional lesson: "a host favours the evaluation it was tuned with"
  is a statement about scale (B.0), not about two tunings of one
  evaluation.
- Objective retry trigger: none. A term's form is chosen per family by
  the family screen, with both evaluations read.
