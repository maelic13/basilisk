"""The engine as a process, driven over UCI: lifecycle and protocol checks.

    python uci_process_check.py <engine> <case> [--fixtures DIR] [--expect-defect]

Each case drives a fresh engine process and decides whether the engine kept
its contract. Without --expect-defect a case passes (exit 0) when the
contract holds and fails (exit 1) when it does not. A case registered with
--expect-defect is a known defect waiting for its repair: it passes only when
it reproduces that defect, printing "KNOWN DEFECT REPRODUCED", and fails once
the behaviour is correct, so the repair's commit drops the flag. A harness
error (a missing fixture, an engine that will not start) fails either way.
"""

import argparse
import base64
import os
import pathlib
import queue
import random
import re
import shutil
import subprocess
import sys
import tempfile
import threading
import time

MOVE = re.compile(r"^[a-h][1-8][a-h][1-8][nbrq]?$")
ITALIAN = "r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 2 3"
MOVE_30 = "r2q1rk1/pp2bppp/2n1pn2/3p4/3P4/2NBPN2/PP3PPP/R2Q1RK1 w - - 0 30"
KQK = "8/8/8/4k3/8/8/8/K2Q4 w - - 0 1"


class HarnessError(Exception):
    pass


class Engine:
    """One engine process; stdout is read on a thread into a queue."""

    def __init__(self, exe, stdout=subprocess.PIPE):
        self.proc = subprocess.Popen([exe], stdin=subprocess.PIPE, stdout=stdout,
                                     stderr=subprocess.DEVNULL, bufsize=0)
        self.lines = queue.Queue()
        self.seen = []
        if stdout == subprocess.PIPE:
            threading.Thread(target=self._read, daemon=True).start()

    def _read(self):
        for raw in self.proc.stdout:
            self.lines.put(raw.decode("utf-8", "replace").rstrip("\r\n"))
        self.lines.put(None)

    def send(self, *commands):
        for command in commands:
            self.proc.stdin.write((command + "\n").encode("utf-8"))
        self.proc.stdin.flush()

    def read_until(self, predicate, timeout):
        """Lines up to and including the first that satisfies predicate.
        Returns (lines, matched); matched is False on timeout or end of output."""
        deadline = time.monotonic() + timeout
        got = []
        while True:
            left = deadline - time.monotonic()
            if left <= 0:
                return got, False
            try:
                line = self.lines.get(timeout=left)
            except queue.Empty:
                return got, False
            if line is None:
                self.lines.put(None)
                return got, False
            got.append(line)
            self.seen.append(line)
            if predicate(line):
                return got, True

    def search(self, go, timeout=60):
        """Sends `go ...` and returns its lines up to bestmove."""
        self.send(go)
        lines, ok = self.read_until(lambda l: l.startswith("bestmove"), timeout)
        if not ok:
            raise HarnessError(f"no bestmove for '{go}' within {timeout} s; tail {lines[-3:]}")
        return lines

    def ready(self, timeout=30):
        self.send("isready")
        lines, ok = self.read_until(lambda l: l == "readyok", timeout)
        if not ok:
            raise HarnessError(f"no readyok within {timeout} s")
        return lines

    def alive(self):
        return self.proc.poll() is None

    def close(self):
        if self.alive():
            try:
                self.send("quit")
            except OSError:
                pass
            try:
                self.proc.wait(10)
            except subprocess.TimeoutExpired:
                self.proc.kill()
                self.proc.wait()


def bestmove(lines):
    return lines[-1].split()[1]


def info_depths(lines):
    return [int(m.group(1)) for l in lines for m in [re.match(r"info depth (\d+) ", l)] if m]


def last_info(lines):
    infos = [l for l in lines if l.startswith("info depth ")]
    return infos[-1] if infos else ""


def tm_field(lines, name):
    for line in lines:
        m = re.search(rf"info string tm .*\b{name}=(-?\d+)", line)
        if m:
            return int(m.group(1))
    raise HarnessError(f"no TM_Debug {name} in {lines[-3:]}")


