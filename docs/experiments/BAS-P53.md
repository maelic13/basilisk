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

#### RESULT

Idle host, 2026-11-15. `b89-final` reproduces `bench 13` and the cohort fingerprint on
all four builds. Archives `tools/results/b89-pass-bench`, `b89-pass-loaded`,
`b89-d9.json`, `b89-profile-bench`, `b89-profile-games`.

| Read | `b84-on` | `b89-final` | Delta | 95% interval | Verdict |
|---|---:|---:|---:|---|---|
| pooled `bench`, step 1 | 2.3958M | 2.5407M | +6.05% | [+5.55%, +6.55%] | ACCEPT |
| loaded cohort, 14 engines | 2.1028M | 2.2008M | +4.66% | [+4.49%, +4.83%] | loaded rule met (one cycle disturbed, absorbed) |

- **D-9** (`bench 13 3`, pinned, best of three, four repeats): `b84-on` 1T 2.374M to
  2.385M, 4T 9.61M to 9.69M, ratio 4.03 to 4.08; `b89-final` 1T 2.491M to 2.529M, 4T
  9.99M to 10.42M, ratio 3.95 to 4.18. No collapse of placement or code; the
  four-thread readings do not resolve a difference between the arms (BAS-P36).
- **Profile** (`profile.ps1`, same method as B.8.5; shares of all samples):

  | Region | `bench 13` ×5, B.8.5 → now | Game cohort, B.8.5 → now |
  |---|---|---|
  | ordering | 24.4% → 24.9% | 25.2% → 23.4% |
  | board | 23.3% → 24.1% | 22.2% → 23.3% |
  | evaluation | 22.6% → 19.4% | 18.1% → 15.3% |
  | search | 17.2% → 17.7% | 15.7% → 15.8% |
  | table | 6.1% → 6.0% | 13.0% → 13.7% |
  | attacks | 6.0% → 7.6% | 5.1% → 5.8% |
  | outside the engine | 0.2% → 0.2% | 0.4% → 2.6% |

  Samples for the same work: 10,207 → 9,105 (`bench`), 63,779 → 60,660 (cohort).
  Top frames, cohort: `evaluate_full` 14.10% → 11.69%, `probe_copy` 6.78% → 7.16%,
  `score_quiets` 7.01% → 6.97%, `CoreMovePicker::next` 4.27% → 3.28%,
  `partial_insertion_sort` 3.72% → 3.78%. Full LTO took the most from the evaluation;
  B3 from `next`; the table's wait is a larger share of a faster node.
- **Unexplained:** the cohort's samples outside the engine image rose from 0.4% to
  2.6% (1,556 samples); not investigated.
- **Prediction, the pass (§8):** bench +3.5% [+1.5, +6.0]: +6.05%, just above the
  interval; loaded +6% [+3, +10]: +4.66%, inside. It came from the build and the code
  items, not from the memory items, the reverse of the packet's ranking. The pass's
  falsifier (under +1.5% on `bench` with A1 kept) cannot fire: A1 was not kept.
- **Disposition: RECORDED.** The pass stands at +6.05% on `bench` and +4.66% under
  load, about +9 to +12 Elo at `3+0.03` by BAS-P01's 2 Elo per 1% (not measured; the
  tree is identical, so no game is owed).
