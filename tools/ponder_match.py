"""Ponder-on engine match: the protocol gate fastchess cannot run.

fastchess (tools/sprt.ps1) has no ponder support, so every harness game Basilisk
has ever played ran with Ponder off. This referee plays real clocked games with
Ponder on and drives the UCI ponder protocol the way a GUI does:

  after engine E moves "bestmove m ponder p":  position ... m p / go ponder <clocks>
  opponent plays p   -> "ponderhit"; E's clock runs from that instant
  opponent plays !p  -> "stop"; E's (discarded) bestmove must arrive; then
                        position / go <clocks>

The ponderhit is sent the moment the opponent's move is read, so an opponent
that replies instantly (a ponder hit whose soft limit has already expired, a
tablebase root) reproduces the ~1 ms reply of BAS-C10.

Failures counted per engine (any one is a hard-gate failure):
  forfeit     - clock exceeded by more than --timemargin
  hang        - forfeit AND still no bestmove 5 s later (BAS-C10's signature)
  stop_stall  - no bestmove within 5 s of "stop" on a ponder miss
  early_bm    - bestmove while pondering, before ponderhit/stop
  illegal     - illegal or unparseable bestmove
  crash       - process exit or no uciok/readyok
Games end only by chess rules (checkmate, stalemate, insufficient material,
threefold, fifty moves) or a failure; no score adjudication. A 600-ply cap
scores a draw and is counted separately.

Exposure is reported too: "fast hit" counts ponderhits sent within 2 ms of
that engine's own `go ponder`, i.e. how often the BAS-C10 window was
actually offered.

Example:
  python tools/ponder_match.py --engine-a A.exe --engine-b B.exe --games 400
"""
import argparse
import datetime
import json
import math
import os
import queue
import random
import subprocess
import sys
import threading
import time
from concurrent.futures import ThreadPoolExecutor

import chess
import chess.pgn

FAIL_KINDS = ("forfeit", "hang", "stop_stall", "early_bm", "illegal", "crash")
STOP_GRACE = 5.0
HANG_GRACE = 5.0
FAST_HIT_S = 0.002
MAX_PLIES = 600


class EngineDead(Exception):
    pass


class Uci:
    def __init__(self, path, options):
        self.p = subprocess.Popen([path], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                  stderr=subprocess.DEVNULL, text=True, bufsize=1)
        self.q = queue.Queue()
        threading.Thread(target=self._pump, daemon=True).start()
        self.send("uci")
        if self.wait_prefix("uciok", 15) is None:
            raise EngineDead("no uciok")
        for name, value in options:
            self.send(f"setoption name {name} value {value}")
        self.ready()

    def _pump(self):
        for line in self.p.stdout:
            self.q.put((time.perf_counter(), line.strip()))
        self.q.put((time.perf_counter(), None))

    def send(self, line):
        try:
            self.p.stdin.write(line + "\n")
            self.p.stdin.flush()
        except OSError as exc:
            raise EngineDead(str(exc))

    def wait_prefix(self, prefix, timeout):
        """Return (t, line) of the first line starting with prefix, else None."""
        end = time.perf_counter() + timeout
        while True:
            left = end - time.perf_counter()
            if left <= 0:
                return None
            try:
                t, line = self.q.get(timeout=left)
            except queue.Empty:
                return None
            if line is None:
                raise EngineDead("process exited")
            if line.startswith(prefix):
                return t, line

    def pending_bestmove(self):
        """Non-blocking: was a bestmove already emitted?"""
        found = False
        while True:
            try:
                _, line = self.q.get_nowait()
            except queue.Empty:
                return found
            if line is None:
                raise EngineDead("process exited")
            if line.startswith("bestmove"):
                found = True

    def ready(self):
        self.send("isready")
        if self.wait_prefix("readyok", 30) is None:
            raise EngineDead("no readyok")

    def close(self):
        try:
            self.send("quit")
            self.p.wait(timeout=3)
        except Exception:
            pass
        if self.p.poll() is None:
            self.p.kill()


