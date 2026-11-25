# Texel fitting in Basilisk: the handbook

The reference for fitting Basilisk's hand-crafted evaluation: the resources,
the contracts, the commands, the surface, the traps, the numbers, and what to
do when a run goes wrong. PROCESS *Texel procedure* holds the policy in a few
rules; this holds the operation. Read it before touching a corpus or a fit.
Rarog's handbook was its template; no number here is Rarog's.

---

## 1. What a fit is

A Texel fit chooses evaluation weights `w` to minimise the mean squared error
between a game's result and a sigmoid of the score the engine serves:

    loss = mean over rows of (result - 1 / (1 + exp(-K * score(w) / 400)))^2

* **The label is the game's result,** exactly 1, 0.5 or 0 from White's side, of
  a Basilisk self-play game that ended by the rules. A continuous target is
  refused on contact: a corpus of engine evaluations once cost 77.92 Elo.
* **The score is the played score,** the value a release build serves: the
  cheap block's value where the lazy checkpoint fires, the engine's own endgame
  step, and the rule-50 damping (§5). Rows whose served score is an override (a
  known win, a bare-king tier, a drawn KPK) or a draw verdict take no part in K
  or the loss.
* **K is fitted once,** on validation at the seed, and frozen for every stage
  and arm.
* **Loss is not Elo.** A lower held-out loss accepts nothing. A fit ends in a
  baked candidate that a registered clean-PGO SPRT decides; its endgame truth
  vetoes apply (PLAN §3).

Texel refits a whole surface at zero games. SPSA spends games on a handful of
knobs with curvature; C.6 decides whether the evaluation gets one.

---

## 2. Resources

| Resource | Where | Rule |
|---|---|---|
| The Beast position pool | `A:/Chess/Beast/data/txt/positions.txt` | Read only. Never write, move or "clean" it, and ask the maintainer before reading the `A:` drive. Its FENs carry no source-game ids, so related starts cannot be proved independent (`audit_starts.py` reports what can be measured). |
| Start books | `tools/texel/data/*.epd` (ignored) | Built by `sample_fens.py`; cited by SHA-256 (the datagen manifest records it), never by path alone. |
| Self-play PGNs | `tools/texel/data/<set>.pgn` (ignored) | Each has `<set>.pgn.manifest.txt` from `datagen.ps1`: engine and book hashes, nodes, seed, natural terminations only. |
| Corpora | `tools/texel/data/<set>/` (ignored) | `train.csv`, `validation.csv`, `test.csv`, `manifest.json`, under the row contract. Never edited after publication. |
| Fit runs | `tools/results/fit-<name>/` (ignored) | One directory per run, never reused. |
| Tablebases | `D:/chess/tablebases/syzygy3456` | Read only by `relabel.py`, whose corpora name the label contract `basilisk-texel-labels-syzygy-v1` (§3). |

---

## 3. The contracts

Each is a named, versioned schema. A corpus or file of a different meaning
gets a new name. A gate is never widened to let unknown data through: it is
extended by naming what it now accepts.

| Contract | Defined in | What it guarantees |
|---|---|---|
| `basilisk-texel-rows-v1` | `extract.py` | Rows `fen,result,game_id,start_id,ply,phase`. Only rules endings are labelled, with every rejection counted, and the run fails past 1%. Train, validation and test are split by the start's hash. The three files and `manifest.json` (inputs and outputs hashed) are published together and never overwritten. The test split is opened once through `claim_test.py`. |
| `basilisk-texel-labels-syzygy-v1` | `relabel.py` | A rows-v1 corpus derived from a game-result one, its manifest naming the source by hash. Only `result` changes, and only on a row of six men or fewer with no castling right whose tables probe. There it is the verdict under the row's rule-50 clock, in Stockfish's root_probe form: a win when `dtz + clock <= 100`, a loss when `-dtz + clock <= 100`, otherwise a draw. A verdict labels only its own row. `check` proves the identity. |
| `basilisk-texel-surface-v1` | `surface.py` → `surface_v1.tsv` | Every slot of the tuner's registry is classed free, fixed or excluded, with a stage, anchor, bounds, step and reason. Built from the registry by rule; `check` fails on drift. |
| `basilisk-texel-vector-v1` | `tuner.cpp`, `bake.py` | A complete `name index value` vector carrying `# root` and `# seed` hashes (FNV-1a 64 over its canonical lines). `--init` refuses a partial one; `bake.py` refuses one whose root is not the header it would overwrite. |
| `basilisk-texel-fit-v1` | `fit.py` → `run.json` | A run's inputs and outputs, hashed, and every step's command, exit status, time and log. |

