# Basilisk HCE analysis

Status: working analysis, intended to be extended  
Analysis date: 2026-07-13  
Basilisk revision: `d4dd504` (`development`)  
Comparison baseline: Stockfish 18/current SFNNv13-era development, PlentyChess 7/8-era development, Reckless 0.9-era development, and Sirius 9 as a strong modern HCE reference

## Executive summary

Basilisk's HCE has reached a local optimum under its current representation, data distribution, and tuning objective. It has not reached feature completeness in the stronger sense of representing all important positional relationships. The cycle-6 self-play wash documented in [`PLAN.md:118`](../PLAN.md#L118) proves that another iteration of the same on-policy result-label tuning pipeline did not help. It does not prove that the evaluator contains sufficient information or that the existing feature activations have correct chess semantics.

The evaluation gap has three qualitatively different parts:

| Source of loss | Assessment |
|---|---|
| Correctness and activation defects | Several verified defects should be fixed before further tuning |
| Missing conditional HCE knowledge | Likely contains useful tens of Elo, especially in king safety and endgames |
| Representation and supervision ceiling | Likely accounts for most of the remaining distance to current top engines |

The most important verified defects are:

1. opposite-coloured-bishop scaling can amplify an evaluation by as much as 2x;
2. the enemy-rook-behind-passer term normally fails to activate because it is nested under a loop over friendly rooks;
3. double attacks by two pawns are omitted from `attacked2`, although later threat and king-safety code consumes it;
4. all winnability parameters are zero, and the claimed Phase-4.5 finite-difference Texel path does not exist for them; and
5. the tuner fits the complete evaluator while release builds can serve a truncated lazy evaluation.

Beyond these defects, Basilisk's principal HCE limitation is conditionality. Many features ask a broad question such as "is the queen advanced?", "is this a bishop on an outpost?", or "is this a central square free of enemy pawn control?" Stronger classical evaluation asks whether the feature is usable, safe, relevant to the material signature, supported by the pawn structure, and convertible. Modern NNUE goes further and learns these interactions directly.

The practical conclusion is:

- there is worthwhile HCE repair work left;
- the statement that the HCE is feature-complete should be retired;
- additional constant optimization alone is unlikely to recover more than a small fraction of the gap; and
- a competitive NNUE should be treated as the main path to the top cluster, but the planned plain `768 -> (256x2) -> 1` network should be a bring-up baseline rather than the final architecture.

## Evidence classifications

This report distinguishes implementation facts from strength estimates:

| Classification | Meaning |
|---|---|
| Verified defect | An unambiguous implementation or semantic error visible in the current source |
| Measured behavior | A result recorded by the project, but not automatically attributable to one mechanism |
| Architectural gap | A concrete difference from current leading evaluators |
| Strength hypothesis | A plausible Elo opportunity that still requires controlled match testing |

Elo ranges in this report are deliberately non-additive. Evaluation features interact with search, pruning, data generation, and one another. A 10-Elo candidate and a 20-Elo candidate can combine to zero, 10, 30, or more depending on redundancy and search effects.

## Scope and evaluation pipeline

The audit covers:

- the main evaluator in [`src/eval.cpp`](../src/eval.cpp);
- parameter definitions in [`src/EvalParams.h`](../src/EvalParams.h);
- evaluation use and correction history in [`src/search.cpp`](../src/search.cpp);
- Texel tracing and tuning in [`tools/texel/tuner.cpp`](../tools/texel/tuner.cpp);
- self-play extraction in [`tools/texel/extract.py`](../tools/texel/extract.py);
- Stockfish-distillation import in [`tools/texel/import_beast.py`](../tools/texel/import_beast.py);
- self-play adjudication in [`tools/datagen.ps1`](../tools/datagen.ps1);
- evaluator tests in [`tests/test_eval.cpp`](../tests/test_eval.cpp); and
- the planned NNUE direction in [`PLAN.md`](../PLAN.md) and [`GUIDE.md`](../GUIDE.md).

At a high level, the current HCE computes:

```text
material + PST
    + material imbalance
    + cached pawn structure
    + dynamic passers and simple piece bonuses
    -> optional lazy exit
    + attack maps and mobility
    + threats and king danger
    + shelter, passer path, and small positional terms
    + mate drive, tempo, and nominal winnability
    -> MG/EG interpolation
    -> endgame overrides/scaling
    -> rule-50 damping
    -> side-to-move score
```

This is a recognizable late-classical architecture. The problem is not absence of feature names. The problem is that many activations discard the relationships that determine whether the named concept is good or bad.

## Mid-2026 comparison baseline