def parse_bestmove(line):
    parts = line.split()
    best = parts[1] if len(parts) > 1 else ""
    ponder = parts[3] if len(parts) > 3 and parts[2] == "ponder" else None
    return best, ponder


def pos_cmd(start_fen, moves):
    cmd = f"position fen {start_fen}"
    return cmd + (" moves " + " ".join(moves) if moves else "")


def play_game(cfg, game_id, fen, a_is_white):
    """Returns a dict describing one game. Engine labels are 'A' and 'B'."""
    stats = {k: {f: 0 for f in FAIL_KINDS} | {"hits": 0, "misses": 0, "fast_hits": 0}
             for k in ("A", "B")}
    white, black = ("A", "B") if a_is_white else ("B", "A")
    engines = {}
    result, reason, loser = "*", "", None
    board = chess.Board(fen)
    start_fen = board.fen()
    moves = []
    clock = {chess.WHITE: cfg.base_ms, chess.BLACK: cfg.base_ms}
    inc = cfg.inc_ms
    # ponder state per engine label: (ponder_move_uci, t_go_ponder_sent) or None
    pondering = {"A": None, "B": None}

    def label(color):
        return white if color == chess.WHITE else black

    def fail(lbl, kind, why):
        nonlocal result, reason, loser
        stats[lbl][kind] += 1
        loser = lbl
        result = "0-1" if lbl == white else "1-0"
        reason = why

    try:
        for lbl, path, opts in (("A", cfg.engine_a, cfg.opts_a), ("B", cfg.engine_b, cfg.opts_b)):
            try:
                engines[lbl] = Uci(path, opts)
                engines[lbl].send("ucinewgame")
                engines[lbl].ready()
            except EngineDead as exc:
                fail(lbl, "crash", f"{lbl} failed to start: {exc}")
                raise StopIteration

        while True:
            outcome = board.outcome(claim_draw=True)
            if outcome is not None:
                result = outcome.result()
                reason = outcome.termination.name.lower()
                break
            if len(moves) >= MAX_PLIES:
                result, reason = "1/2-1/2", "ply_cap"
                break

            stm = board.turn
            lbl = label(stm)
            eng = engines[lbl]
            clocks = (f"wtime {max(0, clock[chess.WHITE])} btime {max(0, clock[chess.BLACK])} "
                      f"winc {inc} binc {inc}")
            last = moves[-1] if moves else None
            ponder_state = pondering[lbl]
            pondering[lbl] = None
            mode = "go"
            try:
                if ponder_state is not None:
                    pmove, t_go = ponder_state
                    if eng.pending_bestmove():
                        fail(lbl, "early_bm", f"{lbl} sent bestmove while pondering")
                        break
                    if last == pmove:
                        t_start = time.perf_counter()
                        eng.send("ponderhit")
                        stats[lbl]["hits"] += 1
                        mode = f"ponderhit {1000 * (t_start - t_go):.1f} ms after go ponder"
                        if t_start - t_go < FAST_HIT_S:
                            stats[lbl]["fast_hits"] += 1
                    else:
                        eng.send("stop")
                        stats[lbl]["misses"] += 1
                        if eng.wait_prefix("bestmove", STOP_GRACE) is None:
                            fail(lbl, "stop_stall", f"{lbl} no bestmove after stop (ponder miss)")
                            break
                        eng.send(pos_cmd(start_fen, moves))
                        t_start = time.perf_counter()
                        eng.send(f"go {clocks}")
                        mode = "go after ponder miss"
                else:
                    eng.send(pos_cmd(start_fen, moves))
                    t_start = time.perf_counter()
                    eng.send(f"go {clocks}")

                budget = clock[stm] + cfg.timemargin_ms
                got = eng.wait_prefix("bestmove", budget / 1000.0)
                if got is None:
                    late = eng.wait_prefix("bestmove", HANG_GRACE)
                    fail(lbl, "forfeit", f"{lbl} loses on time (clock {clock[stm]} ms; {mode})")
                    if late is None:
                        stats[lbl]["hang"] += 1
                        reason += " (hang: no bestmove 5 s later)"
                    break
                t_bm, line = got
                elapsed_ms = int((t_bm - t_start) * 1000)
                if elapsed_ms > clock[stm] + cfg.timemargin_ms:
                    fail(lbl, "forfeit", f"{lbl} loses on time ({elapsed_ms} of {clock[stm]} ms; {mode})")
                    break
                clock[stm] = clock[stm] - elapsed_ms + inc
                best, ponder = parse_bestmove(line)
                try:
                    move = chess.Move.from_uci(best)
                except ValueError:
                    move = None
                if move is None or move not in board.legal_moves:
                    fail(lbl, "illegal", f"{lbl} illegal move {best!r}")
                    break
                board.push(move)
                moves.append(best)

                if ponder and board.outcome(claim_draw=True) is None:
                    try:
                        pm = chess.Move.from_uci(ponder)
                    except ValueError:
                        pm = None
                    if pm is not None and pm in board.legal_moves:
                        pclocks = (f"wtime {max(0, clock[chess.WHITE])} btime {max(0, clock[chess.BLACK])} "
                                   f"winc {inc} binc {inc}")
                        eng.send(pos_cmd(start_fen, moves + [ponder]))
                        eng.send(f"go ponder {pclocks}")
                        pondering[lbl] = (ponder, time.perf_counter())
            except EngineDead as exc:
                fail(lbl, "crash", f"{lbl} died: {exc}")
                break

        # Game over: a pondering engine must still answer stop.
        for plbl, state in pondering.items():
            if state is None or plbl not in engines or loser == plbl:
                continue
            try:
                engines[plbl].send("stop")
                if engines[plbl].wait_prefix("bestmove", STOP_GRACE) is None:
                    stats[plbl]["stop_stall"] += 1
                    reason += f"; {plbl} no bestmove after final stop"
            except EngineDead:
                stats[plbl]["crash"] += 1
    except StopIteration:
        pass
    finally:
        for e in engines.values():
            e.close()

    return {"id": game_id, "fen": start_fen, "white": white, "moves": moves,
            "result": result, "reason": reason, "stats": stats}