---

## 4. The tools

| Step | Command | Notes |
|---|---|---|
| Book | `python tools/texel/sample_fens.py <pool> --out tools/texel/data/<book>.epd --count N` | Five equal phase reservoirs; `--exclude-pgn` for a disjoint extension. |
| Games | `pwsh tools/datagen.ps1 -Suffix <head> -Rounds N -Nodes 8000 -Book <book>.epd -BookFormat epd -OutputPgn tools/texel/data/<set>.pgn -Seed S` | One game per opening: identical engines at fixed nodes replay a game exactly. The maintainer runs it. |
| Size | `python tools/texel/extract.py <set>.pgn --out-dir X --preflight-games 20000` | Rows per game, by split and phase. |
| Extract | `python tools/texel/extract_parallel.py <set>.pgn --out-dir tools/texel/data/<set> --jobs 16 --target-train 3500000` | Byte-identical to `extract.py`. `--audit-only` checks the quotas without publishing. |
| Surface | `python tools/texel/surface.py check --tuner <basilisk-texel>` | `fit.py` also runs `rank`, which derives the corpus's manifest. |
| Fit | `python tools/texel/fit.py --corpus tools/texel/data/<set> --out tools/results/fit-<name> --expect-bench <seed bench>` | From a clean tree. Writes the baked header and its diff into the run directory and restores the tree. |
| Bake | `python tools/texel/bake.py <run>/04-polish.txt --allow-pst` | Only after the run is reviewed, from a clean tree; the run's `eval_params.h` is the same result. |
| Test | `python tools/texel/claim_test.py claim tools/texel/data/<set> --candidate <vector hash> --purpose "..."` | Once, after model selection. |

The tuner, `basilisk-texel` (`cmake --preset release -B build/texel -DCOMP=clang -DTEXEL=ON`), has these modes. Every mode takes `--init FILE` and `--surface FILE`.

| Mode | Use |
|---|---|
| `--tune surface` | The linear fit on the manifest's free slots, under its mean anchors. |
| `--tune-kingsafety` | The king-safety descent on the manifest's knobs, each with its own step. |
| `--verify` | The model against the engine, stage by stage, on every row. |
| `--played-hash` | Equal to `eval_hash`'s `served_hash` on the same file: the played function is the release engine's. |
| `--loss` | K and the loss by phase band at any vector; `--skip` reads the rows a fit did not use. |
| `--dump-coeffs`, `--list-params`, `--dump-played` | Inputs to `surface.py` and the tests. |
| `--feature-support`, `--audit-coverage`, `--dump-eval` | Diagnostics. `--dump-eval` prints the full traced evaluation, not the played score. |

The named groups (`texel`, `all` and the rest) are the older form. They skip
the shelter and storm terms and free the collinear slots; a fit uses `surface`.

---

## 5. The model of the played score

The traced build evaluates every position in full, so each trace is whole, and
records what it did:

* **the endgame step:** `apply_endgame` left the score, scaled it by
  `num / den`, or overrode it, and whether that is a draw verdict;
* **the lazy checkpoint:** whether a release build stops there, and its value.

The tuner recomputes every stage in the engine's integer arithmetic:
* the traced terms times the weights plus the frozen additions, against the
  evaluator's own running sum;
* the taper, the endgame step and the damping;
* the cheap block's taper and step at the lazy checkpoint.

A mismatch is fatal. The gradient is the trace times the seed's scale and the
damping's slope. A row's lazy membership, and the pawnless scaler's strong
side, are the seed's: a row the fit moves across the margin keeps its seed
path.

---

## 6. The surface

`surface_v1.tsv` holds 1,159 slots: 1,025 free linear, 38 free king-safety
knobs and 96 fixed. What is fixed, and why:

* **Gauge anchors:**
  * Material, while the PSTs are free: a piece's material and its PST's level
    are one direction.
  * `KsNoqueenDen`: only the ratio with `KsNoqueenNum` matters.
  * `SafetyTable[0]`: each king reads the table once with opposite signs, so a
    constant shift of the table is invisible.
