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
