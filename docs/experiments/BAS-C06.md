# BAS-C06

<!-- part 1 of 1: from docs/EXPERIMENTS.md, 7. Correctness and protocol lessons -->

| Field | Value |
|---|---|
| ID | BAS-C06 |
| Experiment or failure mode | Positive Syzygy tests depended on `D:\\chess\\Syzygy345`; when that private directory was absent they reported passing placeholder assertions without probing WDL, DTZ, rule-50 normalization, root metadata, TB hits, or PV expansion. |
| Disposition | **Test-instrument repair accepted; search unchanged.** A repository fixture now carries the canonical 272-byte KQvK WDL and 5,392-byte DTZ tables as base64 (SHA-256 `517667df...2d50` and `71ea9444...1c8c`). Tests materialize them in a unique temporary directory and hard-fail initialization or decoding errors; all nine positive Syzygy assertions ran in release and ASan/UBSan builds. **Coverage caveat added 2026-09-07:** KQvK holds no `WDL == +/-1` entries, so the cursed-win and blessed-loss decode path is covered by nothing in the repository. The section formerly named "rule50 cursed win" tests rule-50 CLAMPING of a clean WDL 2 win at halfmove clock 99, a different mechanism; it was renamed and the gap recorded in `tests/fixtures/syzygy/README.md`. This matters because `endgame_truth.py` scores a move that downgrades a clean win to a cursed win as DISCARDING the win, so a WDL +/-1 decoding defect would pass the whole suite while corrupting the endgame instrument's central judgement. Closing it needs a five-man table such as KNNvKP, far too large to embed; the gap is accepted and recorded rather than implied to be covered. |
| Conditional lesson / coverage | A positive integration test must own its smallest valid external-data fixture. Absence of a developer-local resource is a failure unless the test is explicitly classified optional. A test's NAME is part of its contract: one that implies coverage it does not have is a quieter version of the placeholder assertion it replaced. |
| Source | `tests/test_search.cpp`; `tests/fixtures/syzygy/` |
