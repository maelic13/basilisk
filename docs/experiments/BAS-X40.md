# BAS-X40

| Field | Value |
|---|---|
| ID | BAS-X40 |
| Triumviratus evidence | **NNUE architecture, inference and training** (`analysis/triumviratus_review_2026-11-03.md` §4.8–§4.10, TE-1, TE-2, TN-1 to TN-9, TD-1 to TD-8; snapshot `docs/reference/triumviratus-networks/` at `5f3dc60`). Its networks are its own weights from random initialisation on public Leela and Stockfish data re-labelled with BT4. Input-layer experts by material at −7.5% NPS (L1 2048 costs −20%) gave about +15 of 8.0's +27.3; per-side experts by the opponent's non-pawn material with int8 king-relative weights and a cached pawn-king subnetwork read +10.3 ± 4.3; a passed-pawn input block +6.96 ± 6.56. A λ cycle climbing back to 1.0 made a network imitate its labels: the λ=0 holdout loss stopped where Elo flattened, and a fixed λ restored the gain. A policy network inside alpha-beta lost in four forms. A residual regression of deep against shallow scores on thirty concepts found what the network underestimated; every fix added outside the network then lost |
| Basilisk implication | Priors for F.0–F.8 under the own-data rule: the architecture, inference and training method transfer, the data recipe does not. The residual regression is a donor-free instrument for C.0 on an HCE, where a signed residual points at a term's value |
| PLAN coverage | C.0, F.0, F.3–F.8, G.2 |
| Disposition | import |
