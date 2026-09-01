"""KBN-K outcome census, classified and Syzygy-validated (BAS-E35).

Replaces the ad-hoc instrument behind BAS-E29, which reported a single
"other / stalemate" bucket. That bucket hid two different things, and treating
it as a stalemate rate is what put 5.9.21 on the plan.

What this fixes:
  * splits the bucket into stalemate / piece-lost / fifty-move / mated;
  * probes Syzygy so the denominator is positions that are ACTUALLY WON --
    random placement can leave a piece en prise or trapped;
  * records the PLY of each event, not the halfmove clock, because a capture
    resets the clock and makes piece-loss look like it happened at move 0;
  * sends ucinewgame per position, so one game cannot poison the next through
    the TT (the first sweep reused one engine and was not reproducible).

Needs: python-chess, and the Syzygy 3-4-5-6 set at D:/chess/tablebases/syzygy3456.
Usage:  python tools/diag/kbnk_outcomes.py
"""
import chess, chess.engine, chess.syzygy, collections

ENGINE = "D:/code/basilisk/build/release/basilisk.exe"
TB     = "D:/chess/tablebases/syzygy3456"
NODES  = 60000

M64 = (1 << 64) - 1
class Lcg:
    def __init__(self, s): self.s = s
    def next(self):
        self.s = (self.s * 6364136223846793005 + 1442695040888963407) & M64
        return self.s >> 33
    def below(self, n): return self.next() % n

def random_kbnk(rng):
    for _p in range(400):
        sq = [rng.below(64) for _ in range(4)]
        if len(set(sq)) != 4: continue
        grid=[None]*64
        grid[sq[0]]='K'; grid[sq[1]]='B'; grid[sq[2]]='N'; grid[sq[3]]='k'
        rows=[]
        for r in range(7,-1,-1):
            row,run="",0
            for f in range(8):
                c=grid[r*8+f]
                if c is None: run+=1; continue
                if run: row+=str(run); run=0
                row+=c
            if run: row+=str(run)
            rows.append(row)
        fen="/".join(rows)+" w - - 0 1"
        try: b=chess.Board(fen)
        except ValueError: continue
        if not b.is_valid(): continue
        if b.is_check() or not any(b.legal_moves): continue
        return fen
    return None

rng = Lcg(0x5E9D18)
fens=[]
while len(fens) < 200:
    f = random_kbnk(rng)
    if f is None: break
    fens.append(f)

counts = collections.Counter()
sm_clocks, loss_clocks = [], []
ply_of = {"stalemate": [], "piece_lost": []}
with chess.syzygy.open_tablebase(TB) as tb, \
     chess.engine.SimpleEngine.popen_uci(ENGINE) as eng:
    for idx, fen in enumerate(fens, 1):
        if tb.probe_wdl(chess.Board(fen)) <= 0:
            counts["not_a_win"] += 1
            continue
        b = chess.Board(fen)
        game = object()                      # forces ucinewgame -> fresh TT
        for _p in range(400):
            if b.is_checkmate():
                counts["mated"] += 1; break
            if b.is_stalemate():
                counts["stalemate"] += 1; sm_clocks.append(b.halfmove_clock); ply_of["stalemate"].append(_p); break
            if b.is_insufficient_material():
                counts["piece_lost"] += 1; loss_clocks.append(b.halfmove_clock); ply_of["piece_lost"].append(_p); break
            if b.halfmove_clock >= 100:
                counts["fifty_move"] += 1; break
            r = eng.play(b, chess.engine.Limit(nodes=NODES), game=game)
            if r.move is None:
                counts["no_move"] += 1; break
            b.push(r.move)
        else:
            counts["budget"] += 1
        if idx % 50 == 0: print(f"  {idx}/200 ...", flush=True)

won = sum(v for k, v in counts.items() if k != "not_a_win")
print(f"\nOver {won} positions Syzygy confirms are WON:\n")
for k in sorted(counts, key=lambda x: -counts[x]):
    if k == "not_a_win": continue
    print(f"  {k:12} {counts[k]:>4}  ({100*counts[k]/won:5.1f}%)")
print(f"\n  (excluded as not actually won: {counts['not_a_win']})")

def summarise(name, cl):
    if not cl:
        print(f"\n{name}: none"); return
    cl.sort()
    early = sum(1 for c in cl if c < 80)
    print(f"\n{name} halfmove clock at the event: {cl}")
    print(f"  median {cl[len(cl)//2]},  below 80 (a LIVE win thrown away): {early}/{len(cl)}")

summarise("stalemate", sm_clocks)
summarise("piece loss", loss_clocks)

print("\n--- PLY at the event (clock resets on capture, so ply is the honest axis) ---")
for k, v in ply_of.items():
    if not v: print(f"{k}: none"); continue
    v.sort()
    print(f"{k:11} n={len(v):>3}  median ply {v[len(v)//2]:>3}  min {v[0]:>3}  max {v[-1]:>3}")
    print(f"            plies: {v}")
