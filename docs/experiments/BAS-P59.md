# BAS-P59

**BAS-P59 — the universal binary with full LTO, against the pext asset (B.8 delivery)**

- **Registration.** 2026-11-16, before any reading, at the maintainer's request when
  B.8 is delivered. Builds agent-run; the read needs the host idle (no Rarog work) and
  runs when the maintainer frees it.
- **Background.** The release's x86-64 asset is the universal binary: four tiers' PGO
  builds, functions aligned to 64 bytes, linked behind a dispatcher by the assembly
  route (BAS-P20, BAS-P21: universal against the pext asset +0.00%). B.8.9 changed the
  build under it: full LTO, an inline threshold of 500 at the compile and the link, and
  ext-tsp placement at the link (BAS-P43, BAS-P44, read on the single-tier pext build).
  The universal route keeps the tiers' LTO output as assembly and relinks it, so the
  link-time options act inside each tier's LTO link but not at the final link, and
  `74f6bde` binds full LTO's local type identities weak there. Whether the shipped
  binary keeps B.8.9's gain is not read anywhere.
- **Builds**, one revision (the head), clang 23.1.5, `bench 13` 4,688,537:
  `b811-pext` (four `build_test.ps1 -Flavor Release` builds, the measured pext asset);
  four `build_test.ps1 -Universal` builds, pooled as `b811-universal` and as
  `b811-tier-pext` (their pext tier builds, aligned to 64).
- **Read.** One pooled `bench 13` read of the three pools (`nps_read.ps1`), contrasts:
  - `hot`, the decision: `b811-pext` → `b811-universal`, no-regression form;
  - `place`: `b811-tier-pext` → `b811-universal`, the relink alone;
  - `align`: `b811-pext` → `b811-tier-pext`, the alignment on the single-tier build.
- **Prediction (80%).** `place` 0.0% [−0.4, +0.4]; `align` −0.2% [−1.0, +0.6]; `hot`
  −0.2% [−1.0, +0.6], PASS with probability 0.7.
- **Decision rule (frozen).** `hot` passes the no-regression form: the universal binary
  ships with B.8's build as it is. `hot` fails: B.8 still merges (the read decides no
  engine code), and the universal route's loss goes to a leaf of its own, named by
  `place` (the relink) or `align` (the alignment), before the next release.
- **Stop rule.** Step 1, step 2 only on EXTEND; a REPEAT repeats the read once into a
  fresh directory.
- **Conditions.** One pinned physical core (the last) at High priority, one warm-up
  reading per build, 60 s under 10% host load, no Rarog or other engine, harness or
  build process.

#### RESULT

Idle host, 2026-11-16, CPU 30, no cycle disturbed (`tools/results/b811-universal-bench`;
step 1 extended on `place`, six cycles).

| Contrast | Delta | 95% interval | Verdict |
|---|---:|---|---|
| `hot`: universal over the pext asset | +0.30% | [+0.19%, +0.42%] | no-regression PASS (step 1) |
| `place`: universal over its pext tier | +0.75% | [−0.32%, +1.83%] | reported |
| `align`: the pext tier over the pext asset | −0.45% | [−1.52%, +0.63%] | reported |

Means of build medians: pext asset 2.5290M, universal 2.5367M, pext tier 2.5178M (its
builds spread 2.500M to 2.534M, which widens `place` and `align`).

- **Disposition: PASS.** The universal binary ships with B.8's build: it reads +0.30%
  over the measured pext asset, so it keeps B.8.9's gain. Prediction `hot` −0.2%
  [−1.0, +0.6]: inside; `place` and `align` inside their intervals.