* **Mean anchors** hold a set's mean at the seed where only its shape is
  identified: both king PSTs, and the eight mobility tables, where each piece
  reads one entry.
* **Dead slots:** pawns on ranks 1 and 8, relative ranks 0 and 7, piece
  indices never read, and the `ImbTheir` diagonal.
* **Duplicates.** Each has an identity behind it, found by reading the code or
  by `rank`'s SVD on real coefficients:
  * `ImbLinear` is material and the bishop pair again.
  * `ImbOur[0]` is the pair, and `ImbOur[1]` is `BishopPairPawns`.
  * `Rook7th` is the rook PST's seventh rank.
  * `Passed[6]` is the pawn PST's seventh rank: such a pawn is always passed.
  * The taper's phase is a weighted piece count, so a piece's mg PST level
    equals phase-weighted imbalance cross terms. The queen's are held.
  * `ImbOur[6]` and `ImbOur[9]` restate the pair while no side has three
    bishops.

`rank` reads a corpus on two row sets, named apart:
* **Support** is counted on the rows the linear stages read: `fit.py` passes
  its `--max-positions` as `--support-positions`. A slot's support is the
  number of those rows whose coefficient is nonzero, counted exactly by the
  tuner's streamed `--count-support`. Slots under `--min-support` rows (200
  by default) are frozen for that corpus, and each reason gives the count
  and the rows counted. `--feature-support` counts something else: raw
  traces on every row, left-out rows included.
* **The rank** is read on the first `--rank-positions` rows (300,000 by
  default), over the slots the count left free. It must be full under the
  anchors. A free slot with no row in that sample fails by name, as a sample
  too small, apart from an identity.

**The guard.** `rank` fails, and writes no manifest, when a support-frozen
slot belongs to a table (a group of more than one slot) that keeps a free
linear member and no mean anchor. Such a table's level can move away from
the frozen entry. A frozen slot in a mean-anchored table is allowed and
printed.

The linear clamps and monotone chains stay in the tuner's code. They are
checked at the seed, never move a held slot, and run after the anchors. A
held entry in a chain bounds its free neighbours: on corpus A, the queen
mobility tables' frozen entry 0 is their floor.

---

## 7. A fit, end to end

1. Build the book, play the games, and preflight the extraction. Size the
   corpus from the binding phase's yield, not from raw rows (C.2.2).
2. Extract. Check the manifest: rejected games under 1%, zero parse errors,
   every quota met.
3. Commit everything; the tree must be clean.
4. Run `fit.py`. It:
   * builds its own traced tuner and release engine, and checks the seed's
     bench;
   * checks the surface and derives the corpus's manifest;
   * verifies the model and fits K once;
   * runs the stages king safety → linear → king safety → polish, each from
     the vector the last one wrote, never through a bake;
   * bakes, rebuilds, and checks the result as built: the vector held
     exactly, `--verify`, the new bench, the played hash, `test_eval`;
   * restores the header.
5. Read `run.json`:
   * K and the loss by band, before and after;
   * the residuals on unused train rows;
   * each stage's stop line, which must be patience;
   * `test_eval`'s line.
6. Bake the final vector in its own engine commit. A test whose precondition
   depends on fitted weights gets its fixture fixed in a separate commit, with
   the reason (C-70). C.2.8's first bake retired `test_eval`'s PST
   orientation test: it held fitted tables to PeSTO, and its criterion
   failed the correct Stockfish 16 seed and ties on a re-levelled table.
   Orientation is checked where a table is imported: `seed_sf16.py` refuses
   parsed tables that break its three chess facts.
7. Build clean PGO and run the registered gate.
8. After selection, claim the test split once and report it.

---

## 8. Traps

Each of these cost real work here.

1. **A fit stopped by its schedule is not a fit.** Full-batch Adam moves a
   one-signed coordinate about `lr` an epoch, so `lr × epochs` caps every
   move. The tuner exits 3 when the holdout still improves at the last epoch,
   or when a move reaches 0.9 of that cap. Raise the epochs or the rate; do
   not lower the check.
2. **Using the training split as its own holdout never converges:** the loss
   keeps falling and the run exits 3.
