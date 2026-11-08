"""Multi-thread read instruments for a Colosseum run directory.

    python tools/diag/smp_read.py sides RUN_DIR [--fifths 5] [--json OUT]
    python tools/diag/smp_read.py placement RUN_DIR SAMPLER_DIR [--hot 60] [--json OUT]

`sides` reads every move comment `{s=.. d=.. t=..ms h=..ms n=..}` and gives
each arm (`a`, `b`) its moves, its mate-score moves, its mean depth over all
moves and over non-mate moves, and its NPS = sum n / sum (t - h) over moves
with t > h. R = NPS(b) / NPS(a), over the run and over each fifth of its
games in completion order, each fifth with its UTC window for the sampler.
In a self-relative read (arm B at four threads, arm A at one) R is the
in-game thread speedup, the validity check of the leg: an engine packed on
SMT siblings reads about 2.6 at four threads where it reads 4 sibling-free
(Rarog RAR-M82). Mean depth over all moves is not search depth: in decided
positions a side's mate-score moves stop at depths unrelated to the rest
(Rarog's D.2 audit, 2026-11-02), so the non-mate column is the one to read.

`placement` reads the host sampler's `cpu-*.csv` (tools/host_sampler.ps1)
over the run's window and, per Colosseum slot, the physical cores the slot
uses with all their logical CPUs (`topology.json` from the sampler): each
logical CPU's mean busy share, and on the seconds when at least as many of
those logical CPUs as the slot has cores minus one are above --hot percent
(a multi-thread turn covering most of the second), how many distinct
physical cores the hot ones span. One per core means the threads were
spread; fewer cores than threads means they were packed on siblings.
"""
import argparse
import csv
import hashlib
import json
import re
import sys
from collections import Counter, defaultdict
from datetime import datetime, timezone
from pathlib import Path

COMMENT = re.compile(r"\{s=(\S+) d=(\d+) t=(\d+)ms h=(\d+)ms n=(\d+)\}")
TOKEN = re.compile(r"\{[^}]*\}|[^\s{}]+")
MOVE_NUMBER = re.compile(r"^\d+\.(\.\.)?$")
RESULTS = {"1-0", "0-1", "1/2-1/2", "*"}
FEN = re.compile(r'^\[FEN "([^"]*)"\]', re.M)
CPU_COLUMN = re.compile(r"Processor Information\((\d+),(\d+)\)\\% Processor Time$")


def load_games(run):
    """(record, text) per game in completion order, each text hash-checked."""
    raw = (run / "games.pgn").read_bytes()
    records = [json.loads(line) for line in
               (run / "games.jsonl").read_text(encoding="utf-8").splitlines() if line.strip()]
    records.sort(key=lambda r: r["game"]["slot"]["ended_unix_us"])
    out = []
    for r in records:
        off, length = r["pgn"]["offset"], r["pgn"]["length"]
        chunk = raw[off:off + length]
        if hashlib.sha256(chunk).hexdigest() != r["pgn"]["sha256"]:
            raise SystemExit(f"{run}: game {r['game']['number']}'s PGN text does not match its recorded SHA-256")
        out.append((r, chunk.decode("utf-8")))
    return out


def game_moves(text, white_arm):
    """Yield (arm, s, d, t, h, n) for every commented move of one game."""
    fen = FEN.search(text)
    white_to_move = not (fen and fen.group(1).split()[1:2] == ["b"])
    body = "\n".join(line for line in text.splitlines() if not line.startswith("["))
    black_arm = "b" if white_arm == "a" else "a"
    mover = None
    for tok in TOKEN.findall(body):
        if tok.startswith("{"):
            m = COMMENT.match(tok)
            if m and mover is not None:
                s, d, t, h, n = m.groups()
                yield mover, s, int(d), int(t), int(h), int(n)
            continue
        if MOVE_NUMBER.match(tok) or tok in RESULTS:
            continue
        tok = re.sub(r"^\d+\.(\.\.)?", "", tok)
        if not tok:
            continue
        mover = white_arm if white_to_move else black_arm
        white_to_move = not white_to_move


def is_mate(score):
    return not re.fullmatch(r"-?\d+", score)


def side_stats(games):
    acc = {arm: dict(moves=0, mate=0, d_all=0, d_rest=0, rest=0, nodes=0, us=0) for arm in "ab"}
    for record, text in games:
        for arm, s, d, t, h, n in game_moves(text, record["game"]["white"]):
            a = acc[arm]
            a["moves"] += 1
            a["d_all"] += d
            if is_mate(s):
                a["mate"] += 1
            else:
                a["rest"] += 1
                a["d_rest"] += d
            if t > h:
                a["nodes"] += n
                a["us"] += 1000 * (t - h)
    out = {}
    for arm, a in acc.items():
        out[arm] = {
            "moves": a["moves"], "mate_score_moves": a["mate"],
            "depth_all": a["d_all"] / a["moves"] if a["moves"] else None,
            "depth_non_mate": a["d_rest"] / a["rest"] if a["rest"] else None,
            "nps": 1e6 * a["nodes"] / a["us"] if a["us"] else None,
        }
    na, nb = out["a"]["nps"], out["b"]["nps"]
    out["r"] = nb / na if na and nb else None
    return out


def utc(us):
    return datetime.fromtimestamp(us / 1e6, tz=timezone.utc).strftime("%Y-%m-%d %H:%M:%S")