def fixture_dir(fixtures, names):
    """A temporary directory holding the named Syzygy fixture tables."""
    if not fixtures:
        raise HarnessError("this case needs --fixtures")
    target = pathlib.Path(tempfile.mkdtemp(prefix="basilisk-uci-tb-"))
    for name in names:
        encoded = (pathlib.Path(fixtures) / (name + ".b64")).read_text()
        (target / name).write_bytes(base64.b64decode("".join(encoded.split())))
    return target


# ---- Contracts -------------------------------------------------------------
# Each returns (holds, detail).

def case_depth_limit_threads(exe, args):
    """`go depth N` at Threads 4 reports depth N, never a helper's N+1."""
    positions = ["startpos", "fen " + ITALIAN, "fen " + MOVE_30]
    e = Engine(exe)
    try:
        e.send("setoption name Threads value 4")
        for trial in range(60):
            e.send("ucinewgame", "position " + positions[trial % 3])
            e.ready()
            lines = e.search("go depth 8")
            deepest = max(info_depths(lines))
            if deepest != 8:
                return False, f"trial {trial}: info depth {deepest} under go depth 8: {last_info(lines)[:90]}"
        return True, "60 searches, every one reported depth 8"
    finally:
        e.close()


def case_abort_sweep(exe, args):
    """A search stopped at any node count answers with the last completed
    iteration: its move is what a fresh `go depth d` plays, d being the last
    depth line it printed, and the same stop gives the same output."""
    for fen in ["startpos", "fen " + ITALIAN]:
        for nodes in [1, 2, 5, 10, 20, 40, 80, 150, 300, 600, 1200, 2500, 5000, 10000]:
            runs = []
            for _ in range(2):
                e = Engine(exe)
                try:
                    e.send("position " + fen)
                    runs.append(e.search(f"go nodes {nodes}"))
                finally:
                    e.close()
            strip = lambda ls: [re.sub(r" (nps|time) \d+", "", l) for l in ls]
            if strip(runs[0]) != strip(runs[1]):
                return False, f"{fen[:12]} nodes {nodes}: two runs differ"
            lines = runs[0]
            move = bestmove(lines)
            depth = max(info_depths([l for l in lines if "bound" not in l]) or [0])
            if depth == 0:
                return False, f"{fen[:12]} nodes {nodes}: bestmove {move} with no completed iteration"
            e = Engine(exe)
            try:
                e.send("position " + fen)
                reference = bestmove(e.search(f"go depth {depth}"))
            finally:
                e.close()
            if move != reference:
                return False, (f"{fen[:12]} nodes {nodes}: played {move} at reported depth {depth},"
                               f" where depth {depth} plays {reference}: {last_info(lines)[:90]}")
    return True, "every stopped search answered with a completed iteration"


def case_clock_zero(exe, args):
    """A `go` that names a clock is a clock search: on an empty, negative or
    unreadable clock it moves at once, under a hard limit."""
    for clock in ["wtime 0 btime 0", "wtime -1 btime -1", "wtime x btime x"]:
        e = Engine(exe)
        try:
            e.send("setoption name TM_Debug value true", "position fen " + MOVE_30)
            lines = e.search("go " + clock)
            hard = tm_field(lines, "hard_ms")
            if not 0 < hard <= 50:
                return False, f"'go {clock}': hard_ms={hard}, searched to depth {max(info_depths(lines))}"
        finally:
            e.close()
    return True, "every named clock bounded the search"


def case_bare_go(exe, args):
    """A bare `go` searches until `stop`, as Stockfish's does (decided in
    B.9.1): it does not answer at a default depth."""
    e = Engine(exe)
    try:
        e.send("position startpos", "go")
        lines, deep = e.read_until(lambda l: l.startswith("info depth 9 ") or l.startswith("bestmove"), 60)
        if not lines or lines[-1].startswith("bestmove"):
            return False, f"bare go answered unasked at depth {max(info_depths(lines) or [0])}"
        if not deep:
            raise HarnessError("bare go reached neither depth 9 nor bestmove in 60 s")
        e.send("stop")
        _, ok = e.read_until(lambda l: l.startswith("bestmove"), 10)
        return ok, "searched past depth 8 and answered the stop"
    finally:
        e.close()


