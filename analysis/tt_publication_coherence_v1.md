# PLAN 9.3 (BAS-C05 repair) — how to publish a TT record coherently without paying 3.9% NPS

- State / class: `RESEARCH` (returned from `IMPLEMENTED`), `R2`
- Owner / date: agent, 2026-09-07
- Decision needed: whether the `key16 ^ fold16(payload)` tag is the right
  coherence mechanism, or whether a different record layout buys the same
  guarantee at no throughput cost.

## Known evidence

**Evidence — the registered gate ran and trends to H0.** `tt-coherent` vs
`tt-plain-key-base` (candidate `355aec1b`, base `9e52ff4f`, both PEXT+PGO,
identical 1T bench 12,568,898, `3+0.03`, 4T, Hash 256, UHO, adjudication off):
`-11.30 +/- 9.64` Elo, `nElo -17.87 +/- 15.23`, LLR `-1.27` of `(-2.94, 2.94)`
at 2,020 games. Maintainer stopped the run. Zero time forfeits, all 2,020
terminations natural, so the run itself is clean.
Artifacts: `tools/results/sprt_tt-coherent_vs_tt-plain-key-base_20260907_175623.*`.

**Evidence — the candidate is 3.9% slower at one thread.** Strictly alternating
`bench 13` on the two frozen gate binaries, 6 repetitions per arm, on this host:

| arm | mean NPS | min | max |
|---|---|---|---|
| `tt-plain-key-base` | 3,666,005 | 3,621,117 | 3,719,709 |
| `tt-coherent` | 3,522,690 | 3,474,951 | 3,569,695 |

Delta **-3.91%**, with **no overlap** between the two arms' ranges. Node counts
are exactly equal (12,568,898 both arms, every repetition), so this is pure
wall-clock speed at identical work. Limits: one PGO build per arm, and the
benches were not run under `nps_ab.ps1`'s pinned/High-priority protocol
(BAS-M06). Against that, `nps_ab.ps1` records identical-source PGO builds
differing by ~0.36%, so a build-layout explanation of a 3.91% gap needs roughly
an 11-sigma draw. The effect is thread-count independent and was measured at 1T,
where the coherence mechanism has no work to do.

**Inference — the game result is explained by speed, not by coherence.**
BAS-P01 measured roughly **2 Elo per 1% NPS at `3+0.03`** on this host. -3.91%
NPS predicts **~-7.8 Elo**, which sits inside the observed `-11.30 +/- 9.64`.
The candidate is not losing Elo because it rejects incoherent reads; it is
losing Elo because it made every TT probe more expensive.

**Evidence — the mechanism of the cost.** `probe_copy` cannot reject a slot
until it has loaded the 8-byte payload and folded it (`src/tt.h:54-59`,
`139-157`). The baseline rejected on a 2-byte compare and only loaded the
payload on a match. So the candidate adds, per probe, up to three payload loads
and three 4-operation folds on the *scan* path, and puts that fold on the
dependency chain of the loop's branch. `store()` pays three more folds
(`src/tt.h:187-211`). The cluster is 32-byte aligned inside one cache line, so
this is added latency and uops, not added misses.

**Evidence — the run was also read far too early to be a verdict, independently
of the above.** `tools/sprt.ps1:48-50` records that a 4T verdict needs ~10k
games and that "at 4T nothing under that separates 0 from +3". BAS-M02 records
the policy as fixed-N 10k at 4T. `tools/sprt.ps1:45-47` and the run's own
manifest record that `-use-affinity` is dropped at Threads>1, so the per-run
placement lottery is live and **"this thread count needs its own -Mode calibrate
null"**. No 4T calibration null exists anywhere in `tools/results/`. So the
2,020-game reading is neither large enough nor calibrated. This does not rescue
the candidate — the NPS measurement condemns it on its own, at 1T, with far
better resolution than any 4T game run could reach — but it does mean the run
cannot be cited as evidence about coherence.

**Evidence — the defect BAS-C05 describes is real and lands on cutoff paths.**
`src/search.cpp:1588-1596` returns `tt_score` directly as the node's value when
`tt_depth >= depth` and the bound matches. `src/search.cpp:1913-1968` derives
the singular beta from `tt_score` and can multicut with `immediate_score =
s_beta`. Under the plain-key baseline, key16 and the 64-bit payload are separate
publications, so a reader can match its key against one store and consume score,
depth and bound from another position's store. The repair is worth having; the
current implementation of it is not.

