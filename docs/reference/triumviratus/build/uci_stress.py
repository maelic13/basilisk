"""Stress test del protocollo UCI (audit D, 26/09/2026).

Uso: python uci_stress.py <engine.exe> <stockfish.exe> [--threads N] [--hash MB] [--syzygy PATH]
Ogni scenario manda una sequenza di comandi, misura i tempi di risposta e controlla con Stockfish
(`go perft 1` elenca le mosse legali) che `bestmove` e la mossa di ponder siano legali.
Stampa OK/FAIL per scenario; FAIL anche se il motore muore, si blocca o risponde in ritardo.
"""
import argparse
import queue
import subprocess
import threading
import time


class Eng:
    def __init__(self, exe, opts):
        self.p = subprocess.Popen([exe], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                  stderr=subprocess.STDOUT, text=True, bufsize=1)
        self.q = queue.Queue()
        threading.Thread(target=self._reader, daemon=True).start()
        self.send("uci"); self.wait("uciok", 10)
        for k, v in opts:
            self.send(f"setoption name {k} value {v}")
        self.ready(30)

    def _reader(self):
        for line in self.p.stdout:
            self.q.put(line.rstrip("\n"))
        self.q.put(None)

    def send(self, c):
        try:
            self.p.stdin.write(c + "\n"); self.p.stdin.flush()
        except Exception:
            pass

    def wait(self, prefix, timeout):
        """Aspetta una riga che inizia con prefix; ritorna (riga, secondi, righe viste) o (None, ...)."""
        t0 = time.perf_counter(); seen = []
        while True:
            left = timeout - (time.perf_counter() - t0)
            if left <= 0:
                return None, time.perf_counter() - t0, seen
            try:
                line = self.q.get(timeout=left)
            except queue.Empty:
                return None, time.perf_counter() - t0, seen
            if line is None:
                return "DIED", time.perf_counter() - t0, seen
            seen.append(line)
            if line.startswith(prefix):
                return line, time.perf_counter() - t0, seen

    def ready(self, timeout=5):
        self.send("isready")
        return self.wait("readyok", timeout)

    def alive(self):
        return self.p.poll() is None

    def quit(self):
        self.send("quit")
        try:
            self.p.wait(timeout=5)
        except Exception:
            self.p.kill()


class Legal:
    """Mosse legali via Stockfish `go perft 1`."""
    def __init__(self, sf):
        self.e = Eng(sf, [])

    def moves(self, pos_cmd):
        self.e.send(pos_cmd); self.e.send("go perft 1")
        line, _, seen = self.e.wait("Nodes searched", 10)
        return {l.split(":")[0].strip() for l in seen if ":" in l and len(l.split(":")[0].strip()) in (4, 5)}


RESULTS = []


def report(name, ok, detail=""):
    RESULTS.append((name, ok))
    print(f"{'OK  ' if ok else 'FAIL'} {name}  {detail}", flush=True)


def bm_parts(line):
    t = line.split()
    best = t[1] if len(t) > 1 else None
    ponder = t[3] if len(t) > 3 and t[2] == "ponder" else None
    return best, ponder