def case_go_mate(exe, args):
    """`go mate N` searches until it finds a mate in N or less (Stockfish's
    contract; decided in B.9.1), not to a depth of 2N - 1."""
    e = Engine(exe)
    try:
        e.send("position fen 8/8/8/8/8/k7/8/1K1R1N2 w - - 0 1")
        lines = e.search("go mate 7", timeout=120)
        m = re.search(r" score mate (\d+) ", last_info(lines))
        if not m or not 1 <= int(m.group(1)) <= 7:
            return False, f"go mate 7 on a mate in 6 ended: {last_info(lines)[:90]}"
        return True, f"found: {last_info(lines)[:60]}"
    finally:
        e.close()


def case_hash_rounding_notice(exe, args):
    """A Hash the table cannot take exactly says what it uses instead."""
    e = Engine(exe)
    try:
        e.send("setoption name Hash value 128")
        # Only Hash notices count: a build below the CPU's best tier says so
        # at start-up (main.cpp), and that line is not about the table.
        quiet = [l for l in e.ready() if l.startswith("info string") and "Hash" in l]
        e.send("setoption name Hash value 100")
        told = [l for l in e.ready() if l.startswith("info string") and "Hash" in l]
        if quiet:
            raise HarnessError(f"Hash 128 is a power of two and needs no notice: {quiet}")
        if not any("Hash" in l and re.search(r"\b64\b", l) for l in told):
            return False, f"Hash 100 became a 64 MiB table without a word: {told}"
        return True, told[0]
    finally:
        e.close()


def case_syzygy_path_retry(exe, args):
    """Sending the same SyzygyPath again loads tables that have appeared."""
    tables = pathlib.Path(tempfile.mkdtemp(prefix="basilisk-uci-tb-"))
    source = fixture_dir(args.fixtures, ["KQvK.rtbw", "KQvK.rtbz"])
    e = Engine(exe)
    try:
        e.send(f"setoption name SyzygyPath value {tables}")
        first = e.ready()
        if not any("no tablebase files" in l for l in first):
            raise HarnessError(f"an empty directory was not reported: {first}")
        for f in source.iterdir():
            shutil.copy(f, tables / f.name)
        e.send(f"setoption name SyzygyPath value {tables}")
        again = e.ready()
        if not any("Found 1 WDL and 1 DTZ" in l for l in again):
            return False, f"the same path sent again after the files appeared: {again}"
        return True, "reloaded"
    finally:
        e.close()
        shutil.rmtree(tables, ignore_errors=True)
        shutil.rmtree(source, ignore_errors=True)


def case_fathom_unmappable_table(exe, args):
    """A table that cannot be mapped when first probed is skipped with a
    notice; the engine still answers and keeps running (Fathom exits)."""
    tables = fixture_dir(args.fixtures, ["KQvK.rtbw", "KQvK.rtbz"])
    e = Engine(exe)
    try:
        e.send(f"setoption name SyzygyPath value {tables}")
        if not any("Found 1 WDL and 1 DTZ" in l for l in e.ready()):
            raise HarnessError("the fixture tables did not load")
        for f in tables.iterdir():
            f.write_bytes(b"")  # truncated after loading: mapped lazily, at the first probe
        e.send("position fen " + KQK, "go depth 5")
        lines, ok = e.read_until(lambda l: l.startswith("bestmove"), 30)
        if not ok:
            try:
                e.proc.wait(10)
                fate = f"exited {e.proc.returncode}"
            except subprocess.TimeoutExpired:
                fate = "hung"
            return False, f"no bestmove; the process {fate}; tail {lines[-2:]}"
        e.ready()
        if not e.alive():
            return False, "answered, then the process ended"
        if not any(l.startswith("info string") and "skipped" in l for l in lines):
            return False, "answered without saying a table was skipped"
        return True, "answered, said so and kept running"
    finally:
        e.close()
        shutil.rmtree(tables, ignore_errors=True)


