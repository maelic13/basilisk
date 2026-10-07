# BAS-D21

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Search programme investigation (B.0) -->

**BAS-D21 — matched per-family ablation screens: the ×4 is the move-loop
pruning family; razoring and LMR cost tactics at a fixed budget**
(observation; predictions S1–S6 frozen in
`tools/results/b0-20261006/frozen_predictions_screens.md` before exposure).
Each `AblationMask` bit alone on both engines, suite_v2 depths 4–12 and WAC
at 100,000 nodes. Per-position median node factor at depth 12 with the
family off (Basilisk / oracle): razoring 1.00 / 1.00; RFP 1.34 / 1.28; NMP
1.22 / 1.08; ProbCut 1.00 / 1.01; IIR (oracle IID) 1.18 / 1.00; **shallow
move-loop pruning 2.20 / 4.55**; extensions 0.58 / 0.52; LMR 4.54 / 3.41.
With that family off on both sides the trees are **0.98× at depth 4 and
1.64× at depth 12** (median 1.23) against 3.49× and 3.97× with everything on.
WAC at 100,000 nodes with the family off (Basilisk / oracle, baseline 204 /
241): **razoring 255 / 236**, RFP 197 / 243, NMP 202 / 233, ProbCut 217 /
247, IIR 202 / 238, shallow pruning 179 / 245, extensions 206 / 248, LMR 219
/ 236. Single knobs on the Tune build: `QsearchCheckCap=6` 214 (24 gained,
14 lost, 18 of the 50 oracle-only flipped, +32 positions solved first by
depth 3, +33% nodes to depth 12); `RazorCoeff=500` 228 (32 of razoring-off's
59 recovered, +5% nodes), `=400` 223. Calibration: S1 held except ProbCut
(1.21 against ≤ 1.15) and IIR's aggregate (two positions); S2 and S3 held
(the oracle's RFP 1.41 against ≥ 1.5 was the one miss); S4 missed in the
instructive direction (razoring +51, LMR +15, ProbCut +13 against a
predicted band of ±8); S5 held on three of four counts (+10 against ≥ 11);
S6 missed on magnitude (32 recovered against ≥ 35). *Lesson.* The oracle
prunes on better information, not harder: its move-loop family is tactically
free and carries 4.5× of selectivity; Basilisk's razoring at depth 2–3 and
its reductions remove tactically live lines, and its extension family costs
1.9× nodes for no fixed-node tactical return, the same as the oracle's. H2
(the check policy as the differential) is weakened; H1 is specific. BAS-D02's
"far too conservative" reading of LMR is withdrawn. Disposition: observation;
fixed-node screens explain and never accept (PLAN rule 8).
