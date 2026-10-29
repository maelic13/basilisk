# BAS-P26

**BAS-P26 — instruction-specific speed: compressed PEXT tables and code generation**

- **Registration.** Registered 2026-10-29, before any pool was built and
  before any reading; PLAN A.17. Agent-run overnight at the maintainer's
  request (the host idle all night): `pwsh tools/run_a17_nps_reads.ps1`.
- **The survey that chose the arms** (`tools/results/a17-profile/head/`,
  a Profile build of `e7bf352`, `bench 13` ×5, 32,862 samples): board 26.7%,
  move ordering 24.6%, search 19.8%, evaluation 17.2%, attacks 5.4%, TT 4.7%.
  Within them:
  - slider lookups 2.7% to 3.2% (`bishop_attacks` 1.61%, `rook_attacks`
    1.10%, `queen_attacks` 0.48%): candidate 1's ceiling;
  - king shelter and storm 0.75%, the only evaluation loops a PEXT-indexed
    table could replace: candidate 2's ceiling, below what a pooled read
    resolves (about ±0.5%), so it is not built;
  - the scalar hotspot is the material imbalance, 2.89%, 48 products of
    piece counts on every evaluation; it depends on material alone, so a
    material-keyed cache (B.7's speed work) beats hand vectorisation:
    candidate 3 is not built, and candidate 4 has nothing to build on.
- **Instrument.** `tools/nps_read.ps1`, decided by `tools/diag/nps_read.py`
  (PROCESS *Build, bench and NPS*), gain form.
- **Builds**, seven pools of four `build_test.ps1` Release builds, Clang
  23.1.4, one revision, every build `bench 13` 19,470,325:
  - `a17-pext`: the pext asset's recipe, the control;
  - `a17-comp`: pext with `PEXT_COMPRESSED=ON` (candidate 1: the slider
    sets stored as 16-bit pext indices over their empty-board rays,
    expanded by `pdep`: 210 KB instead of 840 KB, inside the 512 KB L2);
  - `a17-tune`: pext with `-mtune=znver3` (candidate 5, the tuning model);
  - `a17-v3`: pext with `-mfma -mf16c -mmovbe` (candidate 5, the rest of
    x86-64-v3, which every pext-tier CPU has);
  - `a17-native`: `-Native` (BAS-P22's native build);
  - `a17-natgen`: `-Native` with `-mtune=generic` (native's instruction
    families without its tuning);
  - `a17-natalign`: `-Native` with `-falign-functions=64` (native's layout).
- **Contrasts**, gain form: `comp` (comp over pext), `tune` (tune over
  pext), `v3` (v3 over pext), `native` (native over pext), `natgen` (natgen
  over pext), `nattune` (native over natgen: znver3 tuning inside native),
  `natalign` (natalign over native).
- **Question.** Does code written for the pext tier's instruction set, or a
  different code-generation choice, make the engine measurably faster on
  this Zen 3 without changing a move?

#### PRE-REGISTERED PREDICTION — freeze before exposure

| Contrast | Prediction | 80% interval |
|---|---:|---|
| `comp` | +0.3% | [−0.5%, +1.2%] |
| `tune` | −0.5% | [−1.5%, +0.5%] |
| `v3` | 0.0% | [−0.4%, +0.4%] |
| `native` | −1.0% | [−1.6%, −0.4%] |
| `natgen` | −0.3% | [−1.0%, +0.4%] |
| `nattune` | −0.7% | [−1.4%, 0.0%] |
| `natalign` | 0.0% | [−0.8%, +0.8%] |

The basis: the lookups are under 3% of a node and `pdep` costs three cycles
on Zen 3, so compression gains only where the flat tables miss the L2;
the integer engine has no use for FMA or F16C; BAS-P22's native loss is
taken to be Zen 3's tuning model, not its instruction families.

- **Decision rule.**
  - `comp` ACCEPT: `PEXT_COMPRESSED` becomes the pext tier's code (the
    option is removed and the compressed form kept), after the universal
    binary is rebuilt and checked with it. NO_CHANGE: the arm is removed.
  - `v3` ACCEPT: the maintainer is offered the pext tier with FMA, F16C and
    MOVBE, the dispatcher checking those bits; NO_CHANGE: nothing.
  - `tune` ACCEPT: the maintainer is offered `-mtune=znver3` for local
    `release` builds on Zen 3 only (a CPU-specific choice cannot ship);
    NO_CHANGE: nothing.
  - `native`, `natgen`, `nattune`, `natalign` decide no code: they explain
    BAS-P22's native result for README and `docs/release_tiers.md`.
- **Stop rule:** step 1, step 2 only on EXTEND; a REPEAT repeats the read
  once into a fresh directory, a second stops for the maintainer.
- **Conditions:** one pinned physical core (the last) at High priority, a
  `bench 8` warm-up per build, 60 s under 10% host load, no engine, harness
  or build process.
- **Reading time:** seven pools, 28 readings a cycle: about 25 minutes at
  step 1, about 70 with step 2.