def case_syzygy_non_ascii_path(exe, args):
    """A SyzygyPath whose name is not ASCII loads (Windows passes UTF-8)."""
    source = fixture_dir(args.fixtures, ["KQvK.rtbw", "KQvK.rtbz"])
    tables = source.parent / (source.name + "-tabulky-čeština")
    source.rename(tables)
    e = Engine(exe)
    try:
        e.send(f"setoption name SyzygyPath value {tables}")
        lines = e.ready()
        if not any("Found 1 WDL and 1 DTZ" in l for l in lines):
            return False, f"{tables.name}: {lines}"
        return True, "loaded"
    finally:
        e.close()
        shutil.rmtree(tables, ignore_errors=True)


def case_ponderhit_game_ply(exe, args):
    """After `ponderhit` the budget is the one a plain `go` on the same clock
    gets: the game ply is the root's, not that of the node the search is at."""
    clock = "wtime 6000 btime 6000 winc 60 binc 60"
    e = Engine(exe)
    try:
        e.send("setoption name Ponder value true", "setoption name TM_Debug value true",
               "position fen " + MOVE_30)
        plain = tm_field(e.search("go " + clock), "soft_ms")
        e.send("go ponder " + clock)
        time.sleep(0.2)
        e.send("ponderhit")
        lines, ok = e.read_until(lambda l: l.startswith("bestmove"), 60)
        if not ok:
            raise HarnessError("no bestmove after ponderhit")
        hit = tm_field(lines, "soft_ms")
        return hit == plain, f"soft_ms plain go {plain}, after ponderhit {hit}"
    finally:
        e.close()


def _started(e, command, first):
    """Sends a long run and waits for its first record. A stop sent before
    the engine thread has taken the command up would only cancel it unrun."""
    e.send(command)
    if not e.read_until(lambda l: l.startswith(first), 120)[1]:
        raise HarnessError(f"'{command}' printed no '{first}' line within 120 s")


def _stopped_run(exe, command, first):
    e = Engine(exe)
    try:
        _started(e, command, first)
        e.send("stop", "position startpos", "go depth 1")
        lines, ok = e.read_until(lambda l: l.startswith("bestmove"), 5)
        if not ok:
            return False, f"'{command}' did not end within 5 s of stop"
        if not any("stop" in l.lower() for l in lines):
            return False, f"'{command}' ended without saying it was stopped"
        return True, "stopped promptly and said so"
    finally:
        e.proc.kill()
        e.proc.wait()


def case_bench_stop(exe, args):
    """`stop` during `bench` ends it promptly, and it says it was stopped."""
    return _stopped_run(exe, "bench 18", "bench 1/")


def case_wac_stop(exe, args):
    """`stop` during `wac` ends it promptly, and it says it was stopped."""
    return _stopped_run(exe, "wac 12", "wac record")


def case_bench_quit(exe, args):
    """`quit` during `bench` ends the process promptly."""
    e = Engine(exe)
    try:
        _started(e, "bench 18", "bench 1/")
        e.send("quit")
        try:
            e.proc.wait(5)
        except subprocess.TimeoutExpired:
            return False, "the process was still running 5 s after quit"
        return True, f"exited {e.proc.returncode}"
    finally:
        if e.alive():
            e.proc.kill()
            e.proc.wait()


def case_closed_stdout(exe, args):
    """An engine whose output is gone ends cleanly instead of searching on."""
    proc = subprocess.Popen([exe], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                            stderr=subprocess.DEVNULL, bufsize=0)
    try:
        proc.stdout.readline()  # the banner
        proc.stdout.close()
        proc.stdin.write(b"position startpos\ngo infinite\n")
        proc.stdin.flush()
        try:
            proc.wait(5)
        except subprocess.TimeoutExpired:
            return False, "still running 5 s after its stdout closed, stdin open"
        return True, f"exited {proc.returncode}"
    finally:
        if proc.poll() is None:
            proc.kill()
            proc.wait()