def check_best(name, eng, legal, pos_cmd, line, dt, limit_s, expect_none=False):
    if line is None or line == "DIED":
        report(name, False, f"nessun bestmove ({line}), {dt:.2f}s, vivo={eng.alive()}"); return
    best, ponder = bm_parts(line)
    lm = legal.moves(pos_cmd)
    if expect_none:
        report(name, best in ("(none)", "0000", "a1a1") and dt <= limit_s, f"'{line}' {dt:.2f}s"); return
    ok = best in lm and dt <= limit_s
    detail = f"'{line}' {dt:.2f}s"
    if best in lm and ponder:
        lm2 = legal.moves(pos_cmd + (" moves " if " moves " not in pos_cmd else " ") + best)
        if ponder not in lm2:
            ok = False; detail += " PONDER ILLEGALE"
    if best not in lm:
        detail += " BESTMOVE ILLEGALE"
    if dt > limit_s:
        detail += f" TROPPO LENTO (> {limit_s}s)"
    report(name, ok, detail)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("exe"); ap.add_argument("sf")
    ap.add_argument("--threads", type=int, default=1)
    ap.add_argument("--hash", type=int, default=64)
    ap.add_argument("--syzygy", default="")
    a = ap.parse_args()
    opts = [("Threads", a.threads), ("Hash", a.hash)]
    if a.syzygy:
        opts.append(("SyzygyPath", a.syzygy))
    legal = Legal(a.sf)
    e = Eng(a.exe, opts)
    S = "position startpos moves e2e4 e7e5 g1f3 b8c6 f1b5"

    # 1. go infinite + stop
    e.send("ucinewgame"); e.send(S); e.send("go infinite"); time.sleep(1.0)
    t0 = time.perf_counter(); e.send("stop"); line, dt, _ = e.wait("bestmove", 5)
    check_best("go infinite + stop", e, legal, S, line, dt, 0.5)

    # 2. isready durante la ricerca
    e.send(S); e.send("go infinite"); time.sleep(0.5)
    r, dt, _ = e.ready(2)
    report("isready durante la ricerca", r == "readyok" and dt < 0.2, f"{r} {dt:.3f}s")
    e.send("stop"); e.wait("bestmove", 5)

    # 3. ponder + ponderhit (il tempo parte al ponderhit)
    P = "position startpos moves d2d4 d7d5 c2c4 e7e6"
    e.send(P); e.send("go ponder wtime 10000 btime 10000 winc 100 binc 100"); time.sleep(1.0)
    t0 = time.perf_counter(); e.send("ponderhit"); line, dt, _ = e.wait("bestmove", 12)
    check_best("ponder + ponderhit (10s+0.1)", e, legal, P, line, dt, 3.0)

    # 4. ponder + stop (ponder miss)
    e.send(P); e.send("go ponder wtime 10000 btime 10000 winc 100 binc 100"); time.sleep(1.0)
    e.send("stop"); line, dt, _ = e.wait("bestmove", 5)
    check_best("ponder + stop", e, legal, P, line, dt, 0.5)

    # 5. tempo bassissimo / negativo / movestogo=1
    for name, go, lim in [("wtime 30", "go wtime 30 btime 30", 0.2),
                          ("wtime negativo", "go wtime -200 btime 1000 winc 0", 0.3),
                          ("movestogo 1 wtime 1000", "go wtime 1000 btime 1000 movestogo 1", 1.0),
                          ("movestogo 40 wtime 60000", "go wtime 60000 btime 60000 movestogo 40", 4.0),
                          ("inc 0 wtime 2000", "go wtime 2000 btime 2000", 0.3),
                          ("movetime 200", "go movetime 200", 0.35),
                          ("nodes 1", "go nodes 1", 0.3),
                          ("depth 1", "go depth 1", 0.3),
                          ("mate 3", "go mate 3 movetime 2000", 2.5)]:
        e.send("ucinewgame"); e.send(S); e.send(go)
        line, dt, _ = e.wait("bestmove", lim + 5)
        check_best(f"go {name}", e, legal, S, line, dt, lim)

    # 6. searchmoves
    e.send(S); e.send("go depth 8 searchmoves a7a6 d7d6")
    line, dt, _ = e.wait("bestmove", 10)
    ok = line not in (None, "DIED") and bm_parts(line)[0] in ("a7a6", "d7d6")
    report("go searchmoves", ok, f"'{line}'")

    # 7. MultiPV maggiore delle mosse legali (3 mosse legali)
    M = "position fen 7k/8/8/8/8/8/6q1/K7 w - - 0 1"
    e.send("setoption name MultiPV value 5"); e.send(M); e.send("go depth 8")
    line, dt, seen = e.wait("bestmove", 10)
    check_best("MultiPV 5 con poche mosse legali", e, legal, M, line, dt, 5.0)
    e.send("setoption name MultiPV value 1")

    # 8. posizioni senza mosse legali
    for name, fen in [("matto", "7k/6Q1/6K1/8/8/8/8/8 b - - 0 1"), ("stallo", "7k/5Q2/6K1/8/8/8/8/8 b - - 0 1")]:
        pc = f"position fen {fen}"; e.send(pc); e.send("go depth 5")
        line, dt, _ = e.wait("bestmove", 5)
        check_best(f"bestmove in {name}", e, legal, pc, line, dt, 1.0, expect_none=True)

    # 9. partite lunghe: storia di 300, 1000, 2100 semimosse (manovre di cavallo)
    shuffle = ["g1f3", "g8f6", "f3g1", "f6g8"]
    for plies in (300, 1000, 2100):
        mv = " ".join(shuffle[i % 4] for i in range(plies))
        pc = f"position startpos moves e2e4 e7e5 {mv}"
        e.send(pc); e.send("go depth 6")
        line, dt, _ = e.wait("bestmove", 20)
        check_best(f"storia di {plies} semimosse", e, legal, pc, line, dt, 10.0)

    # 10. setoption Hash durante la ricerca, poi nuova ricerca
    e.send(S); e.send("go infinite"); time.sleep(0.5)
    e.send("setoption name Hash value 256"); time.sleep(0.3); e.send("stop")
    line, dt, _ = e.wait("bestmove", 5)
    e.send(S); e.send("go depth 10"); line2, dt2, _ = e.wait("bestmove", 30)
    report("setoption Hash durante la ricerca", line not in (None, "DIED") and line2 not in (None, "DIED") and e.alive(),
           f"'{line}' / '{line2}'")

    # 11. raffica: 200 x (ucinewgame, position, go movetime 5)
    t0 = time.perf_counter(); fails = 0
    for i in range(200):
        e.send("ucinewgame"); e.send(S); e.send("go movetime 5")
        line, dt, _ = e.wait("bestmove", 3)
        if line in (None, "DIED"):
            fails += 1; break
    report("raffica 200 go movetime 5", fails == 0 and e.alive(), f"{time.perf_counter() - t0:.1f}s")

    # 12. quit durante la ricerca
    e.send(S); e.send("go infinite"); time.sleep(0.5); e.send("quit")
    try:
        e.p.wait(timeout=5); report("quit durante la ricerca", True, "")
    except Exception:
        report("quit durante la ricerca", False, "non esce"); e.p.kill()

    # 13. FEN non valide (un GUI non le manda, ma non devono far crollare il motore)
    for name, fen in [("senza re", "8/8/8/8/8/8/8/8 w - - 0 1"),
                      ("campi mancanti", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w"),
                      ("lato che non muove in scacco", "4k3/8/8/8/8/8/4R3/4K3 b - - 0 1".replace(" b ", " w ")),
                      ("ep impossibile", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq e3 0 1")]:
        f = Eng(a.exe, opts)
        f.send(f"position fen {fen}"); f.send("go depth 4")
        line, dt, _ = f.wait("bestmove", 5)
        report(f"FEN non valida: {name}", line != "DIED" and f.alive(), f"'{line}' vivo={f.alive()}")
        f.quit()

    # 14. Chess960 (28/09/2026). Arbitro: Stockfish con UCI_Chess960 (arrocco = re cattura torre, come il nostro).
    legal960 = Legal(a.sf); legal960.e.send("setoption name UCI_Chess960 value true"); legal960.e.ready(5)
    g = Eng(a.exe, opts + [("UCI_Chess960", "true")])
    SP = "rkrnnqbb/pppppppp/8/8/8/8/PPPPPPPP/RKRNNQBB w KQkq - 0 1"      # posizione 959, re b1 fra due torri
    cases = [
        ("960: partenza 959 (X-FEN)", f"position fen {SP}"),
        ("960: stessa partenza in Shredder-FEN", f"position fen {SP.replace('KQkq', 'CAca')}"),
        ("960: arrocco lungo con re che resta quasi fermo (b1a1)",
         "position fen 6k1/8/8/8/8/8/8/RK2R3 w KQ - 0 1 moves b1a1"),   # (28/09: prima c'era una torre in b8 che
         # dava scacco -> arrocco illegale; il nostro motore lo rifiutava, Stockfish si bloccava e falsava il resto)
        ("960: arrocco corto con re fermo in g1 (g1h1)",
         "position fen 6k1/8/8/8/8/8/8/R5KR w KQ - 0 1 moves g1h1"),
        ("960: re che arriva sulla casa della torre (f1g1)",
         "position fen 4k3/8/8/8/8/8/8/R4KR1 w KQ - 0 1 moves f1g1"),
        ("960: partita normale in notazione 960 (e1h1)",
         "position startpos moves e2e4 e7e5 g1f3 b8c6 f1c4 g8f6 e1h1"),
    ]
    for name, pc in cases:
        g.send("ucinewgame"); g.send(pc); g.send("go depth 8")
        line, dt, _ = g.wait("bestmove", 10)
        check_best(name, g, legal960, pc, line, dt, 5.0)
    # l'arrocco deve essere GENERATO e giocabile: posizione dove arroccare e' la mossa naturale
    pc = "position fen 1r2k3/8/8/8/8/8/5PPP/4K2R w K - 0 1"
    g.send(pc); g.send("go depth 6 searchmoves e1h1")
    line, dt, _ = g.wait("bestmove", 10)
    report("960: arrocco generato e scritto re-cattura-torre", line not in (None, "DIED") and bm_parts(line)[0] == "e1h1",
           f"'{line}'")
    # si spegne il 960 a meta' sessione: torna la notazione normale (e1g1)
    g.send("setoption name UCI_Chess960 value false"); g.send("ucinewgame")
    pc = "position startpos moves e2e4 e7e5 g1f3 b8c6 f1c4 g8f6 e1g1"
    g.send(pc); g.send("go depth 8"); line, dt, _ = g.wait("bestmove", 10)
    check_best("960 spento di nuovo: arrocco normale e1g1", g, legal, pc, line, dt, 5.0)
    g.quit(); legal960.e.quit()

    legal.e.quit()
    n_fail = sum(1 for _, ok in RESULTS if not ok)
    print(f"\nscenari {len(RESULTS)}  falliti {n_fail}")


if __name__ == "__main__":
    main()
