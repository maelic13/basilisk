# BAS-P31

**BAS-P31 — the board's second round: six candidates on the replay and the engine screen (B.7.7)**

- **Registration.** 2026-11-14, B.7.7. The prototypes and a prediction per arm were
  frozen in `analysis/b77_second_round_2026-11-14.md` §2–§3 (`cfeec37`) before any
  read. Behaviour-neutral: every arm `bench 13` 4,247,965, `board_replay --verify` on
  58,598,640 records, `test_board_v2`, `test_search_path`, the twelve microbenchmark
  checksums.
- **Question.** Does any of PLAN B.7.7's candidates make B.7.6's board faster in the
  engine?
- **Arms.** Plain release builds of one scratch tree at `edc6584`, one macro per arm
  (`tools/results/b77/prototypes.patch`, SHA-256 291762B1F4C2C5D4…): undo as one block,
  the byte mailbox, both, keys by table, SEE's next attacker by index, the hinted make
  with the search wired, one enemy attack map per node.
- **Rule** (PLAN B.7.7, frozen): kept at +0.3% or more on the engine screen with at
  least 13 of 16 rounds faster and no replay class slower by more than twice the null
  spread.

#### RESULT

Idle host, CPU 2 (`tools/results/b77/replay`, `tools/results/b77/screen`). Engine
screen, 16 rounds, against the head built from the same tree; null copy −0.30%
(−0.86 to +0.76):

| Arm | NPS | Rounds faster | Verdict |
|---|---:|---:|---|
| Undo as one block | −2.52% | 0/16 | refused |
| Byte mailbox | −0.57% | 1/16 | refused |
| Both | −7.27% | 0/16 | refused |
| Keys by table | +0.22% | 10/16 | reported, not kept |
| Next attacker by index | −0.76% | 1/16 | refused |
| Hinted make | −0.49% | 0/16 | refused |
| Enemy attack map | −1.32% | 0/16 | refused |

- **Disposition.** NO_CHANGE: B.7.8 closes NO_CHANGE.
- **Lesson.** Each candidate was priced by the branches it removes and costs more in
  what it adds: bytes per make, byte loads, a whole kinds mask, a second make, a map
  built where its consumers would have stopped early.
- **Retry triggers.** In the packet's §5, one per candidate.
- **Artifacts.** `tools/results/b77/` (patch, binaries, replay, screen), the profile
  `tools/results/b77-profile/`.