def case_seldepth_qsearch(exe, args):
    """seldepth counts quiescence plies, as Stockfish's does: a depth-1 search
    whose mate in 3 is proven through quiescence reports at least 5."""
    e = Engine(exe)
    try:
        e.send("position fen r5rk/5p1p/5R2/4B3/8/8/7P/7K w - - 0 1")
        line = last_info(e.search("go depth 1"))
        m = re.search(r" seldepth (\d+) .* score mate 3 ", line)
        if not m:
            raise HarnessError(f"the mate in 3 was not found at depth 1: {line}")
        return int(m.group(1)) >= 5, line[:70]
    finally:
        e.close()


def case_whitespace(exe, args):
    """Any whitespace separates tokens, leading and between, tabs included
    (the UCI specification's own example)."""
    e = Engine(exe)
    try:
        for command in ["  isready", "\tisready", " \t isready \t "]:
            e.send(command)
            _, ok = e.read_until(lambda l: l == "readyok", 3)
            if not ok:
                return False, f"{command!r} got no readyok"
        e.send("position\tstartpos \t moves\te2e4", "go\tdepth \t 2")
        lines, ok = e.read_until(lambda l: l.startswith("bestmove"), 10)
        if not ok:
            return False, "tab-separated position and go got no bestmove"
        if bestmove(lines)[1] not in "78":
            return False, f"tab-separated `position` was not applied: {lines[-1]}"
        return True, "accepted"
    finally:
        e.close()


# ---- Guards that hold on the head ------------------------------------------

def case_lifecycle_guards(exe, args):
    """Behaviour the audit read and found sound, pinned end to end."""
    # Input before any `position` searches the start position (TR-1).
    e = Engine(exe)
    try:
        if bestmove(e.search("go depth 3"))[1] not in "12":
            return False, "go before position did not play from the start position"
        # A trailing CR, repeated spaces and a 600-ply move list (TR-2).
        shuffle = " ".join(["g1f3", "g8f6", "f3g1", "f6g8"] * 150)
        e.send("position   startpos  moves  " + shuffle + "\r")
        if not MOVE.match(bestmove(e.search("go depth 2\r"))):
            return False, "a long, CR-terminated position got no move"
    finally:
        e.close()

    # `go infinite` on a decided root waits for stop, at Threads 4 too.
    for fen, score in [("7k/5Q2/6K1/8/8/8/8/8 b - - 0 1", "cp 0"), ("7k/6Q1/6K1/8/8/8/8/8 b - - 0 1", "mate 0")]:
        e = Engine(exe)
        try:
            e.send("setoption name Threads value 4", "position fen " + fen)
            lines = e.search("go depth 5")
            if lines[-1] != "bestmove 0000" or not any(f"score {score}" in l for l in lines):
                return False, f"terminal root {fen}: {lines}"
            e.send("go infinite")
            early, answered = e.read_until(lambda l: l.startswith("bestmove"), 0.5)
            if answered:
                return False, f"go infinite on a terminal root answered before stop: {early}"
            e.send("stop")
            if not e.read_until(lambda l: l == "bestmove 0000", 5)[1]:
                return False, "terminal root: no bestmove 0000 after stop"
        finally:
            e.close()

    # `setoption` during a search waits for it, and `stop` is still prompt.
    e = Engine(exe)
    try:
        e.send("position startpos", "go infinite")
        time.sleep(0.3)
        e.send("setoption name MultiPV value 3")
        t0 = time.monotonic()
        e.send("stop")
        if not e.read_until(lambda l: l.startswith("bestmove"), 2)[1]:
            return False, "stop after a mid-search setoption was not answered within 2 s"
        lines = e.search("go depth 6")
        if not any(" multipv 3 " in l for l in lines):
            return False, "the MultiPV set during the search did not apply to the next one"
    finally:
        e.close()

    # The end of input during `go infinite` and during a ponder search ends
    # the process cleanly after its bestmove.
    for go in ["go infinite", "go ponder wtime 10000 btime 10000"]:
        e = Engine(exe)
        try:
            e.send("setoption name Ponder value true", "position startpos", go)
            time.sleep(0.3)
            e.proc.stdin.close()
            lines, ok = e.read_until(lambda l: l.startswith("bestmove"), 10)
            e.proc.wait(10)
            if not ok or e.proc.returncode != 0:
                return False, f"end of input during '{go}': bestmove {ok}, exit {e.proc.returncode}"
        finally:
            if e.alive():
                e.proc.kill()
                e.proc.wait()

    # A rejected `position` during a search: the search answers its stop,
    # then the engine exits as decided on 2026-09-27.
    e = Engine(exe)
    try:
        e.send("position startpos", "go infinite")
        time.sleep(0.3)
        e.send("position startpos moves e2e5")
        time.sleep(0.3)
        if not e.alive():
            return False, "a position queued behind a search ended the search"
        e.send("stop")
        lines, ok = e.read_until(lambda l: l.startswith("bestmove"), 5)
        e.proc.wait(10)
        if not ok or e.proc.returncode != 1:
            return False, f"rejected position mid-search: bestmove {ok}, exit {e.proc.returncode}"
    finally:
        if e.alive():
            e.proc.kill()
            e.proc.wait()

    # `go nodes` counts every thread (TP-5): the lines print the pool's
    # total, which a limit counted on one thread would let pass far beyond it.
    e = Engine(exe)
    try:
        e.send("setoption name Threads value 4", "position startpos")
        lines = e.search("go nodes 400000")
        counts = [int(m.group(1)) for l in lines for m in [re.search(r" nodes (\d+)", l)] if m]
        if not counts or max(counts) > 440000:
            return False, f"go nodes 400000 at Threads 4 printed up to {max(counts or [0])} nodes"
    finally:
        e.close()

    # 1.2.3's two illegal moves (A-69): replies to a check by a double push,
    # answered by en passant.
    games = [
        ("8/4Q3/3R4/5k2/6Pp/4P2P/5K2/r7 b - g3 0 51", "h4g3"),
        ("3r4/5R1R/2n3p1/2Pp2k1/3PpP1p/4P2P/3N2P1/6K1 b - f3 0 57", "e4f3"),
    ]
    for fen, only in games:
        for threads in (1, 4):
            e = Engine(exe)
            try:
                e.send(f"setoption name Threads value {threads}", "position fen " + fen)
                move = bestmove(e.search("go depth 10"))
                if move != only:
                    return False, f"{fen}: played {move}, the only legal move is {only}"
            finally:
                e.close()
    return True, "all guards hold"


