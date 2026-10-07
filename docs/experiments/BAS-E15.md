# BAS-E15

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Differential-harness observations -->

**BAS-E15 — rounds above book size buy nothing; deterministic self-play repeats
verbatim** (2026-08-26, corpus-design failure, caught before the fit).

The first 5.9.11 corpus requested 180,000 games: 80,000 and 60,000 rounds
against `SuperGM_4mvs.pgn`, plus 40,000 against `UHO_Lichess_4852_v1.epd`.
Measured over the first 40,000 games: **2,668 distinct games — exactly the book
size — each repeated 15 times, 93.3% duplicates.**

Fixed-node self-play with the same binary on both sides is deterministic, so
replaying an opening reproduces the game bit-for-bit. `datagen.ps1`'s own
`-Rounds` documentation says as much ("one game per opening"), and the round
counts were still chosen without checking how many openings the book holds.
Cost: ~2.5 hours of generation of which only the 40,000-game UHO slice carried
independent information.

*Rates once duplication is removed* (3,000 UHO games per arm; dedup falls to
**0.2%**, against 59% on the duplicated corpus):

| phase | UHO standard | UHO **none** |
|---|---:|---:|
| opening | 6.971 | 7.015 |
| early_mid | 4.691 | 5.050 |
| middlegame | 4.348 | 4.887 |
| endgame | 3.460 | **5.105** |
| deep_endgame | 1.387 | **2.612** |

Adjudication-none **doubles** deep-endgame yield and lifts endgame 47% at
unchanged opening/midgame rates, independently confirming BAS-E14 in the units
that matter — positions per game rather than games reaching a class.

*Conditional lessons.*

1. **Never request more rounds than the book has openings** unless something
   else breaks determinism (varied node counts, differing binaries, randomised
   play). Check book size first; `datagen.ps1` prints `book_openings` to the
   manifest, and the preflight would have shown it.
2. **A raw-vs-unique gap is the detector.** 137,815 raw against 56,985 unique
   was visible in the very first preflight and is what exposed this. Treat a
   large gap as a corpus defect until explained, not as normal dedup.
3. **Adjudication-none should be the default for datagen, not a special slice.**
   It yields strictly more of the scarce classes at identical rates for the
   abundant ones, and it removes adjudication's label error, for ~19% more time.

*Revised design.* One slice: **UHO, adjudication none**, 250,000 rounds. UHO
carries 2,632,036 openings, so duplication is structurally impossible at any
practical round count, and it is the book our SPRTs use — so the eval is fitted
on the distribution it is judged on. The separate "general/sharp" split is
dropped: `SuperGM_4mvs` cannot supply more than 2,668 distinct games and cannot
be the bulk of any corpus. Recorded bias to watch: UHO is unbalanced by
construction, so the corpus skews toward decisive positions.
