# Syzygy test fixture

These files are base64 encodings of the canonical three-piece `KQvK` Syzygy
tables from `http://tablebase.sesse.net/syzygy/3-4-5/`. They are decoded into a
unique temporary directory by `test_search`; production builds never embed
them.

| Decoded file | Bytes | SHA-256 |
|---|---:|---|
| `KQvK.rtbw` | 272 | `517667dff787162dbb1ed9d5d6484d30ee854e686ee0675c08d99ecf045d2d50` |
| `KQvK.rtbz` | 5,392 | `71ea9444fa5bd42897d781a0c356975ea6f23e0f65a4254e470897031c161c8c` |

The fixture is deliberately the smallest WDL+DTZ pair that exercises real
Fathom initialization, probing, rule-50 normalization, root move metadata,
tablebase hits, and PV expansion without a developer-local tablebase install.

## Known coverage gap: cursed wins and blessed losses

**KQvK contains no `WDL == +/-1` entries, so the cursed-win and blessed-loss
decode path is not covered by any test in this repository.** Every KQvK win is
a clean win at small DTZ.

The distinction is not pedantic here. Two different mechanisms are easy to
confuse:

| | What it is | Covered? |
|---|---|---|
| **Rule-50 clamping** | a *clean* win (WDL 2) whose halfmove clock has already run out, reported as a draw at the root, and as a win again with `Rule50` off | **yes**, `test_syzygy_rule50_root_scores` |
| **Cursed win** | `WDL == 1`: the table itself says "won, but DTZ exceeds 100 so the fifty-move rule has already drawn it", independent of the current clock | **no** |

This matters because the endgame doctrine turns on keeping WDL 2 and WDL 1
apart -- `tools/diag/endgame_truth.py` scores a move that downgrades a clean win
to a cursed win as discarding the win, not preserving it. A defect in WDL +/-1
decoding would pass the whole suite.

Closing the gap needs a table that actually contains cursed wins, the smallest
familiar one being the five-man `KNNvKP`, which is orders of magnitude larger
than these files and not worth embedding. Do it only if the WDL decode or the
Fathom integration is changed; until then this gap is accepted and recorded
rather than hidden behind a test name that implies coverage.
