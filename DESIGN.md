# Basilisk design contract

The engine-specific facts an agent must know **before** proposing a mechanism.

This document is deliberately small and it does not duplicate the others:

| File | Holds |
|---|---|
| `AGENTS.md` | how to work: unit of work, gates, refusal obligations |
| `PLAN.md` / `GUIDE.md` | what to work on, in order |
| `EXPERIMENTS.md` | what has been tried, what it measured, retry triggers |
| **`DESIGN.md`** | **what is true about this engine and must stay true** |

If a fact here and a fact in the source disagree, **the source wins and this
file is a defect**. Fix it in the same commit that discovers the disagreement.

## 1. The four questions

No mechanism gets implemented until these are answered in writing, in the
report, before the diff. They exist because chess-engine techniques are not
independent parts: two engines can both have LMR, correction history, SEE
pruning and singular extensions and still need different thresholds, because
the whole selectivity stack differs.

1. **What mechanism should produce strength?** Name the chess or search
   property. "Stockfish has it" is not a mechanism.
2. **What existing features interact with it?** Name them from this codebase,
   with file and line. If the answer is "none", that is a claim to defend.
3. **What engine invariants must remain true?** From section 3 below, plus any
   the change touches.
4. **What experiment would falsify the idea?** Registered before the run, with
   its verdict rule. An experiment that cannot fail is not evidence.

A good answer to (1)-(4) is frequently *"this does not fit the current
architecture; do not implement it yet."* That is a successful outcome of the
gate, not a failure to deliver. See `AGENTS.md`, "Refutation and refusal".

## 2. Evaluation philosophy

- **Hand-crafted, and unfrozen for structural work** until PLAN's freeze leaf.
  NNUE is a later phase and does not license HCE shortcuts now.
- **Game-result labels only.** No engine-evaluation labels in any corpus.
- **Categorical knowledge earns its place by measurement, not by taxonomy.**
  A family term is justified by a measured local defect plus occurrence, not by
  the reference engine having a function with that name.
- **A gradient and a recogniser are different instruments.** BAS-E53 measured
  an only-move failure class and refuted the blanket claim that an evaluator
  cannot resolve it: Stockfish resolved the paired nodes at the same nominal
  budget. That result does not identify a transferable HCE mechanism or make a
  recogniser equivalent to a gradient; current 6.8.a remains `RESEARCH`.
- **Draw scaling never asserts a draw it cannot prove.** Heuristic scaling is
  floored (`SCALE_FLOOR`) so a misfire discounts rather than throwing a win.

## 3. Invariants

Verified against the source at the revision this file was last touched. Each
line names where it lives.

### Score and perspective
- `apply_endgame` receives and returns a **white-perspective** score
  (`src/eval.cpp`). Conversion to side-to-move happens once, at the return.
- The evaluator is called on **two paths**: the full tail and the **lazy** path
  (`src/eval.cpp`, guarded by `LAZY_MARGIN = 700`). Anything added to
  `apply_endgame` runs on both, and the lazy path has **no** `passed[]`
  bitboard — recompute what you need from the board.

### Mate scores
- `MATE_SCORE = 32000`, `MAX_PLY = 128` (`src/search.h`), **duplicated** in
  `src/tt.h`. They must agree; `tests/test_endgames.cpp` static-asserts
  `KBNK_STATIC_MATE_FLOOR == MATE_SCORE - MAX_PLY`. Changing one without the
  other is a silent corruption.
- The mate band is `|score| >= MATE_SCORE - MAX_PLY` = **31872**. Any static
  score must stay strictly below it, or the search reads technique as mate.
  `KNOWN_WIN = 10000` is the "won, mate is technique" magnitude.
- TT scores are **ply-adjusted on both store and probe** (`src/tt.h`). A raw
  stored mate score is meaningless without its ply.

### Endgame scaling
- `SCALE_NORMAL = 64`, `SCALE_DRAW = 0`, `SCALE_MAX = 128` (amplifies),
  `SCALE_FLOOR = 24` (`src/eval.cpp`).
- **Aggressive scale factors are not free.** Unfloored reference rules cost
  +79% and +108% bench nodes by flattening the evaluation across the region
  they cover (BAS-E54). Cost must be measured, not assumed.
- **A family term's blast radius is its dispatcher's promotion closure.** A
  KBNK term reaches KBP-K, KBP-KB, KBP-KN and KBPP-KB through knight
  promotion (BAS-E51). Non-regression sets must cover the closure, not the
  families the safety argument already excluded.

### Transposition table
- 32-byte cluster, 3 entries, 10-byte slots (`src/tt.h`).
- The 16-bit field is a **tag**, `key16 ^ fold16(payload)`, not a plain key. A
  reader reconstructs the key from the payload it actually observed, so a
  payload and tag from different publications fail validation. Detection is
  16-bit, the same strength as the old partial key (BAS-C05).
- Both fields are **relaxed** atomics, and that is deliberate: the tag makes
  detection order-independent, so no release/acquire pair is required.
- An empty slot is `(data 0, tag 0)`, which reconstructs `key16 == 0`. The
  `flag_age` check is what rejects it when `want == 0`. Preserve that.

### Match and measurement
- **Score-based adjudication is off by default in every tool.** Opt-in runs
  must record the policy and never mix with natural-termination evidence.
- Strength is measured at **3+0.03**, paired UHO openings, normalized Elo,
  **tablebases off** — the harness and the games are blind the same way.
- **Bench signature** of the accepted head: `bench 13` = **12,568,898** nodes
  (12,709,666 before 6.5.a). Exact bench identity is a provenance fingerprint,
  **not** proof of behavioural identity: evaluation activation, terminal logic
  and time handling can change play while bench stays equal.

## 4. Measurement doctrine

The four layers, kept separate (`analysis/endgame_measurement_layers_v1.md`):

| Layer | Question | Unit |
|---|---|---|
| Theory truth | did it throw a won position? | one move |
| Move quality | is it making progress? | one move |
| Conversion | did it finish inside the rules? | one position |
| Game strength | does it win more games? | one game pair |

- **Theory truth is an absolute veto.** Conversion never establishes strength.
- **Node budget is a first-class run condition.** A decision taken at 60k does
  not necessarily hold at 200k or 600k (BAS-E45); state and justify it against
  the deployment control.
- **Occurrence gates the ceiling.** KBN-K occurs zero times in trees from real
  roots; KRPPKRP occurs 352 times (BAS-E43).
- **Accepting H1 is a decision, not an effect size.** Report the point estimate
  and interval beside the verdict.

## 5. Performance assumptions

- **The board is not the bottleneck, and "make it faster" is not a motivation.**
  BAS-X16 measured this source ahead of a peer engine by 22-46% on every board
  microbenchmark. Phase 7 is a **defect hunt**; optimization leaves default to
  no-change.
- **Nodes-to-depth, not raw NPS, is where the peer gap lives.**
- Bench node counts are **chaotic** under small evaluation perturbations: in
  the 6.5.a floor sweep, floor 32 cost +28.5% while 24 and 40 sat near
  baseline. Never select a parameter by minimizing a bench curve.

## 6. Known-rejected register

`EXPERIMENTS.md` is the authority, with retry triggers. Do not re-propose a
closed mechanism without meeting its recorded trigger. Consult it *before*
answering question (1), not after writing the diff.
