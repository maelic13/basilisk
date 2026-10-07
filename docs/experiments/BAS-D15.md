# BAS-D15

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Phase 5.9 closing summary -->

**BAS-D15 — 5.7.6 dead-code removal, behaviour-neutral** (2026-08-30).

| removed | why |
|---|---|
| `check_ext_path_cap` | added inert for 5.4.4, which closed **REJECTED** (BAS-S16, −3.48 ±3.32) |
| `lmr_allow_check` | same; current policy (never reduce checking moves) hardcoded |
| `SearchStack::check_exts` | existed only to feed the removed cap |
| stale 5.4.4 doc block | described both as "awaiting an experiment" that had already failed |

| changed | why |
|---|---|
| `double_ext_max` **200 → 16** | at 200 the cap **can never bind**; BAS-D11 measured 16 as behaviour-identical across all 107 positions, so this converts a decorative valve into a real one at no measured cost |

*The distinction that governed this step:* a parked switch whose trial already
**failed** is residue and comes out — leaving it implies an avenue is open when
it is closed. A safety valve that never fires is different: it should be made
capable of firing, not deleted. Deleting `double_ext_max` would have removed the
only bound on a pathological double-extension chain; setting it to 16 gives that
bound teeth for the first time.

Verified: bench **12,709,666** unchanged, CTest 12/12, WAC 137/300.
