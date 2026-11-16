# BAS-S66

**BAS-S66 — B.9.5's repair gate: the lifecycle, protocol and verdict repairs, `[-5,5]`**

- **Registration.** Created 2026-11-17, before any game; maintainer-run games.
  A repair kept unless it harms (`agents/measurement.md`, *Gating*).
- **Candidate.** `39982c0`, B.9.5's head: B.9.1's D1 to D14 and the two
  decisions (bare `go` and `go mate N` take Stockfish's meaning), B.9.3's V1
  and V2, and the thread stacks (F2). Packet:
  `analysis/b95_repairs_2026-11-17.md`.
  `tools/test_engines/basilisk-b95-repair-pext-pgo.exe`, SHA-256
  `6B5C2981F031193CA80B3E4D92BA511D3E6E413DF4F7218EB012E1163E3709DF`, `bench 13`
  **4,777,412**.
- **Baseline.** `9a4a94b`, the head before the repairs (B.9.4's close):
  `tools/test_engines/basilisk-b95-base-pext-pgo.exe`, SHA-256
  `6512294E4CAA9E5465BDE4B85D63B9DE19F08A8B83E78E78EA16E625A954C4C8`, `bench 13`
  **4,688,537**, the accepted fingerprint.
  Both final-PGO PEXT release builds by `tools/build_test.ps1 -Flavor Release`,
  Clang 23.1.5, manifests clean.
- **Research question.** Do the repairs that can change a move cost Elo?

#### EXPOSURE

Measured before the gate, by repair:

- **V2** (the pawnless 0 no longer a draw verdict where the other side has
  pawns): the search's correction now applies at those positions, and
  `bench 13` moves 1.9%. Of BAS-S64's 8,000 gate games, 2,438 (30.5%) reach
  such material at some ply, an upper bound: the change acts only where the
  score favours the side without pawns. Broad exposure, so the full cap.
- **D2** (no stop before the first iteration completes) and **D3** (a named
  clock is a clock whatever it reads): nil in clock games. D2 acts only when
  the hard limit falls inside the first iteration (well under a millisecond),
  D3 only on a clock of 0 or less; a harness game never searches on an empty
  clock.
- **D13** (tablebase wins ranked by DTZ once the game has repeated): nil here,
  since the gate runs without tablebases. With tables, BAS-S39 saw one such
  game in 2,286. It is read deterministically instead (packet §4).
- **D1** (depth limit at Threads > 1), **D8** (ponderhit) and the protocol
  repairs: nil at 1T with no ponder and no `go depth`.

#### CONDITIONS

- Colosseum SPRT, `sprt-repair.toml`: normalized `[-5,5]`, α = β = 0.05.
- `3+0.03`, 1T, Hash 64, paired reversed UHO openings, no adjudication,
  tablebases off; seed 95010. Arm A the candidate, arm B the baseline.
- **Cap: 4,000 pairs**, the full cap for V2's exposure.
- Launcher `python tools/run_b95_gate.py`; results in `tools/results/b95-gate/`.
- **Dry run accepted 2026-11-16**
  (`tools/results/colosseum_sprt_b95-gate_20261116_235550.manifest.txt`).

#### QUALIFICATION (before the gate)

- Release CTest 59 of 59 and the debug sanitizer build 56 of 56 on `39982c0`;
  ThreadSanitizer over the threading, ponder and tablebase tests (Ubuntu WSL,
  Clang 20) 9 of 9; a release PGO build with every untrained file an error.
- Every B.9.1 and B.9.3 known-defect test passes plainly; none remains.

#### VERDICT RULE (frozen)

- H1, or the cap with LLR above the lower bound: the repairs stay; the
  fingerprint record moves to 4,777,412 (GUIDE, DESIGN, AGENTS), and B.9.5
  closes.
- H0: V2, the one repair with exposure here, returns to research. Its
  verdict half is a correctness fix the tables confirm, so the return asks
  what the correction does at those positions (C.7's correction question,
  E7), not whether to keep the false verdict.

#### RESULT

Pending: the maintainer's run.
