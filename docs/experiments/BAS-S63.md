# BAS-S63

**BAS-S63 — B.7.1's repair gate: a return to the root counts as a repetition only when it recurs, `[-5,5]`**

- **Registration.** Created 2026-11-14, before any game; maintainer-run games;
  correctness repair of PLAN B.7.1's finding F1
  (`analysis/b71_board_contract_2026-11-14.md` §3), the maintainer's decision to
  repair it now rather than in B.9.
- **Defect.** `Board::is_repetition` scored a single return to the root position
  as a draw (`distance <= search_ply`), while `upcoming_repetition`, B.3's cuckoo
  test in Stockfish's form, does not raise alpha for the same cycle (`search_ply >
  i`). The search could read a draw after a move that the pre-emptive bound one ply
  above had not seen. Stockfish 19 (`edb0d9db`, `Position::is_repetition`) counts
  one occurrence strictly after the root, or two at or before it.
- **Candidate.** `8bfbbff`: `distance < search_ply`, the donor's rule, which agrees
  with the cuckoo test (`test_board`, "upcoming repetition agrees with the child's
  repetition", fails on the head at the root cycle).
  `tools/test_engines/basilisk-b71-fix-pext-pgo.exe`, SHA-256
  `2B5C8B3057A9689A3B662530B1B1E770EAF115225F7462A2825A6103DB61515F`, `bench 13`
  **4,247,965**.
- **Baseline.** `b9cdba3`, the head before it (built in a temporary worktree):
  `tools/test_engines/basilisk-b71-base-pext-pgo.exe`, SHA-256
  `9632D49BE3C278E61CEFF1A4D9B9BA2BC54D0758A5829FC72F9069EA843EEB18`, `bench 13`
  **4,278,307**, the accepted fingerprint.
  Both final-PGO PEXT release builds by `tools/build_test.ps1 -Flavor Release`,
  Clang 23.1.5, manifests clean.
- **Research question.** Does the consistent rule cost Elo? A repair kept unless it
  harms.
- **Mechanism.** Under the old rule a line that returned to the root was a draw at
  once, although the game would have seen the position only twice and the opponent
  could deviate. Both sides could steer into it: a side ahead could undervalue a
  winning line that passes through the root position, and a side behind could
  claim a draw it does not have. The new rule needs the second occurrence, as the
  cuckoo bound already assumed.
- **Competing explanations.** Basilisk's old form could be the better heuristic at
  this depth and time control; the change also moves the tree everywhere a cycle
  to the root is reachable, so a small difference may be tree-shape noise.

#### EXPOSURE (measured before registration)

`tools/diag/decision_diff.py` over 3,000 positions of BAS-S62's games (every tenth
ply, with the game history before each), both arms at 50,000 nodes from a fresh
game. The self pair read 0 differences in 200 positions, so the read is
deterministic.

| Positions | Differ |
|---|---|
| all, 3,000 | 362 (12.1%) |
| halfmove clock ≥ 4 (a repetition reachable), 947 | 164 (17.3%) |
| at most ten men, 986 | 176 (17.8%) |

The exposure is far above 2%, so the cap is the full one, as BAS-S40's.

#### CONDITIONS

- Colosseum SPRT, `sprt-repair.toml`: normalized `[-5,5]`, α = β = 0.05.
- `3+0.03`, 1T, Hash 64, paired reversed UHO openings, no adjudication,
  tablebases off; seed 71010. Arm A the candidate, arm B the baseline.
- **Cap: 4,000 pairs.**
- Launcher `python tools/run_b71_gate.py`; results in `tools/results/b71-gate/`.
- **Dry run accepted 2026-11-14**
  (`tools/results/colosseum_sprt_b71-gate_20261114_141928.manifest.txt`).

#### QUALIFICATION (before the gate)

- On `8bfbbff`, release CTest 31/31, the strength floors (WAC, endgame conversion)
  included.
- The proven-mate fixture of `test_b5_root` moved with the tree in its own commit
  (`d75bc21`): the head proves that mate in 8 from depth 17, the repair in 9 from
  depth 19 and in 8 from depth 24; the test still requires every line after the
  proof to keep the mate.

#### PRE-REGISTERED PREDICTION — freeze before exposure

| # | Prediction | Probability |
|---|---|---|
| G1 | the cap without a decision | 50% |
| G2 | H1 | 35% |
| G3 | H0 | 15% |
| G4 | the estimate at the stop is **+1 Elo**, 80% interval [−4, +6] | low |

- **Verdict rule:**
  - H1: the repair stays; the fingerprint record moves to 4,247,965 (GUIDE,
    `AGENTS.md`, DESIGN) and `CHANGELOG.md` gains the entry with this result;
  - the cap without a decision: the repair stays as a correctness fix with no
    measured harm, as BAS-S35, BAS-S37 and BAS-S38; the maintainer confirms;
  - H0: the engine commit `83648bb` and its fixture commit `d75bc21` are reverted,
    and F1 returns to research: carry Basilisk's root rule into the cuckoo test
    instead, the other consistent form.
- No arm, bound, cap, book or adjudication changes after a game is observed.

#### RESULT — append after exposure