def case_new_game_resets(exe, args):
    """`ucinewgame` leaves nothing behind: after other searches (another
    position, four threads, MultiPV 3) a new game's search is node for node a
    fresh engine's."""
    probe = ["position fen " + ITALIAN, "go depth 12"]
    other = ["position fen 8/5pk1/6p1/8/3R4/6P1/5PK1/1r6 w - - 0 40"]

    def nodes_of(commands):
        e = Engine(exe)
        try:
            lines = []
            for c in commands:
                if c.startswith("go"):
                    lines = e.search(c)
                else:
                    e.send(c)
            return re.search(r" nodes (\d+)", [l for l in lines if l.startswith("info depth 12 ")][-1]).group(1)
        finally:
            e.close()

    fresh = nodes_of(probe)
    histories = {
        "another search": other + ["go depth 12", "ucinewgame"],
        "four threads": ["setoption name Threads value 4"] + other + ["go depth 12",
                         "setoption name Threads value 1", "ucinewgame"],
        "MultiPV 3": ["setoption name MultiPV value 3"] + other + ["go depth 12",
                      "setoption name MultiPV value 1", "ucinewgame"],
    }
    for name, before in histories.items():
        got = nodes_of(before + probe)
        if got != fresh:
            return False, f"after {name} and ucinewgame: {got} nodes, a fresh engine {fresh}"
    return True, f"{fresh} nodes every time"


def case_multipv_stopped(exe, args):
    """A stopped MultiPV search plays the first move of its last line 1, and
    every line it printed is legal (D-73)."""
    for nodes in [3000, 20000, 60000, 150000, 400000]:
        e = Engine(exe)
        try:
            e.send("setoption name MultiPV value 3", "position fen " + ITALIAN)
            lines = e.search(f"go nodes {nodes}")
            ones = [l for l in lines if " multipv 1 " in l and " pv " in l]
            if not ones or ones[-1].split(" pv ")[1].split()[0] != bestmove(lines):
                return False, f"nodes {nodes}: bestmove {bestmove(lines)} does not head the last line 1"
        finally:
            e.close()
    return True, "bestmove heads the last line 1 at every stop"