def load_openings(path, n, rng):
    size = os.path.getsize(path)
    out = []
    with open(path, "rb") as f:
        while len(out) < n:
            f.seek(rng.randrange(size))
            f.readline()
            line = f.readline().decode().strip()
            if not line:
                continue
            fields = line.split()
            fen = " ".join(fields[:4]) + " 0 1"
            try:
                if chess.Board(fen).is_valid():
                    out.append(fen)
            except ValueError:
                pass
    return out


def write_pgn(fh, g, names):
    game = chess.pgn.Game()
    game.headers["Event"] = "ponder_match"
    game.headers["Round"] = str(g["id"])
    game.headers["White"] = names[g["white"]]
    game.headers["Black"] = names["B" if g["white"] == "A" else "A"]
    game.headers["Result"] = g["result"]
    game.headers["Termination"] = g["reason"]
    game.headers["FEN"] = g["fen"]
    game.headers["SetUp"] = "1"
    node = game
    b = chess.Board(g["fen"])
    for m in g["moves"]:
        mv = chess.Move.from_uci(m)
        node = node.add_variation(mv)
        b.push(mv)
    print(game, file=fh, end="\n\n", flush=True)


def elo(w, d, l):
    n = w + d + l
    if n == 0:
        return float("nan"), float("nan")
    s = (w + d / 2) / n
    if s <= 0 or s >= 1:
        return float("inf") if s >= 1 else float("-inf"), float("nan")
    var = (w * (1 - s) ** 2 + d * (0.5 - s) ** 2 + l * s ** 2) / n
    se = math.sqrt(var / n)
    to_elo = lambda x: -400 * math.log10(1 / x - 1)
    return to_elo(s), (to_elo(min(0.999, s + 1.96 * se)) - to_elo(max(0.001, s - 1.96 * se))) / 2


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--engine-a", required=True)
    ap.add_argument("--engine-b", required=True)
    ap.add_argument("--name-a", default="A")
    ap.add_argument("--name-b", default="B")
    ap.add_argument("--games", type=int, default=400, help="even; each opening is played with both colours")
    ap.add_argument("--tc", default="10+0.1", help="base+inc in seconds")
    ap.add_argument("--threads", type=int, default=1)
    ap.add_argument("--hash", type=int, default=0, help="MB; default 64 x threads")
    ap.add_argument("--syzygy", default="D:/chess/tablebases/syzygy3456", help="'' to disable")
    ap.add_argument("--concurrency", type=int, default=0,
                    help="default floor((physical cores - 2) / (2 x threads)): both engines search at once")
    ap.add_argument("--timemargin", type=int, default=100, help="ms")
    ap.add_argument("--book", default=os.path.join(os.path.dirname(__file__), "books", "UHO_Lichess_4852_v1.epd"))
    ap.add_argument("--seed", type=int, default=0)
    ap.add_argument("--out", default="", help="output directory (default tools/results/ponder_<ts>)")
    args = ap.parse_args()

    if args.games % 2:
        sys.exit("--games must be even")
    base, inc = args.tc.split("+")
    seed = args.seed or random.SystemRandom().randrange(1, 2**31)
    rng = random.Random(seed)
    hash_mb = args.hash or 64 * args.threads
    physical = int(subprocess.run(
        ["powershell", "-NoProfile", "-Command",
         "(Get-CimInstance Win32_Processor | Measure-Object NumberOfCores -Sum).Sum"],
        capture_output=True, text=True).stdout.strip() or os.cpu_count() // 2)
    conc = args.concurrency or max(1, (physical - 2) // (2 * args.threads))
    if conc * 2 * args.threads > physical:
        sys.exit(f"concurrency {conc} x 2 engines x {args.threads} threads oversubscribes {physical} physical cores")

    opts = [("Threads", args.threads), ("Hash", hash_mb), ("Ponder", "true")]
    if args.syzygy:
        opts.append(("SyzygyPath", args.syzygy))

    class Cfg:
        pass
    cfg = Cfg()
    cfg.engine_a, cfg.engine_b = os.path.abspath(args.engine_a), os.path.abspath(args.engine_b)
    cfg.opts_a = cfg.opts_b = opts
    cfg.base_ms, cfg.inc_ms = int(float(base) * 1000), int(float(inc) * 1000)
    cfg.timemargin_ms = args.timemargin

    ts = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    out = args.out or os.path.join(os.path.dirname(os.path.abspath(__file__)), "results", f"ponder_{ts}")
    os.makedirs(out, exist_ok=True)
    names = {"A": args.name_a, "B": args.name_b}
    manifest = {"engine_a": cfg.engine_a, "engine_b": cfg.engine_b, "names": names, "games": args.games,
                "tc": args.tc, "threads": args.threads, "hash": hash_mb, "syzygy": args.syzygy,
                "concurrency": conc, "physical_cores": physical, "timemargin_ms": args.timemargin,
                "book": args.book, "seed": seed, "started": ts}
    with open(os.path.join(out, "manifest.json"), "w") as f:
        json.dump(manifest, f, indent=2)
    print(json.dumps(manifest, indent=2), flush=True)

    openings = load_openings(args.book, args.games // 2, rng)
    jobs = [(2 * i + c, fen, c == 0) for i, fen in enumerate(openings) for c in (0, 1)]

    totals = {k: {f: 0 for f in FAIL_KINDS} | {"hits": 0, "misses": 0, "fast_hits": 0} for k in ("A", "B")}
    wdl = [0, 0, 0]
    lock = threading.Lock()
    done = 0
    discarded = 0
    # Ctrl+C reaches the engines as well as this script, so the games in play
    # end with dead engines. Once the operator stops the run those games are
    # discarded, never counted as engine failures, and no new game starts.
    stopping = threading.Event()
    started = time.monotonic()

    def clock(seconds):
        seconds = int(seconds)
        return f"{seconds // 3600}:{seconds // 60 % 60:02d}:{seconds % 60:02d}"
    pgn = open(os.path.join(out, "games.pgn"), "w")
    events = open(os.path.join(out, "failures.txt"), "w")

    def run(job):
        nonlocal done, discarded
        if stopping.is_set():
            return
        g = play_game(cfg, *job)
        if any(g["stats"][k]["crash"] for k in ("A", "B")):
            time.sleep(0.5)   # the interrupt may land a moment after the engines die
        if stopping.is_set():
            with lock:
                discarded += 1
                print(f"  - game {g['id']} discarded: the run was stopped", flush=True)
            return
        with lock:
            done += 1
            for k in ("A", "B"):
                for f, v in g["stats"][k].items():
                    totals[k][f] += v
            a_white = g["white"] == "A"
            if g["result"] == "1/2-1/2":
                wdl[1] += 1
            elif (g["result"] == "1-0") == a_white and g["result"] != "*":
                wdl[0] += 1
            elif g["result"] != "*":
                wdl[2] += 1
            write_pgn(pgn, g, names)
            if any(g["stats"][k][f] for k in ("A", "B") for f in FAIL_KINDS):
                events.write(f"game {g['id']} ply {len(g['moves'])}: {g['reason']}\n")
                events.flush()
                print(f"  ! game {g['id']}: {g['reason']}", flush=True)
            fa = sum(totals["A"][f] for f in FAIL_KINDS if f != "hang")
            fb = sum(totals["B"][f] for f in FAIL_KINDS if f != "hang")
            elapsed = time.monotonic() - started
            left = elapsed / done * (len(jobs) - done)
            print(f"[{done}/{len(jobs)}] {names['A']} W{wdl[0]} D{wdl[1]} L{wdl[2]} | "
                  f"failures {names['A']}={fa} {names['B']}={fb} | fast hits "
                  f"{totals['A']['fast_hits']}/{totals['B']['fast_hits']} | "
                  f"{clock(elapsed)} elapsed, about {clock(left)} left", flush=True)

    pool = ThreadPoolExecutor(max_workers=conc)
    futures = [pool.submit(run, job) for job in jobs]
    try:
        while not all(f.done() for f in futures):
            time.sleep(0.5)
    except KeyboardInterrupt:
        stopping.set()
        print(f"\nStopped by the operator after {done} games; games in play are discarded.", flush=True)
        for f in futures:
            f.cancel()
    pool.shutdown(wait=True)
    for f in futures:
        if f.done() and not f.cancelled() and f.exception():
            raise f.exception()
    pgn.close()
    events.close()

    e, ci = elo(*wdl)
    summary = {"games": done, "stopped_by_operator": stopping.is_set(), "discarded_games": discarded,
               "wdl_a": wdl, "elo_a": e, "elo_ci95": ci,
               "per_engine": {names[k]: totals[k] for k in ("A", "B")}}
    with open(os.path.join(out, "summary.json"), "w") as f:
        json.dump(summary, f, indent=2)
    print("\n=== Summary ===" + (f" (stopped after {done} of {len(jobs)} games)" if stopping.is_set() else ""))
    print(f"{names['A']} vs {names['B']}: +{wdl[0]} ={wdl[1]} -{wdl[2]}  Elo {e:+.1f} +/- {ci:.1f} (not the gate)")
    for k in ("A", "B"):
        t = totals[k]
        fails = ", ".join(f"{f} {t[f]}" for f in FAIL_KINDS)
        print(f"{names[k]:>12}: {fails} | ponderhits {t['hits']} (fast {t['fast_hits']}), misses {t['misses']}")
    print(f"Results: {out}")


if __name__ == "__main__":
    main()
