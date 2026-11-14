# B.7.7 — the board's second round of candidates (2026-11-14)

Bounded research on B.7.6's head (`0c978c2`, the board of BAS-P30): each
candidate of PLAN B.7.7 prototyped behind a macro in a scratch worktree,
proved exact, read on B.7.5's two instruments, and kept or refused by the
rule PLAN froze. Nothing is merged here. §5 is the frozen handoff for B.7.8.

## 1. The profile that gates the list

`tools/results/b77-profile/` (Profile build of `78e4581`, source as
`0c978c2`; `bench 13` × 5, 8,962 samples, host idle), attributed by
`tools/diag/profile_entry_points.py`, which reproduces B.7.2's table from
B.7.2's samples. Shares of all samples:

| Entry point | Share | Inside it |
|---|---:|---|
| `see_ge` | 5.51% | own 2.82; `attackers_to` 1.94 |
| `gives_check` | 4.12% | own 2.60; slider lookups 1.36 |
| `gen_legal_captures` | 3.68% | generator 2.02 |
| `make_move` | 3.37% | own 1.10; `move_piece` 0.93; checkers 0.67; `remove_piece` 0.41 |
| generation, out of line | 2.96% | own 1.12; king-target tests 0.77 |
| `unmake_move` | 1.46% | own 0.81 (2.68% before C1) |
| `is_legal`, `check_squares`, repetition tests | 0.44%, 0.37%, 0.53% | |

Every candidate's target clears the 0.5% gate: make and unmake 4.83%
(undo as one block, the byte mailbox), make's key helpers 1.34% (keys by
table), `see_ge`'s own code 2.82% (next attacker by index), make's
checkers 0.67% (the hinted make), and for the attack map the king-target
tests 0.77%, castling, the quiet scorer's threat maps and `see_ge`'s
`attackers_to` 1.94%.

The one uncovered target above 0.5% is `gives_check`'s own code (2.60%).
Its two measured forms (C2's pre-test, the lookup-free test) read no gain
and are not retried (PLAN B.7.7); no other mechanism is stated for it, so
nothing is prototyped for it here.

## 2. The prototypes

One scratch worktree at `edc6584`, every candidate behind its own macro
(`tools/results/b77/prototypes.patch`, SHA-256 291762B1F4C2C5D4…). Plain
release builds, Clang 23.1.5, `-DPORTABLE_BUILD=ON -DUSE_PEXT=ON
"-DCMAKE_CXX_FLAGS=-D<macro>"`; binaries in `tools/results/b77/bin/`.

| Arm | Macro | What it does |
|---|---|---|
| p0 | none | the head, built from the same tree: the base of every A/B |
| p1 | `PROTO_UNDO_IMAGE` | make saves the piece bitboards, occupancies and king squares beside the keys; unmake copies them back and restores the mailbox with two to four stores; the repetition scans read a dense key array |
| p2 | `PROTO_BYTE_MAILBOX` | `Piece` in one byte, the mailbox 64 bytes |
| p12 | both | the mailbox joins the image: unmake has no branch on the move's kind |
| p3 | `PROTO_KEY_TABLE` | one row per colour, piece and square with the main, pawn, minor and non-pawn key parts; the key helpers test no piece type |
| p4 | `PROTO_SEE_INDEX` | `see_ge` filters pins once over the side's non-king attackers and takes the cheapest kind as the lowest set bit of a kinds mask |
| p5 | `PROTO_MAKE_HINT` | negamax and quiescence pass `gives_check` to make where they already have it; on "no", the child's checkers are 0 without `attackers_to` |
| p6 | `PROTO_ATTACK_MAP` | one enemy attack map per node, built on first use and kept per ply under the position key, read by the quiet stage's king targets (out of check) and castling, the quiet scorer's threat maps, and `see_ge`'s exit (not for en passant, and with a line test through the mover's origin) |

Not prototyped as written: undo's fixed array and its no-allocation
contract. The prototype keeps the reserved vectors, which allocate at no
search node already (B.7.1's capacity contract); the speed question is the
image, which is built.

**Exact, every arm** (PLAN's rule): `bench 13` 4,247,965; `test_board_v2`;
`test_search_path`; the twelve microbenchmark checksums of
`tools/results/b72-micro/manifest.json`; `board_replay --verify` on B.7.6's
fresh `bench 13` trace (all 58,598,640 records). p1 and p12 also pass
`test_board`, `test_stack_unwind` (the 160-ply unwind) and
`test_invariants`. p5's hint was checked by a build that aborts when the
search's answer disagrees with the checkers make computes
(`PROTO_HINT_CHECK`): `bench 13` ran through. The replay cannot exercise
the hint (its replayer makes moves unhinted), so p5's replay row reads only
the unhinted path; the engine screen decides p5.

## 3. Predictions, frozen before any read

Engine screen against p0, median of 16 rounds, plain builds:

| Arm | Prediction | 80% interval | Why |
|---|---:|---|---|
| p1 | +0.3% | [−0.3%, +1.0%] | PLAN's prior, 0 to +1% beyond C1; make writes 136 bytes more |
| p2 | 0.0% | [−0.3%, +0.3%] | smaller mailbox, byte loads |
| p12 | +0.4% | [−0.3%, +1.1%] | p1 with unmake's last branches gone |
| p3 | +0.1% | [−0.3%, +0.5%] | three type tests per helper replaced by loads from a table four times larger |
| p4 | +0.2% | [−0.3%, +0.6%] | TB-6's −0.38% and −0.14% cycles; the pin filter runs once |
| p5 | 0.0% | [−0.5%, +0.4%] | the review's replay read no gain, its nearest form −0.74% |
| p6 | −0.3% | [−0.9%, +0.2%] | all three priors negative (−0.35%, −0.58%, −0.16%) |

Falsifiers: an arm outside its interval is a miss in magnitude or sign,
recorded in §4 against its row.

## 4. Reads

Waiting for the idle host.

## 5. Handoff

Waiting for the reads.
