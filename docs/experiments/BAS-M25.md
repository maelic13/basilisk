# BAS-M25

**BAS-M25 — C.3.4: the rebuilt evaluation at seed values against the head**

- Date / owner / category: registered 2026-11-25, before the port
  exists and before any game; maintainer-run, agent-read; measurement.
- Arms: clean final PGO builds of one commit after C.3.3's acceptance:
  the new arm (the evaluation rebuilt in classical Stockfish's form, at
  the reference's values converted by the packet's §7) against the off
  arm, which is the head (`bench 13` 3,359,329).
- Research question: does the donor's evaluation carry its lead into
  Basilisk's own search and endgame layer? In `9587eeeb`'s search it
  leads the head's by +146.2 (BAS-O14); this is the first read of it in
  Basilisk's.
- Registration, mechanism and the rule:
  `analysis/c31_rebuild_2026-11-25.md` §3 and §11.
- Registered design and stop rule: no gate; one fixed match of 3,000
  games, never extended or replaced.
- Full conditions: Colosseum CLI, two-engine match, 1,500 cycles, UHO
  `UHO_Lichess_4852_v1.epd` in random order, seed `210004`, `3+0.03`,
  margin 20 ms, 1T, Hash 64, concurrency 14, no adjudication, no
  tablebases, zero permitted faults. The launcher and the builds are
  C.3.4's; this entry is completed with their hashes before the run.
- Beside it, deciding nothing alone: pooled NPS of the two arms and the
  fixed-depth tree read, by PROCESS.

#### PRE-REGISTERED PREDICTION — frozen 2026-11-25

- The new arm minus the head: +85 Elo, 80% band +40 to +130.
- Return rule: below +30 the leaf returns to research before any fit.
  The premise would be false.
- The seed arm never ships: its values are the reference's.

#### RESULT — append after exposure

