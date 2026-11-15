# BAS-P53

**BAS-P53 — the speed pass whole, against B.8.5's head; D-9; the profile again (B.8.9)**

- **Registration.** 2026-11-15, B.8.9, after the last item and before the closing
  reads' first counted cycle (written while the final pool built; committed during
  the pooled read's warm-up). Agent-run on the idle host.
- **The pass as kept.** Full LTO (BAS-P43), the inline threshold and ext-tsp
  (BAS-P44), the kernel's parameters as constants outside Tune (B2, BAS-P46) and the
  picker's stages falling through (B3, BAS-P47). Reverted: A1 to A4, B1, B4, B5, S1,
  S2. The verify switch removed. Head `d17a62d`, `bench 13` 4,688,537.
- **Arms.** `b84-on` (B.8.5's head, BAS-P35's pool) against `b89-final`, four
  final-PGO builds of the head.
- **Prediction (packet §8, frozen).** The pass: bench +3.5% [+1.5, +6.0], loaded
  +6% [+3, +10] (80%). The pass's falsifier: under +1.5% on `bench` with A1 kept (A1
  was not kept, so it cannot fire as written).
- **Reads.**
  - The pooled `bench` read, gain form, step 2 when step 1 extends.
  - The loaded cohort read (BAS-P37's instrument), gain form.
  - D-9, B.8.4's pinned scaling check (BAS-P35's method): `bench 13 3` at one thread
    on CPU 0 and at four threads on CPUs 0, 2, 4 and 6, each arm's first build, four
    interleaved repeats; reported, a collapse (four threads under 3.5 times one) is a
    finding.
  - The profile again on both cohorts (`profile.ps1` on a `-Flavor Profile` build of
    the head; `bench 13` ×5 and the game cohort at 150,000 nodes), against B.8.5's.
- **What the reads decide.** Nothing is kept or reverted by them; they are the pass's
  record. The pooled read's estimate is the number the CHANGELOG cites.