def sides(args):
    run = Path(args.run)
    games = load_games(run)
    if not games:
        raise SystemExit(f"{run}: no games")
    report = {"run": str(run), "games": len(games), "whole": side_stats(games), "fifths": []}
    k = max(1, args.fifths)
    for i in range(k):
        part = games[i * len(games) // k:(i + 1) * len(games) // k]
        if not part:
            continue
        slots = [r["game"]["slot"] for r, _ in part]
        stats = side_stats(part)
        stats.update(games=len(part), first_utc=utc(min(s["started_unix_us"] for s in slots)),
                     last_utc=utc(max(s["ended_unix_us"] for s in slots)))
        report["fifths"].append(stats)
    w = report["whole"]
    print(f"{run}: {len(games)} games")
    for arm in "ab":
        s = w[arm]
        print(f"  arm {arm}: {s['moves']} moves ({s['mate_score_moves']} mate-score), depth all "
              f"{s['depth_all']:.2f}, non-mate {s['depth_non_mate']:.2f}, NPS {s['nps'] / 1e6:.3f}M")
    print(f"  R = NPS(b) / NPS(a) = {w['r']:.3f}")
    for i, f in enumerate(report["fifths"], 1):
        print(f"  part {i}: {f['games']} games, {f['first_utc']} to {f['last_utc']} UTC, R {f['r']:.3f}, "
              f"non-mate depth a {f['a']['depth_non_mate']:.2f} b {f['b']['depth_non_mate']:.2f}")
    if args.json:
        Path(args.json).write_text(json.dumps(report, indent=2), encoding="utf-8")


def read_cpu_csvs(sampler, start, end):
    """Yield {logical cpu: busy %} per one-second sample inside [start, end] (UTC seconds)."""
    for path in sorted(sampler.glob("cpu-*.csv")):
        with open(path, newline="", encoding="utf-8", errors="replace") as f:
            reader = csv.reader(f)
            head = next(reader, None)
            if not head:
                continue
            columns = {}
            for i, h in enumerate(head):
                m = CPU_COLUMN.search(h)
                if m:
                    columns[64 * int(m.group(1)) + int(m.group(2))] = i
            for row in reader:
                try:
                    stamp = datetime.strptime(row[0], "%m/%d/%Y %H:%M:%S.%f").astimezone(timezone.utc).timestamp()
                except (ValueError, IndexError):
                    continue
                if not start <= stamp <= end:
                    continue
                sample = {}
                for cpu, i in columns.items():
                    try:
                        v = float(row[i])
                    except (ValueError, IndexError):
                        continue
                    if 0 <= v <= 101:
                        sample[cpu] = v
                yield sample


def placement(args):
    run, sampler = Path(args.run), Path(args.sampler)
    topology = json.loads((sampler / "topology.json").read_text(encoding="utf-8"))
    core_of = {cpu: i for i, core in enumerate(topology["cores"]) for cpu in core}
    resolved = json.loads((run / "resolved-config.json").read_text(encoding="utf-8"))
    slots = {}
    for slot in resolved["execution"]["slots"]:
        listed = [64 * c["group"] + c["number"] for c in slot["engine_a"]["allocation"]["cpus"]]
        cores = sorted({core_of[c] for c in listed})
        slots[slot["slot_index"]] = (listed, sorted(c for i in cores for c in topology["cores"][i]))
    records = [json.loads(line)["game"]["slot"] for line in
               (run / "games.jsonl").read_text(encoding="utf-8").splitlines() if line.strip()]
    start = min(r["started_unix_us"] for r in records) / 1e6
    end = max(r["ended_unix_us"] for r in records) / 1e6
    busy = defaultdict(lambda: defaultdict(float))
    seconds = Counter()
    spans = defaultdict(Counter)
    for sample in read_cpu_csvs(sampler, start, end):
        for index, (listed, lps) in slots.items():
            if not all(c in sample for c in lps):
                continue
            seconds[index] += 1
            for c in lps:
                busy[index][c] += sample[c]
            hot = [c for c in lps if sample[c] > args.hot]
            if len(hot) >= max(1, len({core_of[c] for c in listed}) - 1):
                spans[index][len({core_of[c] for c in hot})] += 1
    report = {"run": str(run), "sampler": str(sampler), "window_utc": [utc(start * 1e6), utc(end * 1e6)], "slots": []}
    print(f"{run}: sampler {sampler}, {report['window_utc'][0]} to {report['window_utc'][1]} UTC")
    for index in sorted(slots):
        listed, lps = slots[index]
        n = seconds[index]
        if not n:
            print(f"  slot {index}: no complete sample in the window")
            continue
        means = {c: busy[index][c] / n for c in lps}
        hist = spans[index]
        total = sum(hist.values())
        report["slots"].append({"slot": index, "listed": listed, "seconds": n,
                                "mean_busy": {str(c): round(v, 1) for c, v in means.items()},
                                "turn_seconds": total, "cores_spanned": {str(k): v for k, v in sorted(hist.items())}})
        per = " ".join(f"{c}{'*' if c in listed else ''}:{means[c]:.0f}" for c in lps)
        dist = " ".join(f"{k} cores {100 * v / total:.1f}%" for k, v in sorted(hist.items())) if total else "-"
        print(f"  slot {index}: {n} s | {per}")
        print(f"          multi-thread-turn seconds {total}: {dist}")
    print("  (* a listed logical CPU; the others are its cores' SMT siblings)")
    if args.json:
        Path(args.json).write_text(json.dumps(report, indent=2), encoding="utf-8")


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    sub = parser.add_subparsers(dest="command", required=True)
    s = sub.add_parser("sides")
    s.add_argument("run")
    s.add_argument("--fifths", type=int, default=5)
    s.add_argument("--json")
    p = sub.add_parser("placement")
    p.add_argument("run")
    p.add_argument("sampler")
    p.add_argument("--hot", type=float, default=60.0)
    p.add_argument("--json")
    args = parser.parse_args(argv)
    {"sides": sides, "placement": placement}[args.command](args)


if __name__ == "__main__":
    sys.exit(main())
