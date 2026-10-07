# BAS-E16

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Differential-harness observations -->

**BAS-E16 — Beast positions as STARTS with self-play labels; adopting Manta's
design after wrongly rejecting it** (2026-08-26).

Manta's `docs/HCE_DATAGEN.md` states the contract plainly: *"uses Manta
self-play results rather than an external evaluation oracle. The Beast file
supplies starts only. Every retained row receives the White-perspective result
of its own game."* They sample 1M starts from the Beast position file with a
per-pawn-family cap and play each one out.

**That was reviewed during the Manta import and rejected — wrongly.** The
recorded reason was that their corpus is smaller, labelled by a weaker engine,
and that reusing their FENs "would still require replaying them, which is the
expensive half — there is no saving." Every clause is true and the conclusion
does not follow: the question was never whether to import their *data*, it was
whether to adopt their *method*. Compute cost is identical either way, because
you play the games regardless. What the method buys is start diversity and phase
coverage, which are free.

*Measured, 3,000 games per arm, adjudication none, 8,000 nodes, same binary:*

| phase | UHO opening book | **Beast starts** |
|---|---:|---:|
| opening | 7.015 | 2.060 |
| early_mid | 5.050 | 3.036 |
| middlegame | 4.887 | 4.025 |
| endgame | 5.105 | **5.335** |
| deep_endgame | 2.612 | **3.018** |
| **games needed for a 1M-row target** | **173,036** | **116,528** |

An opening book forces every game to traverse an opening before it can reach an
endgame, so the scarce classes are paid for at the price of ~60 plies each.
Beast starts span all phases, so endgames are entered directly. The binding
constraint moves off `deep_endgame` — the class we are starved of — and onto
`opening`, which we have in abundance and value least. A third fewer games for
the same corpus.

`tools/texel/data/beast_seed_2m.epd` already holds 2,000,000 sampled Beast
positions, so no sampling pass is needed; `sample_fens.py` and `audit_starts.py`
are present if a fresh seed is ever wanted.

*Conditional lesson.* Judge an imported idea on its **method**, not on whether
its artifacts are worth copying. "No saving" was the wrong criterion — the right
one was "does this produce better data for the same cost", and it does.

*What survives from the old corpus.* Nothing of its labels. The Beast **file**
remains the right start source; the Stockfish **targets** are the thing to
discard, per the BAS-E11 correction and BAS-X02.
