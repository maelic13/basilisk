# Basilisk / Stockfish HCE oracle (PLAN 5.1)

This directory builds a **measurement instrument**, not a chess engine we ship.
It answers one question:

> How much of Basilisk's gap to the historical frontier is caused by *search*,
> and how much by *evaluation*?

Those two are normally impossible to separate, because every engine's search
and evaluation are measured together. Here each is isolated by holding the
other exactly constant.

## What it is

One executable containing **Stockfish `9587eeeb`** — the last pure-HCE master
commit, immediately before NNUE merged — with a UCI switch selecting which
evaluator its search calls:

| `Use Basilisk HCE` | What runs | Role |
|---|---|---|
| `true` (default) | Stockfish search + **Basilisk 1.9.3 HCE** | treatment |
| `false` | Stockfish search + Stockfish's own HCE | exact-revision control |

Both arms are the same binary, same compiler, same UCI setup and same time
management, so the *only* difference between them is the evaluation function.

## What crosses the boundary

Stockfish hands the bridge twelve piece bitboards, side to move, castling
rights and the rule-50 clock. The bridge rebuilds a Basilisk `Board` from them
and calls `Evaluator::evaluate`, returning centipawns from the side-to-move
point of view, which the adapter rescales to Stockfish's internal unit
(`PawnValueEg`/100).

Because Basilisk is C++, its evaluator **links directly** — there is no DLL, no
exported C ABI and no ABI-version handshake. What remains is a deliberate
*type firewall* (`basilisk_bridge.h`): both projects define `Color`,
`PieceType`, `Square`, `Value`, `WHITE`, `PAWN` and more at global scope, so
exactly one translation unit (`basilisk_bridge.cpp`) is allowed to see Basilisk
headers, and it exposes only primitive types.

En passant is deliberately **not** transferred. Basilisk's evaluator reads only
`pieces`, `occupancy`, `all_occ`, `board_sq`, `king_sq`, `side_to_move`,
`halfmove_clock`, `castling_rights` and `pawn_key`; no term consults `ep_sq`.
The conformance test enforces this rather than trusting it.

## Nothing in `src/` is modified

The oracle compiles Basilisk's real sources unchanged. Two consequences worth
knowing:

- `Board::put_piece()` is private, so the bridge populates the public fields
  directly instead. Making it reachable would have meant editing `src/` for an
  experiment, and the oracle's value depends on the evaluator under test being
  the released one.
- Both projects define a global `Bitboard PawnAttacks[2][64]`. This is the only
  genuine strong-symbol collision between the two link sets, and it is resolved
  by compiling the Basilisk group with `-DPawnAttacks=basilisk_PawnAttacks`.
  **`--allow-multiple-definition` would be silently catastrophic here**: it
  links cleanly and leaves Basilisk's evaluator reading Stockfish's table.
  Since both tables plausibly hold identical contents under identical colour
  numbering, the corruption could produce sane-looking evaluations and never be
  noticed.

## Build

```powershell
.\hybrid\build.ps1
```

Requires MSYS2 clang64. Output goes to `hybrid\dist\` with a manifest recording
both source revisions, the compiler, the flags and the binary SHA-256. The
script warns loudly if `src/` is dirty, because a dirty tree means the binary
does not measure released 1.9.3 evaluation.

## Conformance test — run this before believing any result

```powershell
clang++ -std=c++23 -O2 -DNDEBUG -DUSE_PEXT -DUSE_POPCNT -mbmi2 -mpopcnt `
  -Isrc -Ihybrid hybrid/conformance_test.cpp hybrid/basilisk_bridge.cpp `
  src/board.cpp src/bitboard.cpp src/move.cpp src/attacks.cpp `
  src/zobrist.cpp src/eval.cpp -o hybrid/build/conformance_test.exe
.\hybrid\build\conformance_test.exe
```

It random-walks legal games and, for every position, compares the bridge's
bitboard reconstruction against the same position parsed by Basilisk's own
`try_set_fen()`. Any disagreement means the oracle is not evaluating what
Basilisk evaluates. It also fails if the walk never reached en-passant or
in-check positions, since those are exactly the states the bridge drops.

Last run: **471,519 positions — 1,931 with en passant, 21,361 in check, 0
mismatches.**

Scale check, same position through both paths: Basilisk reports `+74` cp and
the oracle's `eval` reports `0.74`.

## Measured throughput

| Arm | NPS (`bench 16 1 10 default depth`) |
|---|---|
| Oracle — Basilisk HCE through Stockfish search | 2.55M |
| Control — Stockfish HCE through Stockfish search | 2.96M |

Native Basilisk 1.9.3 runs about 2.5M on this host, so the adapter costs
roughly 14%. For comparison, Rarog's DLL-based Stage-1 hybrid paid about 37%
(1.5M against 2.4M native). The oracle therefore runs at essentially native
Basilisk speed, which means the search contrast is close to throughput-neutral
— a better-controlled measurement than the experiment it replicates.

## How to run the experiment

Register the **same executable twice** in Colosseum:

| Colosseum name | `Use Basilisk HCE` |
|---|---|
| `Basilisk-SF-Oracle` | `true` |
| `SF-9587eeeb-HCE-control` | `false` |

Run against Basilisk 1.9.3 and Rarog 2.3.2 at `3+0.03`, 1T, paired UHO,
tablebases and ponder off, and **adjudication off** — score-based adjudication
is invalid across engines with different evaluators and moved a headline
estimate by ~75 Elo in the Rarog run (PLAN durable lesson 14).

Three contrasts decide the phase:

| Contrast | Isolates | Decides |
|---|---|---|
| Oracle − Basilisk 1.9.3 | search, HCE held at ours | size of the search track |
| Control − Oracle | HCE, search held at Stockfish's | size of the HCE track |
| Oracle − Rarog hybrid | our HCE vs Rarog's, one search | whether our HCE leads |

**Stop rule:** if the search contrast is under roughly 50 Elo, Phase 5's
premise is wrong for Basilisk — close the acceleration program and go to NNUE.

## Rules

This branch is frozen once the experiment closes. Never merge it, never edit it
retrospectively, never ship any part of it. It exists to size the opportunity
and later to explain results — not to become product code. Basilisk remains an
independent engine; see PLAN's Independence contract.

## Licensing

Stockfish is GPLv3 and so is Basilisk. The vendored Stockfish tree keeps its
own `Copying.txt`, `AUTHORS` and attribution, and `SOURCE_COMMIT` records the
exact revision. The Stage-1 adapter design is adapted from Rarog's `hybrid`
branch.
