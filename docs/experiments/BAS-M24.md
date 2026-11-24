# BAS-M24

**BAS-M24 — C.2.10: the frontier pair's baseline, the refitted head against Stockfish 16 HCE**

- Date / owner / category: registered 2026-11-24, before any game;
  maintainer-run, agent-read; measurement.
- Arms: a fresh final PGO PEXT build of the head (`bench 13` 3,359,329)
  against `D:\chess\engines\stockfish-16-x64-bmi2-classical-pgo.exe`
  (`041396A4…`, BAS-M18's) with `Use NNUE=false`, otherwise at its
  defaults.
- Research question: how far is the head from the pre-NNUE target, in
  the form every later cluster repeats (PLAN, the cluster shape's
  step 7)?
- Registration and prediction:
  `analysis/c210_family_order_2026-11-24.md` §2.2 and §5.
- Registered design and stop rule: no gate; one fixed match of 3,000
  games, never extended or replaced.
- Full conditions: `python tools/run_c210_reads.py --read frontier`;
  Colosseum CLI, a match with an external second arm, UHO
  `UHO_Lichess_4852_v1.epd` in random order, seed `210002`, `3+0.03`,
  margin 20 ms, 1T, Hash 64, concurrency 14, no adjudication, no
  tablebases, the wrapper's 1% fault allowance for the foreign engine.

#### PRE-REGISTERED PREDICTION — frozen 2026-11-24

- The head minus Stockfish 16 HCE: −140 Elo, 80% band −170 to −110.
  BAS-M18 read −193.2 [−217.7, −170.3] on the 1.11.0 head, over 600
  games in a field.
- Falsifier: worse than −185. The refit's +85 in self-play would then be
  worth nothing against the target.

#### RESULT — append after exposure