3. **Baking one block of a jointly fitted vector, or hand-reverting one,
   breaks the joint fit** (C-55). The other blocks were fitted around it. Bake
   the whole vector, or refit.
4. **A stale dump.** A vector fitted from another header reverts every member
   that header changed. `bake.py` refuses it by its root hash.
5. **python-chess's `claim_draw` is not the rules ending.** It allows a
   threefold claim a move early. The row contract counts a position that
   actually occurred three times, as the harnesses do.
6. **Exact identities hide in float32.** A coefficient dump in float32 put
   true null directions at 1e-9; in float64 they sit at 1e-16. `rank` reads
   float64.
7. **The clamps and the held slots.** An ordering clamp once raised a held
   slot past a free one and broke the next stage's seed check. The clamps now
   bound free slots by held ones.
8. **The bench fingerprint cannot see an evaluation change** the bench
   positions never reach. Trust `run.json`'s source revision and header hash,
   not bench equality alone.
9. **Loss is not Elo.** Register the gate.
10. **Check exit status directly**, not through a pipe: `cmd > log 2>&1; echo $?`.
11. **Support counted on a sample is a different threshold.** C.2.8's run 1
    froze slots under 200 rows of a 30,000-row sample, about 23,650 rows of
    the train split. That froze 242 slots where the whole split freezes two.
    Its 204 frozen PST entries stayed at the seed while their tables
    re-levelled by up to 215 cp. Support is counted on the fit's own rows.
12. **Two fits of nearly the same data differ by about 2e-5 by their path
    alone.** C.2.8's corpus-B fit was worse than corpus A's on both label
    sets by that much. Compare matched arms on each other's labels, and take
    the difference of their advantages.

---

## 9. Reference numbers

Measured on the Ryzen 9 5950X in 2026-11 (C.2.1). The fit timings are for a
100,000-row corpus; scale them by rows.

| Quantity | Value |
|---|---|
| Registry | 1,159 slots (1,025 free linear, 38 knobs, 96 fixed) |
| Seed bench | 4,777,412 |
| Datagen, 8,000 nodes, concurrency 31 | 125,000 games in 36 minutes |
| `extract_parallel.py`, 14 jobs | 125,000 games (354 MB) in 1 min 38 s |
| Rejected by the rules-ending check, that PGN | 0 of 125,000 |
| Rows left out of the fit (overrides and verdicts) | about 7% |
| Rows at the lazy checkpoint | about 9% |
| K on that corpus's validation | 1.714; by band 1.44 (opening) to 1.94 (endgame) |
| `rank` on 30,000 rows (C.2.1, support on the sample) | 2 min; under 200 rows of support, 269 slots |
| Support on corpus A's train split (3,547,892 rows) | under 200 rows: 2 slots, `MobQMg[0]` and `MobQEg[0]` |
| `rank` on 300,000 rows, corpus A | 1,013 of 1,013 under the anchors |
| `fit.py` end to end, 100,000 rows, trees built | well under a minute after the builds |
| Full scale, 3.5M rows | sparse rows; a dense matrix would need 14 GB |

---

## 10. Recovery

* **A run fails.** `run.json` names the step and its log. `src/eval_params.h`
  has already been restored, and `run.json` says so (`header_restored`).
  Start again in a new run directory.
* **A run is killed hard.** Nothing restored the header: run
  `git status src/eval_params.h`, then `git checkout -- src/eval_params.h`.
  The run's `run.json` holds the original header's hash.
* **A stage exits 3.** Rerun with more epochs or a higher rate, from the same
  seed. Never lower the check.
* **The test split was claimed by mistake.** It is spent: build a new corpus
  with a new seed. Never delete a claim.
* **`surface.py check` fails** after an evaluation change. Rebuild the
  manifest (`surface.py build`), review the diff, rerun `rank`, and commit it
  with the change.
* **`rank` finds a null direction.** Name the identity, add the rule that
  holds one slot of it, with its reason, to `surface.py`, and rebuild the
  manifest.

---

## 11. Related documents

* PROCESS *Texel procedure*: the policy.
* PLAN C.2.1's sub-steps: what each contract settled and how it was checked.
* `analysis/rarog_audit_2026-10-24.md` (C-*) and
  `analysis/code_audit_2026-10-27.md` §6 (T1–T5): the defects this toolchain
  closes.
* `tools/texel/README.md`: the tools at a glance.