## Unknowns and hypotheses

- **H1 (leading).** The coherence *guarantee* is free; only this *encoding* of
  it is expensive. Moving the partial key inside the single atomic 64-bit word
  and moving `move16` out restores the baseline's probe cost (or beats it) while
  making the tear structurally impossible for every bound-deciding field.
- **H2 (credible).** Part of the 3.91% is PGO layout rather than the fold. Cheap
  to separate: build H1 and re-measure under `nps_ab.ps1`. H1 predicts ~0%; if
  H1 also lands near -4%, the cost is layout and H2 wins.
- **H3 (credible, weaker).** The 4T loss is partly a placement draw on top of
  the speed loss. Only a 4T calibration null can bound this, and it is needed
  regardless of what happens to this leaf.

## Proposed mechanism — one atomic word carries the whole validated record

Today the slot is `{ atomic<uint64_t> payload; atomic<uint16_t> key-or-tag; }`
and the argument is about how to bind two publications together. The cheaper
answer is to stop having two publications of anything that decides a bound.

The field budget makes this exact:

| field | bits |
|---|---|
| `key16` | 16 |
| `score16` | 16 |
| `eval16` | 16 |
| `depth8` | 8 |
| `flag_age8` | 8 |
| **total** | **64** |

So the word becomes `key16 | score16<<16 | eval16<<32 | depth8<<48 |
flag_age8<<56`, and `move16` moves to the separate 16-bit field. The cluster
stays `3 x 8 + 3 x 2 = 30` bytes, `alignas(32)`, `sizeof == 32` — both existing
`static_assert`s at `src/tt.h:85-87` still hold, and the 8.5.D1 density that
measured +4.27 is untouched.

Consequences:

- **Coherence is structural, not probabilistic.** The candidate leaves a
  1/65536 residual (a torn pair is accepted when `fold16(P_new) ==
  fold16(P_old)`). Under this layout the key and every bound-deciding field are
  one atomic store, so the torn-pair probability for those fields is **zero**.
  Detection strength for a genuinely different position falls back to the plain
  1/65536 key16 collision, the same as the baseline and as Stockfish.
- **Probe cost returns to baseline or better.** Scan: one 8-byte load per slot,
  compare its low 16 bits. The baseline needed one 2-byte load per slot *plus* an
  8-byte load on a match; the candidate needed 8-byte + 2-byte + a fold on every
  slot. Same cache line throughout.
- **`store()` gets cheaper than baseline.** `entry_quality` needs depth, flag and
  age, which now arrive in the same load that carries the key — one load per slot
  instead of two.
- **Replacement decisions become coherent for free.** The baseline chose a
  victim from a possibly-torn `old_entry` (`src/tt.h:187-211`); that input is now
  one publication.

### What this deliberately does not close

`move16` is outside the guarantee. A reader can get a valid word for its key
together with a move published by a different store. This is a real reduction in
scope versus the candidate, which covered the move too, and it must be stated
rather than glossed.

It is defensible because `move16` is the one TT field this engine already
validates at every consumption point, and its remaining influence is ordering:

| consumer | file:line | guard |
|---|---|---|
| `MovePicker` TT stage | `src/search.cpp:792-799` | piece present, correct colour, `is_legal` |
| qsearch capture ordering | `src/search.cpp:1407` | compared only against generated legal captures |
| `tt_capture` LMR input | `src/search.cpp:1603-1605` | accessors masked, always in-bounds; feeds a reduction only |
| singular gate | `src/search.cpp:1913` | `m` is generated-legal; a foreign move can only fail to match |
| IIR condition | `src/search.cpp:1736` | can only suppress or allow a reduction |
| `ponder_from_tt` | `src/search.cpp:1299-1300` | `is_legal_move_on_board` |
| move preservation on `MOVE_NONE` store | `src/tt.h:213-216` | carries a foreign move forward; re-validated on next read |

All `Move` accessors mask (`src/move.h:41-44`), so an arbitrary 16-bit value
decodes to in-bounds squares and cannot index out of range. Critically, the
singular multicut at `src/search.cpp:1965-1968` returns `s_beta`, derived from
`tt_score` — with this layout that value is guaranteed to belong to our key, and
the cutoff is additionally verified by a real search. That is the path where the
baseline's incoherence actually mattered, and it closes.