def case_tablebase_threads(exe, args):
    """An untimed four-thread search whose line enters the tables (KQ against
    KR, the rook capturable) answers legally; under ThreadSanitizer it also
    runs the root's DTZ probes beside the helpers' WDL probes."""
    tables = fixture_dir(args.fixtures, ["KQvK.rtbw", "KQvK.rtbz"])
    e = Engine(exe)
    try:
        e.send(f"setoption name SyzygyPath value {tables}", "setoption name Threads value 4")
        e.ready()
        e.send("position fen 8/8/8/3k4/8/8/2r5/KQ6 w - - 0 1", "go infinite")
        time.sleep(1.0)
        e.send("stop")
        lines, ok = e.read_until(lambda l: l.startswith("bestmove"), 10)
        if not ok or bestmove(lines) != "b1c2":
            return False, f"KQ v KR: {lines[-1] if lines else 'no output'}"
        return True, "b1c2"
    finally:
        e.close()
        shutil.rmtree(tables, ignore_errors=True)


def case_fuzz(exe, args):
    """Unknown, malformed and out-of-place commands never crash the engine or
    cost a `go` its single bestmove (A-67). Seeded."""
    rng = random.Random(20261116)
    # No `go` whose limit is missing or unreadable: what such a `go` means is
    # case_bare_go's and case_clock_zero's.
    garbage = ["", "xyzzy", "go depth -3",
               "setoption", "setoption name", "setoption name Hash", "setoption name Hash value x",
               "setoption name Threads value 0", "setoption name MultiPV value 999",
               "ponderhit", "stop", "debug", "debug on", "debug off", "register later",
               "go searchmoves a1a1 depth 2", "go searchmoves e2e4 nodes 50", "\x00\x01", "uci"]
    e = Engine(exe)
    try:
        fens = ["startpos", "fen " + ITALIAN, "fen " + MOVE_30, "fen " + KQK]
        expected = 0
        for step in range(300):
            roll = rng.random()
            if roll < 0.45:
                command = rng.choice(garbage)
            elif roll < 0.6:
                command = "position " + rng.choice(fens)
            else:
                command = rng.choice(["go depth 1", "go depth 2", "go nodes 200", "go movetime 5"])
            e.send(command)
            expected += command.startswith("go")
        count = lambda: sum(1 for l in e.seen if l.startswith("bestmove"))
        e.read_until(lambda l: l.startswith("bestmove") and count() >= expected, 120)
        if not e.alive():
            return False, f"the engine died during the fuzz, exit {e.proc.returncode}"
        e.ready()
        if count() != expected:
            return False, f"{expected} go commands, {count()} bestmoves"
        moves = [l.split()[1] for l in e.seen if l.startswith("bestmove")]
        if any(not MOVE.match(m) and m != "0000" for m in moves):
            return False, "a malformed bestmove"
        return True, f"{expected} searches answered, {moves.count('0000')} of them 0000"
    finally:
        e.close()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("engine")
    parser.add_argument("case")
    parser.add_argument("--fixtures")
    parser.add_argument("--expect-defect", action="store_true")
    args = parser.parse_args()
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    check = globals().get("case_" + args.case)
    if check is None:
        print(f"no case {args.case}")
        return 2
    try:
        holds, detail = check(args.engine, args)
    except HarnessError as error:
        print(f"HARNESS ERROR in {args.case}: {error}")
        return 2
    if args.expect_defect:
        if holds:
            print(f"DEFECT GONE in {args.case}: {detail}. Drop --expect-defect for it.")
            return 1
        print(f"KNOWN DEFECT REPRODUCED: {args.case}: {detail}")
        return 0
    print(f"{'ok' if holds else 'FAILED'}: {args.case}: {detail}")
    return 0 if holds else 1


if __name__ == "__main__":
    sys.exit(main())
