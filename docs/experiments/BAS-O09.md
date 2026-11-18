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