Optional hardening, not recommended by default: store `move16 ^ fold16(word)`
so a foreign move decodes to noise. It reintroduces a fold, on the hit path
only, to convert "a valid move from another position" into "a random 16-bit
value" — both are equally wrong for ordering and both are rejected by
`is_legal`. Cost for no measurable benefit.

## Interaction map

Producer `TranspositionTable::store` -> stored state (one atomic word per slot +
one loosely-published move) -> consumers:

- **TT cutoff** `src/search.cpp:1588-1596` — consumes score+depth+flag. Was
  exposed; now sound.
- **Singular extension / multicut** `src/search.cpp:1913-1968` — consumes
  score+depth+flag and produces a cutoff value. Was exposed; now sound.
- **TT-bound-eval-for-pruning (6.1, +7.18)** `src/search.cpp:1615+` — consumes
  `static_eval` together with score and flag. Those three were already mutually
  coherent (same payload) but not coherent with the key; now all four are.
- **Rule-50 damping (+3.29)** `src/tt.h:248-254` — `score_from_tt` takes the
  clock; unchanged, but now applied to a score that provably belongs to this key.
- **Aging / replacement** `src/tt.h:134-137`, `287-293` — age and flag ride in
  the same word as depth, as before, and now also with the key.
- **`hashfull`** `src/tt.h:226-239` — reads slots with no key check at all, by
  design; it feeds a UCI info string only. Unchanged, and worth a comment saying
  so explicitly.
- **Lazy SMP** — the entire mechanism is inert at Threads=1; the deployed 4T
  configuration is the only place it can act.

### Things that must be redesigned to accept this

1. **`tests/test_tt.cpp::test_concurrent_publication_stays_coherent`** (added by
   `355aec1b`) asserts that a concurrently observed record has a coherent
   **move** as well as score/eval/flag. That assertion is stronger than the
   proposed contract and will fail. It must split into: (a) the validated record
   is coherent — must hold, tightened to also assert `key16`; (b) `move16` is
   explicitly outside the guarantee, with the test asserting that every consumer
   rejects or safely tolerates a foreign move rather than asserting the move
   matches.
2. **`tests/test_tt.cpp::test_torn_publication_is_rejected`** cannot be written
   against the new layout at all — a torn validated pair is unconstructible
   through the API. Replace it with a structural test: a `static_assert` that the
   word carries key+score+eval+depth+flag, plus a negative control that a
   deliberately re-split layout fails.
3. **`DESIGN.md` section 3** needs the invariant written down: *every field that
   can decide a bound, a cutoff or an extension is published in one atomic word
   together with its partial key; `move16` is explicitly outside that guarantee
   and every consumer must validate it before use.* Right now the tolerance is
   implicit and each consumer re-derives it.
4. **`tools/run_tt_coherence_sprt.ps1` is deleted** (maintainer decision,
   2026-09-07). It existed only to rebuild frozen revisions in temporary
   worktrees for a second machine, a workflow that will not recur. Gate arms are
   now built here with `tools/build_test.ps1` and handed over as one `sprt.ps1`
   command line.

## Cheapest discriminating tests

In order; stop as soon as a step fails.

1. **1T `bench` fingerprint.** The re-pack is a pure relabelling of bits, so the
   bench must stay exactly **12,568,898**. Anything else means the pack/unpack is
   wrong. Cost: seconds. This is the primary correctness instrument.
2. **`nps_ab.ps1`, pooled, >=2 PGO builds per arm, proposal vs
   `tt-plain-key-base`.** Decides H1 vs H2. Cost: ~1h, agent-runnable.
3. **Focused TT tests + ASan/UBSan/TSan TT/search/threading suites**, including
   the redesigned tests above and the existing collision-stress repetitions.
4. **4T `-Mode calibrate` null, 10,000 games.** Owed regardless; without it no
   4T number in this repository means anything. Maintainer-owned overnight run.
5. **4T SPRT `[-5,0]`, 10,000 games, adjudication off**, only after 4 passes.
   Maintainer-owned.

Known-bad controls: the current `355aec1b` binary must reproduce ~-3.9% under
step 2; a deliberately re-split layout must fail the step-3 structural test.

## Prospective prediction — freeze before exposure

Frozen 2026-09-07, before building anything.

