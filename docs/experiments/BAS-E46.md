# BAS-E46

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Cluster 5.8 closing summary -->

**BAS-E46 - 8,000-node datagen mislabels a fifth of won endings, concentrated
in the Group B families** (2026-09-03). 20,000 games per corpus, stride 6 over
125,000, Syzygy six-man, cursed wins excluded.

| corpus | nodes | clean wins reached | converted | drawn | lost | mislabel rate |
|---|---:|---:|---:|---:|---:|---:|
| armA | 8,000 | 8,538 | 6,850 | 1,684 | 4 | **19.77%** |
| armC | 25,000 | 8,190 | 7,072 | 1,116 | 2 | **13.65%** |

About 43% of games reach a tablebase-adjudicable clean win, so at 8,000 nodes
roughly 8.5% of ALL games carry a final result that contradicts tablebase
truth, and the error is one-directional: won endings become draws. That teaches
the evaluator to undervalue exactly the advantages that win endgames. The
converse error is far smaller -- 296 of 3,605 objectively drawn positions were
decided, 8.2%.

*It is concentrated where the roadmap goes next.* Mislabel rate by family,
8,000 against 25,000 nodes: KRPP-KR 26.2% -> 13.4%, KRP-KRP 30.7% -> 14.0%,
KPP-KPP 25.2% -> 8.7%, KBPP-KP 17.9% -> 13.9%. Those are Plan 6.8's Group B
rook- and bishop-pawn families almost exactly. KBN-K does not appear at all,
consistent with BAS-E43.

*Correcting an earlier recommendation.* The judgement that datagen should stay
fast because WDL labels self-average was too confident. A one-directional 20%
error in won endings does not average out. But raising the budget is the weaker
of the two available fixes: 3.1x the compute buys only a 31% relative
reduction, from 19.77% to 13.65%, and even 25,000 nodes leaves one ending in
seven mislabelled. Adjudicating a game by tablebase once it reaches six men
would remove the bias outright at the cost of one probe per game.

*That is not authorized here.* Phase 8 keeps post-hoc tablebase relabeling and
datagen-v3 game adjudication as deliberately distinct arms that must not be
conflated, and 6.0.f declined to license relabeling without its own
halfmove-clock and row-domain analysis. This entry quantifies the prize for
those registered arms; it does not spend it.
