# BAS-E53

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Cluster 5.8 closing summary -->

**BAS-E53 - current 6.8.a: the narrow-window rook failure IS resolvable, and not by
depth** (2026-09-07). The leaf's direction rested on the claim that "a static
evaluation supplies a gradient over many moves; it cannot reliably pick two
exact moves out of twenty". That is an assertion about possibility, and it has
a decisive test.

Instrument: `tools/diag/narrow_node_probe.py`. Whole-game replays cannot answer
this, because two engines diverge at the first differing move and any
preservation difference is then partly a difference of trajectory. So the 48
clean wins of KRP-KR and KRPP-KRP were replayed ONCE by the frozen head at
60,000 nodes and every White-to-move node still holding a clean win was frozen
with its `win_moves` count -- **841 nodes**, 9 dropped as unprobeable. Each arm
then answers those same 841 positions with one move, with a fresh game token
per node so no arm gets TT carry-over another lacks, and with `SyzygyPath`
cleared (Stockfish verified at `tbhits 0`). Buckets fixed before any arm ran.

| arm | narrow (161) | mid (133) | wide (547) |
|---|---|---|---|
| basilisk@60k | **91.3%** | 98.5% | 99.6% |
| basilisk@300k | **92.5%** | 98.5% | 100.0% |
| basilisk@600k | **93.8%** | 99.2% | 100.0% |
| stockfish@60k | **99.4%** | 100.0% | 100.0% |

Paired McNemar on the narrow nodes against basilisk@60k: stockfish@60k **13-0,
z = 3.61**; basilisk@600k 6-2, z = 1.41; basilisk@300k 4-2, z = 0.82.

**The claim is refuted.** Stockfish resolves 160 of 161 narrow nodes at the
SAME budget where Basilisk resolves 147. **Depth is not the answer either**:
ten times the nodes recovers 4 of 14 errors and does not approach
stockfish@60k, so this is not deferrable to the later whole-search phase. The
wide bucket is at ceiling for both engines, so the deficit is specific to the failure class and
not general strength -- 13 lost nodes out of 161 narrow against 4 out of 680
elsewhere.

By exact winning-move count it is an **only-move** problem: at `win_moves == 1`
(68 nodes) Basilisk preserves 85%, at 600k nodes 90%, Stockfish 100%. Every
bucket at 4 or more winning moves is at or within one node of ceiling for
everyone.

**Piece selection is not the mechanism.** At the 13 discordant nodes Basilisk's
losing moves are 8 rook, 4 king, 1 pawn; Stockfish's winning moves are 8 rook,
5 king. The engine reaches for the right piece and the wrong square. This kills
premature passer advance (one pawn move in thirteen) and any "wrong piece"
account.

*What this does not license.* Stockfish evaluates with an NNUE, so "resolvable
by an evaluator" is not "resolvable by a hand-crafted term"; the impossibility
argument is removed, no mechanism is supplied. The 13 discordant nodes suggest
rook placement but are not a mechanism either -- thirteen cases is the same
sample size that already misled this leaf once. Cross-engine node parity is
approximate; the 600k arm is what protects the conclusion.

*Disposition.* Observation. No engine change. The only-move question is now
6.8.a: it is no longer whether the class is resolvable, but what knowledge
resolves it. The separate 6.5.a scale candidate remains prepared. Analysis in
`analysis/rook_narrow_node_resolvability_v1.md`.
