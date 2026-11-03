"""Profondita' raggiunte nelle partite di un PGN fastchess (commenti "{score/depth time...}"). 27/09/2026.
Uso: python pgn_depths.py <file.pgn> [--knee 17] [--engine nome]
Stampa, per fase (pezzi sulla scacchiera, stimati dal numero di catture nel SAN) e in totale, la distribuzione della
profondita' di ricerca delle mosse e la quota di mosse con profondita' > knee (dove agiscono le opzioni RDR*).
"""
import argparse
import re
from collections import defaultdict

ap = argparse.ArgumentParser()
ap.add_argument("pgn"); ap.add_argument("--knee", type=int, default=17); ap.add_argument("--engine", default="")
a = ap.parse_args()
txt = open(a.pgn, encoding="utf-8", errors="replace").read()
by_phase = defaultdict(list)
for g in re.split(r"\n(?=\[Event )", txt):
    if "\n\n" not in g:
        continue
    tags = dict(re.findall(r'\[(\w+) "([^"]*)"\]', g))
    body = g.split("\n\n", 1)[1]
    ply = 0; pieces = 32
    for san, com in re.findall(r"([^\s{}]+)\s*\{([^}]*)\}", body):
        if re.match(r"^\d+\.+$", san):
            continue
        mover = tags.get("White") if ply % 2 == 0 else tags.get("Black")
        ply += 1
        if "x" in san:
            pieces -= 1
        m = re.match(r"\s*[+-]?(?:M)?[\d.]+/(\d+)", com)
        if not m or "book" in com:
            continue
        if a.engine and a.engine not in (mover or ""):
            continue
        ph = ">24 pezzi" if pieces > 24 else "13-24" if pieces > 12 else "7-12" if pieces > 6 else "<=6"
        by_phase[ph].append(int(m.group(1)))
allv = [d for v in by_phase.values() for d in v]
def line(name, v):
    v = sorted(v); n = len(v)
    if not n:
        return
    over = sum(1 for d in v if d > a.knee)
    print(f"{name:10s} mosse {n:6d}  mediana {v[n // 2]:3d}  p25 {v[n // 4]:3d}  p75 {v[3 * n // 4]:3d}  "
          f"> {a.knee}: {100 * over / n:5.1f}%")
for ph in [">24 pezzi", "13-24", "7-12", "<=6"]:
    line(ph, by_phase[ph])
line("TOTALE", allv)
