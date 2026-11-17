# Syzygy test fixture

These files are base64 encodings of canonical three- and four-piece Syzygy tables from
`http://tablebase.sesse.net/syzygy/3-4-5/`. Each table set is decoded into its
own unique temporary directory (`tests/syzygy_fixture.h`): `KQvK` by
`test_search` and `test_engine_threading`, `KPvK` by `test_endgames`'s KPK
truth test, which probes every legal KPK position. Production builds never
embed them.

| Decoded file | Bytes | SHA-256 |
|---|---:|---|
| `KQvK.rtbw` | 272 | `517667dff787162dbb1ed9d5d6484d30ee854e686ee0675c08d99ecf045d2d50` |
| `KQvK.rtbz` | 5,392 | `71ea9444fa5bd42897d781a0c356975ea6f23e0f65a4254e470897031c161c8c` |
| `KPvK.rtbw` | 7,824 | `63ad9e15cd0f5e91e42e6f669a9f6116ae9f9eabd85757e9fb28d2be6074aed9` |
| `KNNvK.rtbw` | 1,360 | `8b5195abd7f1d4be7aa4b4dca669e9075d61d1c6b02dec8b71f5e9ea21d9eced` |
| `KBNvK.rtbw` | 7,632 | `76958da65bc243f2f0a16ceb83f1c00cbc4fb6d6e2f43d052c2f9d65bf16fee8` |
| `KBvKB.rtbw` | 1,232 | `f0bfff470a9775c0f6693d7c1b73a0879cfed64f9b50cb56872cc15dbe8336ad` |
| `KNvKN.rtbw` | 1,168 | `290b5d295f531ac680e35a3295ff265e89da864fc0363b5d7aca928c8e38faf6` |
| `KPPvK.rtbw` | 25,104 | `55aa91c460bc5eea322b8a6cde4f94d2e0a244b6fc14f6aca17e6ccfb2e56ef6` |
| `KRvKR.rtbw` | 12,944 | `797081b5b3fbac20c4baa5453d68bdae1425b63895f5c9a5ac5c68ea63a97712` |
| `KRvKB.rtbw` | 32,912 | `69b247cb390ffcd729cf5fe5c40f8a24803190bf67b108b8f05b8b08b26a6a72` |

`KPvK` ships its WDL file alone: WDL probes need no DTZ table.

The seven four-man WDL tables listed after KPvK are the hard-verdict instrument's
(`tests/endgame_verdicts.cpp`, `verdict_fixture_path`): one small table for
each three- and four-man site it reads, the sites over B.9.3's line among
them. They are the same canonical tables, 82 KB together.

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
