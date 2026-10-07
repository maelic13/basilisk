# BAS-X29

<!-- part 1 of 1: from docs/EXPERIMENTS.md, 8. Cross-engine evidence imported from Rarog -->

| Field | Value |
|---|---|
| ID | BAS-X29 |
| Rarog evidence | Rarog's 2026-09-09 review of the shared SEE lineage: Basilisk's `see_ge` has no king rule (the king recaptures regardless of remaining attackers), pins are frozen at the exchange's starting occupancy, and pawn recaptures on the last rank are scored as pawns. Rarog's exact-legality kernel costs about 8% of the SEE column and its cluster measured +12.12 +/- 10.17 Elo; the cheap Stockfish king rule was never tested in either engine on its own. |
| Possible Basilisk implication | Phase 15 fixes the king rule first (15.0.a), decides created pins and promotions on reachability and cost (15.0.c), and gates the release line against 1.9.3 (BAS-E55). The rule itself is two lines; the fixtures are the deliverable. |
| Existing PLAN coverage | 15.0.a, 15.0.c, 15.1.a |