- **Diagnostics.** Bench exactly 12,568,898. Pooled NPS versus
  `tt-plain-key-base` in **[-0.5%, +1.5%]**, central expectation **+0.3%** (store
  loses a load per slot; probe is a wash). Pooled NPS of `355aec1b` versus the
  same baseline reproduces **-3.0% to -4.5%**.
- **Elo.** Versus `tt-plain-key-base` at 4T: **0 +/- 3**, i.e. a non-regression,
  not a gain. The coherence repair fixes an event rare enough that I do not
  expect it to be visible in 10,000 games; its justification is that it removes
  a wrong-position cutoff from `search.cpp:1589` and `1967`, not that it scores.
  I explicitly predict this will **not** recover the 11 Elo, because I claim
  those 11 Elo were never lost to coherence.
- **Probability the design is useful** (correct, and NPS-neutral within the band
  above): **0.8**. Probability it is measurably *faster* than baseline: 0.35.
- **Confidence in the diagnosis** that the candidate's loss is throughput, not
  coherence: **high** — 6 alternating repetitions, zero overlap, identical node
  counts, and a mechanism visible in the diff.
- **Likely failure mode.** The 8-byte scan load is worse than expected because
  the baseline's three 2-byte key loads at offsets 24/26/28 were being merged
  into one load by the compiler, so the baseline scanned with one load where the
  proposal needs three. If pooled NPS comes back at -1% to -2%, this is why.
- **Falsifiers.** (a) Bench differs from 12,568,898 -> the re-pack is wrong,
  return to research. (b) Pooled NPS outside [-0.5%, +1.5%] -> H1 is wrong as
  stated; do not "rescue" it by trimming fields, report it. (c) `355aec1b` does
  **not** reproduce -3% to -4.5% pooled -> my diagnosis is wrong, H2 wins, and
  the whole packet must be rewritten. (d) Any consumer of `move16` is found that
  can change a bound rather than ordering -> the field split is unsound.
- **Stop rule.** Two consecutive layout attempts failing falsifier (b) close
  this as `NOT_WORTH_PURSUING` at 10-byte density, and the question becomes
  whether coherence is worth 16-byte slots — which costs the +4.27 that 8.5.D1
  measured, and would need its own gate.

## Decision

`READY_FOR_IMPLEMENTATION` for the layout change, conditional on the maintainer
accepting the `move16` scope reduction, which is the one judgement call here.

`355aec1b` is **rejected**: it does what it claims and costs 3.91% NPS at
identical nodes, which on this host and TC is worth about -7.8 Elo. Per its own
registered rule, H0 does not license restoring incoherent reads.

Retry trigger for the XOR-tag mechanism specifically: it becomes worth
revisiting only if a future layout has no spare bits for the key inside the
atomic word *and* pooled NPS shows the fold costing under 0.5%.

## Implementation handoff

- **Goal.** Pack `key16 | score16<<16 | eval16<<32 | depth8<<48 | flag_age8<<56`
  into the existing `atomic<uint64_t>`; move `move16` into the separate
  `atomic<uint16_t>`. Delete `tt_fold16` and the tag scheme.
- **Why it should work here.** The engine's exposed consumers
  (`search.cpp:1589`, `search.cpp:1967`) read score/depth/flag; those fit in the
  same word as the key with zero bits to spare, so the guarantee becomes a
  property of the store instruction rather than of a checksum.
- **Producer -> state -> consumer.** `store` publishes move16 first, then the
  word with `memory_order_release`; `probe_copy` loads the word with
  `memory_order_acquire`, then move16. On x86 both are plain accesses; the pair
  matters on the ARM64 targets the project ships, exactly as the baseline
  comment at the removed `src/tt.h:190-205` argued.
- **Files.** `src/tt.h` (layout, `probe_copy`, `store`, `clear`, `hashfull`,
  `entry_quality`); `tests/test_tt.cpp` (items 1-2 above); `DESIGN.md` section 3
  (item 3); `tools/run_tt_coherence_sprt.ps1` (item 4).
- **Invariants that must not change.** `sizeof(TTCluster) == 32` and
  `alignof == 32`; the empty-slot rejection where an all-zero word yields
  `key16 == 0` and is refused by the `flag_age & 3` test at `src/tt.h:148`;
  mate-score `score_to_tt`/`score_from_tt` semantics; `depth` keeping its signed
  `-1` sentinel; `INF_EVAL == 32001` round-tripping through 16 bits.
