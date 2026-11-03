"""Suite perft del Chess960 (27/09/2026). Uso:
    python perft960.py <motore.exe> <frcperftsuite.epd> [--depth 4] [--cmd perft] [--max N]
Riga della suite: FEN ;D1 n ;D2 n ... (formato di _reference/Viridithas_20/assets/epds/frcperftsuite.epd).
Un solo processo, UCI_Chess960 acceso; confronta il conteggio del comando `<cmd> D` (perft = scacchiera globale,
tdperft = scacchiera per thread, se il binario lo ha) col valore atteso a profondita' D. Stampa gli errori.
"""
import argparse
import re
import subprocess
import time

ap = argparse.ArgumentParser()
ap.add_argument("exe"); ap.add_argument("epd")
ap.add_argument("--depth", type=int, default=4)
ap.add_argument("--cmd", default="perft")
ap.add_argument("--max", type=int, default=0)
a = ap.parse_args()

p = subprocess.Popen([a.exe], stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True, bufsize=1)
def w(c): p.stdin.write(c + "\n"); p.stdin.flush()
w("setoption name UCI_Chess960 value true")
lines = [l.strip() for l in open(a.epd) if l.strip()]
if a.max: lines = lines[:a.max]
bad = 0; tot = 0; t0 = time.time()
for i, l in enumerate(lines):
    fen, *rest = [x.strip() for x in l.split(";")]
    exp = {int(m.group(1)): int(m.group(2)) for x in rest for m in [re.match(r"D(\d+)\s+(\d+)", x)] if m}
    if a.depth not in exp: continue
    w("position fen " + fen); w(f"{a.cmd} {a.depth}"); w("isready")
    got = None; checks = ""
    while True:
        s = p.stdout.readline()
        if not s: raise SystemExit("motore uscito su " + fen)
        m = re.search(r"Nodes:\s*(\d+)", s)
        if m: got = int(m.group(1))
        # tdperft: chiavi/mailbox/occupazioni (keymism) e falsi positivi di td_is_pseudo_legal (plfp) devono essere 0
        c = re.search(r"keymism (\d+) plfp (\d+)", s)
        if c and (c.group(1) != "0" or c.group(2) != "0"): checks = f" keymism {c.group(1)} plfp {c.group(2)}"
        if s.startswith("readyok"): break
    tot += 1
    if got != exp[a.depth] or checks:
        bad += 1
        print(f"ERRORE {fen}  D{a.depth}: atteso {exp[a.depth]}, ottenuto {got}{checks}", flush=True)
    if (i + 1) % 100 == 0:
        print(f"{i + 1}/{len(lines)}  errori {bad}  ({time.time() - t0:.0f} s)", flush=True)
w("quit")
print(f"FINE: {tot} posizioni a profondita' {a.depth}, errori {bad}, {time.time() - t0:.0f} s")