The latest published CCRL 40/15 snapshot available during this audit lists Stockfish 18 at 3651 and PlentyChess 7 at 3644. Sirius 9, which uses a large tapered handcrafted evaluator, is roughly 200 Elo below Stockfish on the same broad rating scale. These are whole-engine ratings rather than evaluator ablations, but they demonstrate the scale of the modern HCE-to-NNUE separation. See the [CCRL 40/15 complete list](https://computerchess.org.uk/ccrl/4040/rating_list_all.html).

The open comparison sources are:

| Engine | Relevant evaluator design | Sources |
|---|---|---|
| Stockfish 18/current | SFNNv13: king-conditioned piece features, explicit threat features, PSQT output, material/output buckets | [Stockfish 18 release](https://stockfishchess.org/blog/2026/stockfish-18/), [official NNUE documentation](https://github.com/official-stockfish/nnue-pytorch/blob/master/docs/nnue.md), [current evaluation](https://github.com/official-stockfish/Stockfish/blob/master/src/evaluate.cpp) |
| PlentyChess | Incremental threat-input NNUE trained on 15+ billion self-generated positions with self-distillation | [repository](https://github.com/Yoshie2000/PlentyChess), [architecture snapshot](https://github.com/Yoshie2000/PlentyChess/blob/04e07a98ee6ac104c30e7374450c94b96d94ef4d/src/nnue.h), [threat inputs](https://github.com/Yoshie2000/PlentyChess/blob/04e07a98ee6ac104c30e7374450c94b96d94ef4d/src/threat-inputs.h) |
| Reckless | King-bucketed threat-input NNUE, material outputs, richer correction history | [NNUE implementation](https://github.com/codedeliveryservice/Reckless/blob/dd0e676007f2e53e1bc59054b24b6ca9003d9ca2/src/nnue.rs) |
| Sirius 9 | Rich tapered HCE with conditional threats, nonlinear king safety, complexity, scaling, and specialized endgames | [repository and feature list](https://github.com/mcthouacbb/Sirius), [classical evaluator](https://github.com/mcthouacbb/Sirius/blob/3bedea6e45cc7eac4f3d2969b13878e6d6a12959/Sirius/src/eval/eval.cpp), [endgames](https://github.com/mcthouacbb/Sirius/blob/3bedea6e45cc7eac4f3d2969b13878e6d6a12959/Sirius/src/eval/endgame.cpp) |
| Stockfish 11 | Mature pre-NNUE classical reference for space, passers, king safety, complexity, and scaling | [classical evaluator](https://github.com/official-stockfish/Stockfish/blob/sf_11/src/evaluate.cpp) |

Torch is closed-source, so no detailed claims are made about its internal evaluator.

## Priority overview

| Priority | Area | Basilisk limitation | Classification |
|---:|---|---|---|
| 1 | OCB scaling | Draw scaler increases `abs(eval)` above four total pawns | Verified defect |
| 2 | Rook/passers | Enemy rook behind a passer usually does not activate | Verified defect |
| 3 | Attack maps | Two-pawn double attacks are absent from `attacked2` | Verified defect |
| 4 | Winnability | All coefficients are zero and generic Texel cannot tune them | Verified defect/process gap |
| 5 | Train/serve equivalence | Lazy evaluation is disabled during tracing | Architectural/process gap |
| 6 | King safety | Degenerate fitted attacker weights and shallow shelter semantics | High-value architectural gap |
| 7 | Endgames | Sparse material-specific scaling and over-general mate drive | High-value architectural gap |
| 8 | Passers/pawns | Current-state, mostly additive features miss races and interactions | Architectural gap |
| 9 | Threats | Aggregated counts discard attacker/victim square relationships | Architectural gap |
| 10 | Phase model | One MG/EG scalar represents materially different position classes | Architectural gap |
| 11 | Training data | Self-referential outcome labels and reused holdout hide residual errors | Development-process gap |
| 12 | NNUE plan | Plain 768 input net is below modern feature capacity | Strategic architecture gap |

## 1. Opposite-coloured-bishop scaling can amplify the evaluation

### Basilisk behavior

The OCB rule at [`src/eval.cpp:376`](../src/eval.cpp#L376) computes:

```cpp
int scale = 32 + total_pawns * 4;
score = score * scale / 48;
```

The multiplier is therefore:

| Total pawns | Scale | Effective multiplier |
|---:|---:|---:|
| 0 | 32 | 0.667 |
| 2 | 40 | 0.833 |
| 4 | 48 | 1.000 |
| 6 | 56 | 1.167 |
| 8 | 64 | 1.333 |
| 16 | 96 | 2.000 |

Any opposite-bishop position with more than four total pawns receives a larger absolute evaluation than it had before draw scaling. This contradicts both the surrounding description and the normal purpose of an OCB scale factor.

The tuner duplicates the formula at [`tools/texel/tuner.cpp:354`](../tools/texel/tuner.cpp#L354). As a result, trace reconstruction remains internally consistent and the defect is difficult to notice from optimizer verification: the tuner simply learns parameters in the presence of the amplification.

### Current reference behavior

Stockfish 11 first identifies the side with the endgame advantage. It applies a very small factor to pure opposite-bishop endings and otherwise uses the winning side's pawn count while capping the result at the normal scale. It never turns draw scaling into an evaluation amplifier. See Stockfish 11's [`scale_factor()`](https://github.com/official-stockfish/Stockfish/blob/sf_11/src/evaluate.cpp#L3091-L3120).

### Recommended correction

1. Determine the strong side from the endgame score, not total material alone.
2. Base the general OCB factor on strong-side pawns.
3. Cap the factor at `SCALE_NORMAL`.
4. Retain a stronger draw factor for bishops-only OCB endings.
5. Apply the same implementation to the evaluator and tuning reconstruction through one shared helper if possible.

Add tests asserting:

- `abs(scaled) <= abs(unscaled)` for every OCB pawn count;
- adding pawns can relax draw scaling but never exceed normal;
- score sign is preserved; and
- same-coloured bishops do not activate the rule.

This is a high-confidence correctness fix. Its Elo effect may still be modest because it is restricted to one material family, but the affected family is common and strategically important.

## 2. Enemy rook behind a passed pawn usually does not activate

### Basilisk behavior

The complete rook/passer block begins at [`src/eval.cpp:1138`](../src/eval.cpp#L1138). It loops over friendly rooks, derives `f` from the friendly rook square, and finds friendly passers on that file. The enemy-rook penalty is then evaluated inside the same friendly-rook loop:

```text
for each friendly rook R on file f
    find our passers on f
    reward R if it is behind our passer
    for each enemy rook on f
        penalize it if it is behind our passer
```

This means the enemy-rook feature fires only when all of the following are true:

1. we have a passed pawn on file `f`;
2. the enemy has a rook behind it on `f`; and
3. we also have a rook on `f`.

Condition 3 is unrelated to the intended feature. If there is no friendly rook on the passer file, the enemy rook is never examined. If two friendly rooks are stacked on the file, the enemy-rook penalty may be counted twice.

### Recommended correction

Separate the concepts:

- loop over friendly rooks to reward a friendly rook behind a friendly passer;
- loop over enemy rooks or friendly passers independently to penalize an enemy rook behind the passer.

The implementation should also define whether blockers between the rook and pawn matter. A rook geometrically behind a passer but separated by another piece is not equivalent to a rook actively supporting or blockading it.

Add exact FEN tests for:

- own passer plus enemy rook behind, with no own rook;
- own and enemy rooks both behind the passer;
- a blocker between rook and passer;
- rook in front rather than behind; and
- mirrored black positions.

## 3. `attacked2` omits double attacks by two pawns

### Basilisk behavior

At [`src/eval.cpp:776`](../src/eval.cpp#L776), the union of all pawn attacks is inserted into the attack substrate as a single bitboard:

```cpp
Bitboard patk = pawn_atk[c];
attacked2[c] |= attacked[c] & patk;
attacked[c]  |= patk;
```

Because `attacked[c]` is initially empty for the colour, overlap between the left-pawn-attack set and right-pawn-attack set is lost. A square attacked by two different pawns appears only once in `patk` and is not inserted into `attacked2`.

The comment says that no current consumer depends on this property. That comment is stale. Later code uses `attacked2` for:

- strongly protected targets at [`src/eval.cpp:917`](../src/eval.cpp#L917);
- hanging targets at [`src/eval.cpp:949`](../src/eval.cpp#L949);
- weak king-ring squares at [`src/eval.cpp:1024`](../src/eval.cpp#L1024); and
- king-flank pressure at [`src/eval.cpp:1032`](../src/eval.cpp#L1032).

Consequently, a target defended by two pawns can be classified as less protected than it is, and a king square attacked by two pawns can be classified as only singly attacked.

### Recommended correction

Compute the two pawn attack directions independently:

```text
left  = attacks made by left-diagonal pawn moves
right = attacks made by right-diagonal pawn moves
attacked2 |= left & right
attacked2 |= attacked & (left | right)
attacked  |= left | right
```

Add an attack-map unit test with two pawns converging on one square, plus threat and king-ring tests that prove the corrected substrate changes its consumers as intended.

## 4. The winnability model is effectively absent

### Intended model

The block at [`src/eval.cpp:1427`](../src/eval.cpp#L1427) constructs an endgame complexity term from:

- king outflanking;
- pawns on both flanks;
- king infiltration;
- pure pawn-ending status;
- number of passed pawns;
- total pawn count; and
- a bias.

It then changes the endgame component without allowing the score to cross zero. This is a reasonable late-classical technique: the same nominal advantage is less convertible when all pawns are on one wing, no passer exists, or the defending king has access to a blockade.

### Actual state

All seven coefficients are zero at [`src/EvalParams.h:335`](../src/EvalParams.h#L335). The contribution is therefore exactly zero in every position.

The source comments say these parameters were tuned through a Phase-4.5 finite-difference path. The tuner does expose a `winnable` group at [`tools/texel/tuner.cpp:294`](../tools/texel/tuner.cpp#L294), but the generic optimizer at [`tools/texel/tuner.cpp:1002`](../tools/texel/tuner.cpp#L1002) only consumes linear trace coefficients. The winnability block is intentionally not traced because it is sign-dependent and nonlinear. Its generic gradient is therefore zero.

The only implemented re-evaluation/finite-difference optimizer begins at [`tools/texel/tuner.cpp:1115`](../tools/texel/tuner.cpp#L1115) and is specialized for king-safety knobs. It does not include winnability. Current external SPSA configuration may expose some of these parameters, but that does not make the historical Phase-4.5 Texel claim true, and the shipped values remain zero.

### Consequence

Basilisk has explicit knowledge for a small number of exact or near-exact material cases, but no general conversion estimate. This is particularly costly for:

- single-wing pawn endings;
- opposite-bishop endings;
- rook endings with no realistic entry path;
- fortress-like positions with nominal material advantage;
- queenless positions in which phase alone overstates winning chances; and
- advantages that become winning only after creating a passer on the second wing.

### Recommended correction

Do not simply assign plausible constants. First repair the optimization path:

1. create a true finite-difference or direct re-evaluation tuner for the seven parameters;
2. use an endgame-enriched dataset rather than the general phase mixture;
3. validate calibration separately for drawn, won, and uncertain tablebase-adjacent positions;
4. retain sign preservation; and
5. SPRT the complete model against zero, followed by ablations of individual inputs.

It may be preferable to use material-signature scale functions rather than force all endgame convertibility through one seven-variable scalar.

## 5. Lazy evaluation creates a train/serve mismatch

### Basilisk behavior

The lazy exit at [`src/eval.cpp:731`](../src/eval.cpp#L731) activates when the cheap tapered evaluation exceeds 700 cp in absolute value. Release builds then omit:

- mobility;
- king safety;
- threats and hanging pieces;
- shelter/storm;
- passer path and several small terms;
- space; and
- winnability.

The project reports the lazy exit as an accepted `+16.6` Elo speed/strength tradeoff. The mechanism is nevertheless disabled under `TEXEL_TRACE`, so the tuning dataset always sees the complete evaluator.

The optimizer therefore minimizes loss for:

```text
full HCE(position)
```

while release search may consume:

```text
cheap HCE(position), if abs(cheap HCE) > 700
full HCE(position), otherwise
```

This matters precisely in materially unbalanced positions where omitted king danger, fortresses, trapped pieces, or promotion races can overturn the apparent margin.

### Recommended experiments

1. Measure residual error for full and lazy evaluation separately on the same untouched teacher set.
2. Stratify by material imbalance and king-danger level.
3. Compile a trace mode that reproduces the release lazy decision and fit that served function.
4. Compare a constant threshold with a threshold conditioned on non-pawn material or evaluator disagreement.
5. Reconsider the mechanism after incremental NNUE; a richer incrementally updated evaluator may make this blunt truncation unnecessary.

The existing SPRT result is evidence that the current lazy path is net beneficial at its tested time control. It is not evidence that the omitted positions are evaluated accurately.

## 6. Endgame scaling and specialized knowledge are too sparse

### Existing strengths

The endgame block at [`src/eval.cpp:280`](../src/eval.cpp#L280) includes several useful cases:

- KNNK draw;
- exact KPK classification;
- KBNK corner drive;
- wrong-bishop rook-pawn draw against a bare king;
- no-pawn/minor-advantage scaling; and
- opposite-bishop scaling.

These are worthwhile and should be preserved while the general framework is repaired.

### Missing general structure

The scaler does not use a material hash or a broad set of material-signature-specific evaluators/scalers. Important families receive only the generic tapered score:

- rook and pawn versus rook;
- queen versus rook/minor fortress patterns;
- rook-pawn plus minor-piece conversion cases;
- bishop versus knight endings with fixed pawn colours;
- multi-pawn races outside exact KPK;
- wrong-rook-pawn cases with additional defensive material; and
- many pawnless material imbalances.

Stockfish's mature classical design used specialized material-table evaluation and scaling before falling back to general heuristics. Sirius similarly separates known-endgame evaluation and scaling from its general tapered terms.

### Generic mate-drive contamination

At [`src/eval.cpp:1403`](../src/eval.cpp#L1403), any phase-6-or-lower position with an approximate advantage above 200 cp receives a bonus for pushing the losing king to the edge and bringing the kings closer.

That geometry is appropriate in bare-king mating endings. It is not generally appropriate in:

- pawn races;
- rook endings;
- zugzwang-sensitive minor endings;
- positions where king opposition matters more than distance; or
- positions where driving the king toward the correct promotion corner helps the defender.

The term can contribute roughly a large fraction of a pawn and is frozen in the Texel residual. It should be restricted to material signatures where mate-driving geometry is valid, or removed in favour of specialized endgame functions.

### Rule-50 damping

Basilisk applies at [`src/eval.cpp:1467`](../src/eval.cpp#L1467):

```text
score *= (100 - halfmove_clock) / 100
```

Current Stockfish uses approximately:

```text
score *= 1 - rule50_count / 199
```

See [current Stockfish evaluation](https://github.com/official-stockfish/Stockfish/blob/master/src/evaluate.cpp#L515-L549).

The difference is substantial:

| Halfmoves since reset | Basilisk retained score | Stockfish retained score |
|---:|---:|---:|
| 25 | 75% | 87% |
| 50 | 50% | 75% |
| 75 | 25% | 62% |
| 99 | 1% | 50% |

This is not a proof that Stockfish's denominator is universally correct. It does show that Basilisk makes the static evaluator pessimistic much earlier. Because corrected static evaluation drives reverse futility, null-move conditions, improving, and qsearch stand pat, the damping affects search selectivity as well as score display. It deserves an isolated SPRT and calibration study.

## 7. King safety is under-modelled and poorly identified

### Fitted parameter state

The king-safety parameter block begins at [`src/EvalParams.h:282`](../src/EvalParams.h#L282). Several fitted values are warning signs:

- base attacker units are `{N=4, B=0, R=0, Q=0}`;
- open-file, ring-pressure, flank-attack, flank-defence, and pawnless-flank inputs are zero;
- the king-file storm coefficient is `-1`, which slightly rewards the side whose king faces the enemy pawn;
- the comment claims no-queen scaling preserves `2/3`, but the actual ratio is `2/5`; and
- a 25-entry safety table must absorb most of the nonlinear relationship after a lossy integer index.

Bishop, rook, and queen safe checks remain active, and weak-ring/blocker terms contribute. The evaluator is not literally blind to these pieces. The problem is that ordinary coordinated zone pressure from bishops, rooks, and queens has little direct base weight, leaving the model dependent on sparse safe-check and weak-ring events.

### Tuner limitations

The king-safety tuner at [`tools/texel/tuner.cpp:1210`](../tools/texel/tuner.cpp#L1210) performs axis-aligned integer coordinate descent:

1. change one knob by `+step` or `-step`;
2. accept it only if training MSE improves;
3. maintain a monotone safety table; and
4. shrink the step when no single-axis improvement exists.

This optimizer can become trapped when two changes are useful only together. For example, increasing a rook attacker weight can be harmful until nearby safety-table bins are reshaped. A single-axis method rejects the first half of the useful joint move.

The holdout is reported but does not select or restore king-safety coordinates. `--max-positions` also takes the first `N` positions rather than a representative sample. The resulting unusual zero and negative weights are consistent with correlated features and a local optimum rather than strong evidence that rook zone pressure or king-file storms are genuinely harmless.

### Shelter and storm semantics

The shelter code at [`src/eval.cpp:1077`](../src/eval.cpp#L1077) evaluates only the king's current files when the king is already on a flank. It does not compare:

- the current shelter;
- kingside castling shelter;
- queenside castling shelter; or
- whether castling rights and paths make those shelters relevant.

The storm model counts enemy pawns within one file and applies a linear rank coefficient. It does not distinguish:

- blocked and unblocked storms;
- pawn levers;
- supported storms;
- a pawn that can advance with tempo;
- a storm opposed by a friendly pawn; or
- whether the attacking side has a queen.

### Missing king-danger interactions

A stronger classical model should combine:

- number and type of attackers;
- safe and unsafe checking squares;
- contact-check potential;
- pinned or overloaded defenders;
- weak ring and escape squares;
- shelter and storm state;
- queen presence;
- open and semi-open files toward the king;
- flank attack versus flank defence; and
- a nonlinear danger function.

These variables are strongly interactive. A safe queen check matters much more when a rook controls the escape file and a pinned pawn cannot cover the ring. An additive feature list cannot express that relationship unless explicit cross-terms are introduced.

## 8. Passed-pawn and pawn-structure evaluation miss dynamic interactions

### Passed pawns

Basilisk includes rank bonuses, a free advance, safe advance, king proximity, block-square defence, and current promotion-path safety. The attack-map-driven portion starts at [`src/eval.cpp:1200`](../src/eval.cpp#L1200).

The main limitation is that it evaluates the current board's attacks on future path squares. As the pawn advances:

- the pawn itself vacates a square and may open a rook or bishop line;
- a blocker may move or be captured;
- an enemy rook may establish a rear blockade;
- the defending king's square-rule relation changes with side to move; and
- the pawn may require support rather than merely an unattacked path.

Important missing conditions include:

- blocker type and ownership;
- rook/queen behind the passer;
- supported, connected, and candidate passer relationships;
- safe push sequences rather than one static safe path;
- pawn race tempo and promotion with check;
- rook-pawn conversion context; and
- material-dependent values of a distant passer.

### Backward pawns and majorities

Backward-pawn detection is based mainly on an enemy pawn attack on the stop square and absence of a nearby supporting pawn in a coarse region. It does not fully model whether the pawn can advance through a lever, whether its support pawn is itself blocked, or whether the weakness can be attacked by a piece.

The majority term counts flank pawn numbers. A nominal majority may be immobile, permanently blocked, or unable to create a passer; a numerical minority can possess the only useful lever. These cases receive the same feature activation.

### Why top neural evaluators improve here

King-conditioned piece-square inputs allow a network to learn that the same passer has different value depending on both king locations. Explicit pawn-pair features, as used by some current engines, represent chains, levers, rams, and candidate passers without forcing independent pawn values to compose the relationship indirectly.

## 9. Threat evaluation aggregates away decisive context

The threats package at [`src/eval.cpp:907`](../src/eval.cpp#L907) is materially better than flat hanging-piece penalties. It distinguishes minor, rook, king, hanging, queen-protected, restricted, and pawn-push threats.

However, each term aggregates targets by victim type or count. It generally discards:

- exact attacker square;
- exact victim square;
- whether the attacker is pinned;
- whether the victim can move with tempo;
- the cost of executing the threat;
- discovered and x-ray attacks;
- overloaded defenders; and
- the relationship to king safety.

The `weak_queen_prot` term at [`src/eval.cpp:955`](../src/eval.cpp#L955) illustrates semantic drift. Its comment says "only defender is the enemy queen", but the implementation counts weak targets attacked by the queen. It does not prove that the queen is the only defender.

The `restricted` term counts a broad intersection of attack sets. It does not require that a restricted square is relevant to a particular piece's mobility. The same square can be counted even if no enemy piece could use it.

Stockfish 18's threat-input architecture addresses this representation problem directly. Its inputs encode attacker type/square to victim type/square relationships rather than asking a later scalar feature to summarize them. Stockfish's release notes explicitly credit threat inputs with more natural and accurate threat evaluation, and current SFNNv13 retains compressed `FullThreats + HalfKAv2_hm` inputs. See the [Stockfish 18 release](https://stockfishchess.org/blog/2026/stockfish-18/) and [SFNN architecture history](https://github.com/official-stockfish/nnue-pytorch/blob/master/docs/nnue.md#historical-stockfish-evaluation-network-architectures).

## 10. Several positional terms encode an over-broad or incorrect question

### Space

The space block at [`src/eval.cpp:1352`](../src/eval.cpp#L1352) defines space as central squares that:

- lie on files C-F and ranks 2-4/5-7;
- are not occupied by a friendly pawn; and
- are not attacked by an enemy pawn.

It does not require friendly control. It does not give special meaning to squares behind a friendly pawn chain. It does not exclude general enemy piece control from the base set. Its base and piece-count weights are zero; only the blocked-pawn interaction is active at `-2`, making the surviving semantics especially difficult to interpret.

Stockfish 11's classical space term used a material threshold, safe central squares, extra weight for squares behind friendly pawns and not controlled by the opponent, and a nonlinear piece-count multiplier. See [Stockfish 11 `space()`](https://github.com/official-stockfish/Stockfish/blob/sf_11/src/evaluate.cpp#L2969-L3024).

### Bad bishops

At [`src/eval.cpp:1260`](../src/eval.cpp#L1260), every friendly pawn on the bishop's colour contributes equally. This conflates:

- fixed blocked central pawns that genuinely restrict the bishop;
- mobile flank pawns that can leave their squares;
- pawns in front of versus behind the bishop; and
- pawns whose colour-complex control is strategically useful.

The result can penalize a bishop for a healthy pawn chain even when the bishop operates outside it.

### Queen infiltration

At [`src/eval.cpp:1305`](../src/eval.cpp#L1305), an advanced queen is "safe" if it is not attacked by an enemy pawn. It may still be attacked or trapped by a knight, bishop, rook, queen, or king. The feature can therefore reward an advanced hanging queen.

### Rook features

The rook-on-seventh bonus is mostly unconditional. Stronger formulations normally consider the enemy king, enemy pawns on the back ranks, and whether the rook has actual targets or checking access.

The rook/enemy-queen-file term at [`src/eval.cpp:1315`](../src/eval.cpp#L1315) rewards file alignment without fully establishing an open line or useful direction. A blocked rook and queen can activate the same term as a genuine x-ray.

### Trapped bishops

The trapped-bishop term at [`src/eval.cpp:1387`](../src/eval.cpp#L1387) runs only below half phase and requires zero pseudo-mobility. It misses classical opening and middlegame traps, while also partly duplicating mobility when it does activate.

### Outposts

Actual outpost bonuses are concentrated on knights. Bishops can receive related small-term logic but lack an equivalent well-conditioned outpost model. The reachable-knight-outpost count also values the number of reachable squares without asking whether the route is tactically feasible or whether the resulting knight can be exchanged profitably.

These are examples of feature misspecification rather than bad constants. Optimizing a single weight forces the tuner to average good and bad activations, often driving the coefficient toward zero or an unintuitive sign.

## 11. One MG/EG scalar is not enough phase specialization

Basilisk interpolates every tapered term using one phase value from 0 to 24. This assumes that positions with equal phase require approximately the same feature relationships.

They do not. For example, the following can share a similar scalar phase while requiring very different evaluation:

- queenless middlegame with four rooks;
- queen plus minor pieces with no rooks;
- rook-and-minor ending;
- opposite-bishop ending with queens;
- closed centre with undeveloped pieces; and
- open tactical position after mass pawn exchanges.

Modern NNUE architectures commonly use eight output/material buckets chosen by piece count. This is effectively a small mixture of experts: sparse endings and dense middlegames use different downstream weights even if the same input feature is active. Stockfish's official NNUE documentation describes both direct PSQT outputs and eight piece-count-selected layer stacks.

A classical analogue would use material-signature-specific scales, feature multipliers, or sub-evaluators. Adding more global MG/EG constants does not solve this ambiguity.

## 12. Correction history exists, but its consumption is less expressive

Basilisk should receive credit for already implementing multiple correction histories. At [`src/search.cpp:609`](../src/search.cpp#L609), it updates:

- pawn structure;
- minor-piece structure;
- white non-pawn structure;
- black non-pawn structure; and
- one previous piece-to-square continuation.

It correctly stores raw static evaluation in the TT and applies correction at probe time around [`src/search.cpp:1263`](../src/search.cpp#L1263).

The current value at [`src/search.cpp:624`](../src/search.cpp#L624) is nevertheless a simple equal average:

```cpp
return (pawn + minor + own + opp + cont) / 5;
```

Current Stockfish uses separately tuned weights and continuation-correction contexts from two and four plies back. It also uses the absolute correction magnitude as an uncertainty input to reverse-futility margins. See [current Stockfish correction history](https://github.com/official-stockfish/Stockfish/blob/master/src/search.cpp#L2357-L2392) and its use in [static evaluation and pruning](https://github.com/official-stockfish/Stockfish/blob/master/src/search.cpp#L3468-L3778).

The conceptual difference is:

```text
Basilisk: corrected eval = HCE + averaged residual

Top-engine pattern:
    corrected eval = learned evaluator prior + weighted contextual residual
    pruning confidence = function(eval, residual magnitude, histories, node context)
```

Recommended experiments include:

- fit weights for each correction source;
- add two- and four-ply continuation contexts;
- measure collision and support rates for each table;
- condition correction by rule-50 bucket only if data supports it;
- add `abs(correction)` to selected pruning margins; and
- gate updates to avoid learning from tactically unstable or unsuitable bounds.

This is an evaluation/search coupling improvement, not a replacement for evaluator capacity.

## 13. The tuning data can converge while preserving misconceptions

### Self-play result labels

The extraction loop at [`tools/texel/extract.py:71`](../tools/texel/extract.py#L71) assigns every selected position the final game result. It skips positions in check and skips a position when the move actually played is a capture or promotion.

The latter is not a true quiet-position test. A position can contain a winning tactical capture while the engine plays a quiet move; such a position enters the HCE dataset with no quiescence label. Conversely, a strategically useful position is excluded if the played move happens to be a harmless exchange.

Up to 12 or 24 positions from a game share one result. Sampling reduces correlation but does not remove it. A long game can therefore contribute many nearly related labels.

### Self-referential adjudication

At the time of this analysis, the self-play generator used draw adjudication
after a window of evaluations below 10 cp and resignation after repeated
600-cp evaluations. The current tool defaults to natural termination; the old
policy is now explicit opt-in only.

This produces a closed feedback loop:

```text
current evaluator
    -> move choices and search pruning
    -> draw/resign adjudication
    -> result labels
    -> next evaluator
```

The loop is valuable for on-policy optimization. It is weak at correcting positions that the current engine systematically misjudges, avoids, or adjudicates early. A cycle can wash because the policy/data fixed point has been reached, not because the evaluator is objectively complete.

### Holdout reuse

The generic tuner fits sigmoid scale `K` on the holdout at [`tools/texel/tuner.cpp:1022`](../tools/texel/tuner.cpp#L1022), then uses the same holdout to select and restore the best epoch at [`tools/texel/tuner.cpp:1082`](../tools/texel/tuner.cpp#L1082). This set is validation data, not an untouched test set.

The king-safety tuner reports holdout loss but accepts coordinates solely on training loss. It does not restore the best holdout state.

### Distillation import

The Stockfish/Beast import path reads files in order until `max_positions`, uses random position-level train/holdout assignment, and does not apply the self-play extractor's deduplication, by-game split, phase balancing, or tactical filter. Adjacent or trajectory-related positions can therefore leak between train and holdout.

### Objective limitations

The generic tuner minimizes WDL MSE against final results. It does not blend:

- deep teacher centipawn or WDL estimates;
- actual game results; and
- explicit uncertainty or search depth.

Modern NNUE training commonly blends search evaluation and result targets. Stockfish 18 reports an automated reproducible pipeline capable of using more than 100 billion Lc0-evaluated positions. PlentyChess reports more than 15 billion self-generated positions and partial self-distillation.

### Parameter identifiability

Joint material/PST tuning contains a simple degeneracy: adding a constant to every PST square for one piece and subtracting the same amount from that piece's material value leaves the evaluation nearly unchanged. Without centring constraints or strong priors, multiple parameter vectors represent the same function.

Correlated features create similar problems. Examples include:

- mobility versus trapped-piece terms;
- king-ring pressure versus safe checks;
- passed-rank versus path-safety bonuses;
- material versus imbalance; and
- pawn-island versus isolated/backward-pawn penalties.

Unintuitive fitted signs such as positive endgame pawn islands, negative king-file storm weight, and zero rook/queen king attacker units are signals to inspect identifiability and activation semantics rather than accept the values as chess conclusions.

## 14. Test and diagnostic coverage is insufficient for evaluator semantics

The current evaluator tests provide useful sanity coverage, including simple material and known-endgame cases. They do not systematically test feature meaning.

Missing high-value tests include:

- OCB scaling never amplifies;
- enemy rook behind a passer without a friendly rook;
- double pawn attacks enter `attacked2`;
- storm penalties have the correct sign;
- queen infiltration is rejected when attacked by a non-pawn;
- blocked versus unblocked rook/queen file alignment;
- score symmetry under colour-and-board mirroring;
- feature monotonicity where guaranteed;
- lazy/full evaluation disagreement cohorts; and
- winnability parameters actually affect and can be optimized on known positions.

Unit tests alone cannot establish evaluator quality. Add a fixed offline diagnostic corpus containing:

1. deep teacher cp/WDL labels;
2. tablebase WDL/DTZ where available;
3. stratification by phase and exact material signature;
4. loss-game positions before the tactical collapse;
5. fortress and conversion suites;
6. paired counterfactual positions; and
7. both static and qsearch-resolved labels.

For every evaluator candidate, report:

| Metric | Purpose |
|---|---|
| Global WDL/cp residual | Overall calibration |
| Residual by material signature | Detect endgame and phase failures |
| Residual by king-danger bucket | Detect attack underestimation |
| Residual by halfmove clock | Validate rule-50 treatment |
| Full versus lazy disagreement | Quantify train/serve truncation |
| HCE versus corrected HCE | Measure correction-history value |
| HCE versus qsearch/depth-N | Separate static and tactical error |
| Paired-feature deltas | Verify intended monotonic behavior |

This benchmark should be untouched by optimizer epoch selection and self-play adjudication.

## 15. Why current top evaluators do better beyond constants

### A more expressive function class

A tapered HCE is approximately:

```text
evaluation = sum(feature_i(position) * weight_i), blended by one phase scalar
```

An NNUE with king-conditioned and threat inputs can learn relationships such as:

```text
this bishop on this square
    with our king in this region
    and their king in that region
    attacking this victim
    through this pawn structure
    at this material density
```

Encoding that relationship in HCE would require many explicit cross-terms. The network learns a compressed nonlinear representation from data.

### King conditioning

Plain piece-square values are global. King-conditioned inputs give every piece/square a different first-layer representation depending on the relevant king bucket. This is crucial for:

- king attacks;
- shelter and storm;
- checking geometry;
- pinned defenders;
- passer races; and
- endgame king activity.

### Threat inputs

Stockfish's current feature set explicitly represents attacker and victim relationships. PlentyChess and Reckless use related designs. This avoids forcing aggregate threat counts to stand in for exact tactical geometry.

### Material-specialized outputs

Eight output/layer-stack buckets selected by piece count allow the evaluator to specialize its downstream function. A queenless middlegame and sparse rook ending do not have to share one MG/EG relationship merely because their scalar phase is similar.

### Direct PSQT output

A direct linear PSQT/material branch handles large material imbalances and basic piece placement without requiring the nonlinear layer stack to reproduce them. The nonlinear stack can spend capacity on positional interactions.

### Better supervision

Deep-search or Lc0 labels provide a dense target for every training position. Final result labels provide only a noisy game-level outcome. Blending both teaches positional value while preserving the actual win/draw/loss objective.

### Online residual learning

Correction history turns the evaluator into a prior that search can locally correct. Leading engines also treat correction magnitude as information about confidence, changing how aggressively they prune. This is more powerful than adding a post-hoc centipawn offset.

## 16. Assessment of the planned Basilisk NNUE

The current plan specifies a `768 -> (256x2) -> 1` perspective SCReLU network at [`PLAN.md:190`](../PLAN.md#L190). This is a good implementation baseline because:

- the input is sparse;
- accumulator updates are simple;
- inference can be made fast;
- the architecture proves the loader, trainer, quantization, embedding, and search interface; and
- it should already learn nonlinear global piece relationships better than HCE.

It should not be treated as the final competitive design. Plain 768 piece-square inputs provide no explicit:

- king buckets;
- attacker-to-victim threat inputs;
- material/output buckets;
- halfmove context; or
- direct PSQT output.

The network can approximate some of these relationships through hidden units and the two perspectives, but it spends capacity reconstructing context that top architectures supply directly.

Recommended staged A/B sequence:

| Stage | Architecture | Purpose |
|---:|---|---|
| A | 768 inputs, H256, SCReLU, one output | Bring-up and speed baseline |
| B | 8 or 16 king buckets, H256/H512 | Add explicit king-relative capacity |
| C | Eight piece-count output buckets and direct PSQT output | Add material specialization |
| D | Explicit threat inputs | Address tactical/positional threat residuals |
| E | Pawn-pair inputs if supported by residual analysis | Address structure and lever errors |

Do not choose the final architecture by static loss alone. Compare:

- nodes per second;
- accumulator refresh/update cost;
- static teacher loss;
- self-play SPRT at short and long time controls;
- tactical and fortress cohorts; and
- interaction with pruning after search retuning.

## 17. Recommended implementation and experiment roadmap

### Phase A: repair correctness before retuning

1. Fix OCB scaling and share the formula with the tuner.
2. Separate friendly and enemy rook-behind-passer loops.
3. Correct pawn contributions to `attacked2`.
4. Add targeted semantic tests for all three.
5. Verify colour/mirror symmetry on a large random corpus.

These changes should be tested individually and as a bundle. Their individual Elo is likely small and noisy; correctness and downstream feature quality justify them even before a clean Elo signal appears, provided no performance regression is material.

### Phase B: establish an external evaluator benchmark

1. Create a frozen teacher-labelled corpus independent of Basilisk adjudication.
2. Split by game or source trajectory, not by individual adjacent position.
3. Maintain train, validation, and untouched test sets.
4. Include endgame/tablebase and king-attack enrichment.
5. Record full HCE, lazy HCE, corrected HCE, qsearch, and depth-N outputs.
6. Publish residual tables by cohort for every evaluation experiment.

This phase is the most important process improvement. Without it, further self-play cycles can wash without explaining whether the evaluator is accurate or merely self-consistent.

### Phase C: classical HCE recovery work

Recommended order:

1. real winnability/material-signature scaling;
2. king shelter and danger rework;
3. passer/race and rook-behind-passer semantics;
4. correction-history weighting and uncertainty use;
5. phase/material specialization; and
6. cleanup or removal of misspecified zero/negative-value terms.

Every feature family should have:

- activation-count reporting;
- teacher residual before/after;
- a zero-weight ablation;
- an isolated SPRT where affordable; and
- a final bundled LTC test.

### Phase D: NNUE implementation

Bring up the 768-input network, but preserve the ability to change feature sets and output buckets without replacing the complete inference interface. The accumulator and file format should encode architecture/version metadata from the beginning.

Train with a blend of:

- deep teacher evaluation/WDL;
- actual game result;
- on-policy Basilisk positions;
- tactically quiet and qsearch-normalized positions; and
- endgame/material balancing.

Once the baseline is stable, A/B king buckets and output buckets before spending a large compute budget on the plain architecture.

### Phase E: retune search around the final evaluation scale

NNUE changes centipawn scale, residual distribution, evaluation cost, and pruning reliability. After the evaluator architecture stabilizes:

- retune correction-history weights;
- retune reverse futility, null move, futility, and ProbCut margins;
- re-evaluate lazy evaluation;
- re-evaluate time management under changed node economics; and
- run LTC and multi-thread validation.

## 18. Non-additive strength estimates

These ranges are planning priors, not forecasts:

| Work package | Plausible scale | Confidence |
|---|---:|---|
| OCB, rook-passer, and `attacked2` correctness bundle | 5-20 Elo | High confidence in defects; low confidence in Elo |
| King-safety and shelter rework | 10-35 Elo | Medium |
| Winnability, material scaling, and endgame cleanup | 5-25 Elo | Medium |
| Passer/pawn and threat semantic improvements | 10-30 Elo | Medium-low due overlap |
| Better correction-history consumption | 5-20 Elo | Medium |
| Better independent data/teacher objective | 10-30 Elo for HCE, larger enabling value | Medium |
| Competitive king-conditioned NNUE | 100-250+ Elo | High confidence in direction, low confidence in range |

The HCE packages overlap heavily and should not be summed. A reasonable working prior is that a disciplined HCE repair campaign may contain roughly 20-60 net Elo. Recovering the full couple-hundred-Elo gap through additional additive terms is unlikely.

The comparison with current engines supports this conclusion. Stockfish 18 reports up to 46 Elo over Stockfish 17 from a release combining threat-input evaluation, correction-history improvements, training workflow, search, and hardware work. PlentyChess obtains top-tier strength from an independently trained threat-input NNUE. Sirius demonstrates how much conditional logic a competitive HCE requires and still remains materially behind the top neural cluster in whole-engine lists.

## Conclusion

Basilisk's evaluator is sophisticated enough that obvious feature-name additions now give diminishing returns. The next gains require repairing semantics and changing representation, not another pass over the same constants.

The immediate engineering priorities are clear:

1. fix the three verified activation/scaling defects;
2. stop describing the zero winnability block as tuned;
3. build an untouched teacher-residual benchmark;
4. rework king safety and endgame conversion using that benchmark; and
5. treat the planned 768-input NNUE as a baseline on the way to a king-conditioned, material-specialized architecture.

The central distinction for future work should be:

```text
Did this change make Basilisk agree better with objective/deep chess evidence?

or

Did it merely make Basilisk more self-consistent with positions and labels
generated by its previous evaluator?
```

The existing cycle-6 result answers only the second question. Closing the remaining strength gap requires answering the first.
