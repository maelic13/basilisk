# PST orientation defect (2026-10-23)

Research record for PLAN **B.10**. Found during the 1.10.2 originality audit;
investigated without games on the B.2.4 head (`97cd8cf`, whose
`src/eval_params.h` is byte-identical to 1.10.2's).

## Defect

`init_eval_tables` (`src/eval.cpp`) reads `pst_mg`/`pst_eg` with index 0 =
a1 for White and `sq ^ 56` for Black, as `src/eval_params.h` documents. The
tables were seeded from PeSTO (Ronald Friederich, Chess Programming Wiki),
whose published arrays list **rank 8 first**, so every table needs a rank flip
on import. Five got it and seven did not:

| Table | Matches flipped PeSTO | Matches unflipped PeSTO | Orientation |
|---|---:|---:|---|
| mg pawn / rook / king | 21/48, 53/64, 60/64 | 1, 0, 0 | correct |
| eg pawn / rook | 3/48, 60/64 | 0, 0 | correct |
| mg knight / bishop / queen | 0, 0, 0 | 54, 54, 54 of 64 | **mirrored** |
| eg knight / bishop / queen | 2, 2, 0 | 63, 63, 63 of 64 | **mirrored** |
| eg king | 0 | 56/64 | **mirrored** |

The mirroring is present in every release from v1.0.0 to v1.10.2 (checked per
tag). Every later PST fit was L2-anchored toward these seeds, so the
anchor preserved the error. 604 of 736 entries are still identical to PeSTO.

## Chess impact

Largest per-square changes from the fix (White squares, current → fixed):
- mg knight: f3 +110 → +17 and f6 +17 → +110 (advanced outposts are underpaid);
- mg queen: h1 +45 → −50;
- eg queen: e2 +58 → −16 and e7 −16 → +58 (the endgame queen is paid to stay home);
- mg bishop: c1 −77 → −14.

The change is identical for both colours, so symmetric positions cancel. On
52,632 positions (`armC_basilisk25k_holdout.csv`), the flip moves the static
eval by a mean of 31.7 cp:
- median 22.5 cp, p90 72 cp;
- 19.7% of positions move by 50 cp or more.

The mean signed change (White POV) is −7.74 cp in White losses, −0.50 in
draws and +4.41 in White wins: the fix moves the eval toward the result.

## Fit-loss screens (screens only; BAS-E23: holdout loss is not Elo)

`basilisk-texel --tune pst` on armC train (1M) / holdout (52,632). K is
re-fitted per arm (1.914 current, 1.855 flipped).

| Fit | Current | Flipped |
|---|---:|---:|
| none (initial holdout) | 0.0788077 | **0.0787334** |
| none, `holdout_phase911.csv` (184,211) | 0.105099 | **0.104950** |
| `--l2 1e-6`, best holdout | 0.0787535 | **0.0786001** |
| `--l2 1e-5`, best holdout | 0.0787712 | **0.0786849** |
| unanchored, best holdout | 0.0777814 | **0.0777684** |

**Directional test.** Project an unanchored fit's movement from the current
seeds onto each table's flip direction:
- all seven mirrored tables move toward their flip (projection +0.19 to +0.78,
  correlation +0.17 to +0.79);
- the five correct tables do not (projection −0.54 to +0.11).

The data picks out exactly the mirrored tables.

**The anchor.** At `--l2 1e-6` the PSTs move about 0.1 cp mean. With that
anchor a refit cannot repair a seed error.

## Interactions and risks

- Search margins (Cluster 1's fitted RFP, futility and razoring) were fitted
  on the current eval. The flip shifts eval shape and slightly lowers K, and
  C.10 re-fits search to the evaluation regardless.
- The eg knight, bishop and king tables feed the KBNK and KXK mate drives.
  The endgame cohort floors (KBN-K 23/24 at 600k nodes, inside its slack) are
  the fragile check.
- Outposts, `reachable_outpost`, mobility and `queen_infiltration` were fitted
  around mirrored tables. They may partly compensate, which is the main reason
  the gate could read neutral.

## Decision and routing (maintainer, 2026-10-23)

- **B.10 flips the seven tables and nothing else**, then gates the flip
  alone with `sprt-repair.toml`. Phase B does no PST refit.
- **No refit was chosen instead of the flip.** Rarog repaired the same
  defect with a re-fit, which for 736 linear coordinates means a Texel fit.
  For Basilisk:
  - the flip gives a clean, attributable gate;
  - a proper refit needs Phase C's fit contract (a corpus from the B.9
    head, K frozen once, splits by game);
  - C.8 refits the whole surface again in any case.
- **Phase C fits onward from the flipped tables.** No neutral start is
  required, and PeSTO-derived values may stay (C.2.5).
- **Follow-ups in PLAN:**
  - C.0: the refit protocol must let a fit correct a seed error despite its
    anchor;
  - C.2.1: the orientation test is a precondition of every fit;
  - C.7: read outposts, `reachable_outpost`, mobility and
    `queen_infiltration` for compensation;
  - C.10: refit Cluster 1's eval-valued margins.

## Recipe and artifacts

Artifacts are in `tools/results/pst-orientation-20261023/` (ignored):
- scripts: `pstorient.py`, `pstimpact.py`, `fitdir.py`, `doflip.py`, plus
  `pesto.txt`, the PeSTO tables as published (rank 8 first);
- fit logs and dumps in `fit/`;
- the candidate header `eval_params.flipped.h`, sha256
  `a535b968d609df5dc6d34ed98f97e58c397c4d5d56b6e275898cd2c8e9bae1ff`. It is
  the source header with the seven tables rank-flipped and nothing else
  (`doflip.py`), CRLF preserved.

Tuner build: `cmake --preset release-pext -B build-texel -DTEXEL=ON`, target
`basilisk-texel`.
