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
