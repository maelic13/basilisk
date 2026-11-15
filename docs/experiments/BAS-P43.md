# BAS-P43

**BAS-P43 — the build: full LTO, IR-level PGO, both (B.8.9, item C)**

- **Registration.** 2026-11-15, B.8.9, before the reads. Agent-run on the idle host.
- **Change.** Two CMake options, both off by default (`a662fd9`): `FULL_LTO`
  (Clang's `-flto=full` for the IPO targets in place of ThinLTO) and `IR_PGO`
  (Clang's `-fprofile-generate` / `-fprofile-use` in place of the front end's
  `-fprofile-instr-generate` / `-fprofile-instr-use`); both forwarded to the PGO
  children. The training stays `bench 13` at one thread and the training build stays
  without LTO. The source is unchanged.
- **Arms.** `b89-head` (the head's pool, A1 to A4 reverted; the options' defaults
  build the same code) against three pools of four final-PGO builds each:
  `b89-c-lto` (`-ArmOption FULL_LTO -Arm On`), `b89-c-irpgo` (`-ArmOption IR_PGO
  -Arm On`), `b89-c-both` (a throwaway revision of `a662fd9` with both defaults on,
  built by `nps_pool.ps1 -Revision`; cited by that recipe). Every arm must hold
  `bench 13` 4,688,537 and the cohort fingerprint; one that moves is a compiler
  finding, not a candidate.
- **Prediction (packet §8, frozen).** The best arm: bench +0.8% [−0.5, +2.5] (80%);
  P(kept) 0.50.
- **Reads.**
  - One pooled `bench` read of the four arms (`nps_read.ps1`), three gain contrasts
    against `b89-head`.
  - The cohort read at one engine (`cohort_read.py run --engines 1`) of the four
    arms, three contrasts against `b89-head` (the held-out workload: the build
    trains on `bench`).
  - A second read, only on a winner (an arm kept by the rule below at +0.5% or
    more): the winner with `-mllvm -inline-threshold=500` at the compile and the
    link and ext-tsp block placement, against the winner, registered then.
- **Verdict rule (packet §6, frozen).** An arm is kept when its `bench` gain
  contrast ACCEPTs by the standing method (+0.9% at step 1, or +0.5% with the lower
  bound above 0 at step 2) and its one-engine cohort estimate is −0.5% or above. Of several kept arms the one with
  the largest `bench` estimate is the winner. A kept arm owes CI's and the release's
  flags, the universal binary's tiers and `check_universal.py`.

#### RESULT

Idle host, 2026-11-15. All twelve arm builds reproduce `bench 13` 4,688,537 and the
cohort fingerprint 24,421,195 `8bc7f3c8d1dde0d2`. Archives
`tools/results/b89-c-bench` (step 1 decided every contrast), `b89-c-single`.

| Arm | Pooled `bench` | 95% interval | Verdict | Cohort, one engine | 95% interval |
|---|---:|---|---|---:|---|
| `b89-head` | 2.3932M | | | 2.3901M | |
| full LTO | +3.23% | [+2.94%, +3.52%] | ACCEPT at step 1 | +2.56% | [+2.29%, +2.84%] |
| IR PGO | −0.70% | [−1.03%, −0.37%] | NO_CHANGE | −0.68% | [−1.00%, −0.37%] |
| both | +2.53% | [+2.26%, +2.80%] | ACCEPT at step 1 | +1.93% | [+1.62%, +2.24%] |

- **Disposition: full LTO KEPT** (`4612bbf`, `FULL_LTO` on by default), the winner by
  its `bench` estimate; both kept arms pass the cohort's −0.5% floor. IR-level PGO
  loses alone and costs about 0.7% inside the pair, so it stays off.
- Prediction, the best arm +0.8% [−0.5, +2.5]: beaten (+3.23%, above the interval).
- **Owed with the kept arm:** CI's and the release's builds take the preset's
  default, so they take full LTO without a flag. The universal binary
  (`build_test.ps1 -Universal`) builds with it and its four tier builds and the
  dispatcher each reproduce 4,688,537 on this host (the pext tier); the Intel SDE
  check (`check_universal.py`) needs SDE, not installed here, and runs in the
  release workflow. `hot_path_calls.py` needed the null move's undo path in its
  KNOWN list (`1255e75`): full LTO inlines it.
- The second read on the winner is BAS-P44.