- **Cheap local qualification.** Steps 1-3 of the test list.
- **Maintainer-owned expensive gate.** Steps 4-5.
- **Acceptance/rejection.** Accept on exact bench identity + pooled NPS inside
  [-0.5%, +1.5%] + green sanitizer/TT suites + a 4T SPRT `[-5,0]` at 10,000
  games that does not reject, run after a passing 4T null.
- **Non-goals.** Do not change replacement policy, aging, cluster size, hash
  sizing, prefetch, or any search constant. Do not "recover" the 11 Elo by
  touching selectivity. Do not widen the slot to 16 bytes in this leaf.

---

## Post-exposure calibration (appended 2026-09-07; the frozen sections above are unchanged)

Implemented as `2bf43fb`. Deterministic qualification passed: bench exactly
12,568,898 on both arms, release CTest 12/12, ASan/UBSan CTest 12/12.

**Falsifier (b) fired.** Predicted pooled NPS `[-0.5%, +1.5%]`, central `+0.3%`;
measured **-1.22%, 95% CI [-1.72%, -0.76%]** against plain-key at the same head
(`tools/nps_ab.ps1`, 2 pooled PGO builds per arm, 16 alternating rounds,
identical fingerprints, A faster in 1/16 rounds). Per-build medians agree to
0.03% within arm A and 0.14% within arm B, so this is not PGO luck.

What the original causal model got wrong: I costed the probe scan as "one load
per slot either way, same cache line, therefore a wash". It is not a wash — the
baseline rejects on a 2-byte load and only widens to 8 bytes on a match, and
that asymmetry is worth ~1.2%. The **failure mode was predicted correctly and in
range** ("if pooled NPS comes back at -1% to -2%, this is why"), so the mechanism
model was sound and only the magnitude estimate was optimistic. That is the part
of the model to distrust next time: I treat same-cache-line accesses as free and
they are not.

Instrument caveat, not hidden: the self-pair read `+0.33%` against a `0.30%`
tolerance, CI `[-0.27%, +1.05%]`. It marginally failed. A 32-round re-run was
started and deliberately abandoned to save maintainer clock, because a possible
0.3% bias cannot change a 2.7-point separation between encodings — but it does
sit inside the acceptance band, so the -1.22% figure should be read as
"-1.2% +/- instrument", not as a sharp number.

**Decision stands at `MORE_RESEARCH` on the cost, `IMPLEMENTED` on the
mechanism.** Per the frozen rule the field split was NOT trimmed to rescue the
number. The constraint is arithmetic: key+score+eval+depth+flag+move needs 80
bits, so no 10-byte layout buys this coherence for free. The live options are
therefore only:

| option | probe cost | coherence |
|---|---|---|
| plain key (pre-BAS-C05) | baseline | none — the defect at `search.cpp:1589` and `1967` is live |
| **word layout (`2bf43fb`)** | **-1.22%** | every bound-deciding field, structurally |
| `key16 ^ fold16` tag (rejected) | -3.91% | as above, plus `move16`, minus a 1/65536 residual |

The word layout is **2.7 percentage points faster than the rejected tag** and is
what `dev` now carries. Whether ~-2.4 Elo is an acceptable price for closing a
wrong-position cutoff is a maintainer judgement, not a measurement, and it is
the question the registered 4T gate now asks.

## Closure (2026-09-07) — `NO_CHANGE`, risk accepted

Maintainer decision after seeing the -1.22%: revert to plain-key and record the
incoherence as an accepted risk. Neither repair ships.

Reasoning, in the order that decided it: ~2.4 Elo is a quarter of the entire
BAS-P01 speed wave; the defect has never cost a measured game; Stockfish ships
this same tolerance in this same 10-byte structure; and coherence is not
purchasable at this density, since key+score+eval+depth+flag+move needs 80 bits
against a 64-bit word. No game gate was run, because a `[-5,0]` SPRT on a true
value near -2.4 sits between the hypotheses and cannot terminate.

The risk is now recorded where the next reader will hit it -- the `src/tt.h`
comment that used to assert the mismatch was "harmless ... bounds validated",
and `DESIGN.md` section 3 -- rather than only in this packet. That correction
is the durable output of this leaf; the two rejected implementations are not.

Retry trigger: see the BAS-C05 row in the open retry map. Reasoning alone does
not reopen it. Both repairs were correct, and both lost Elo.
