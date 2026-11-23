#!/usr/bin/env python3
"""Corpus B: a basilisk-texel-rows-v1 corpus relabelled row by row by Syzygy.

PLAN C.2.4 (archived 8.4). Corpus B is corpus A row for row: the same files,
the same rows in the same order, every column the same except ``result``,
which changes only on an eligible row. The label contract is
``basilisk-texel-labels-syzygy-v1``:

* **Eligible:** a row with at most six men and no castling right, whose WDL
  and DTZ tables both probe. Every other row keeps A's game-result label, and
  a probe that fails is counted as unknown, never as agreement.
* **The verdict, in Stockfish's form** (``tbprobe.cpp`` root_probe, the
  ``r >= bound`` test with the fifty-move rule on): with ``dtz`` the side to
  move's distance to zeroing and ``clock`` the row's rule-50 clock, the row
  is a win when ``dtz > 0`` and ``dtz + clock <= 100``, a loss when
  ``dtz < 0`` and ``-dtz + clock <= 100``, and a draw otherwise. A cursed win
  or blessed loss is a draw, and so is a clean one the clock has run out on.
* **Row-local:** a verdict labels its own row only. Nothing is propagated
  back into the game's larger positions (8.4.e); that is whole-game
  adjudication, a different change, which ``games`` measures and does not
  build.

Subcommands:

``build A_DIR B_DIR --syzygy PATH``
    Write B's three splits and ``manifest.json`` (derived from A's manifest,
    hashed), or nothing; then run ``check``.
``check A_DIR B_DIR``
    Prove B row-identical to A except eligible labels, and both manifests'
    hashes true.
``games A_DIR --syzygy PATH --out JSON``
    For every game of A's input PGNs, the verdict at the first position with
    six men or fewer against the game's result, and how many of A's rows a
    whole-game relabel would change beyond B's.
"""

from __future__ import annotations

import argparse
import collections
import datetime
import json
import multiprocessing
import sys
from pathlib import Path

import chess
import chess.pgn
import chess.syzygy

sys.path.insert(0, str(Path(__file__).resolve().parent))
import extract  # noqa: E402

LABEL_CONTRACT = "basilisk-texel-labels-syzygy-v1"
MAX_MEN = 6
SPLITS = extract.SPLITS
LABEL_TEXT = {1.0: "1", 0.5: "0.5", 0.0: "0"}
TEXT_LABEL = {"1": 1.0, "0.5": 0.5, "0": 0.0}
RESULT_POINTS = {"1-0": 1.0, "0-1": 0.0, "1/2-1/2": 0.5}

_tb = None


def material_key(board: chess.Board) -> str:
    """`KRP-KR` style signature, the side with more pieces first."""
    def side(color):
        return "K" + "".join(
            chess.piece_symbol(piece).upper() * len(board.pieces(piece, color))
            for piece in (chess.QUEEN, chess.ROOK, chess.BISHOP, chess.KNIGHT, chess.PAWN))
    white, black = side(chess.WHITE), side(chess.BLACK)
    return f"{white}-{black}" if len(white) >= len(black) else f"{black}-{white}"


def men(placement: str) -> int:
    return sum(ch.isalpha() for ch in placement)


def verdict(board: chess.Board, tb) -> tuple[float | None, str, int]:
    """White's points under the rule-50 clock, why, and the DTZ margin.

    Returns ``(points, kind, margin)``. ``points`` is None for a row that is
    not eligible. ``kind`` is one of ``too_many_men``, ``castling``,
    ``missing``, ``win``, ``loss``, ``draw``, ``cursed`` (a WDL of one) or
    ``clock`` (a clean result the clock turns into a draw). ``margin`` is
    ``|dtz| + clock - 100`` for a decided WDL, else 0.
    """
    if chess.popcount(board.occupied) > MAX_MEN:
        return None, "too_many_men", 0
    if board.castling_rights:
        return None, "castling", 0
    try:
        wdl = tb.probe_wdl(board)
        dtz = tb.probe_dtz(board) if wdl else 0
    except (chess.syzygy.MissingTableError, KeyError, ValueError):
        return None, "missing", 0
    clock = board.halfmove_clock
    if wdl == 0:
        return 0.5, "draw", 0
    margin = abs(dtz) + clock - 100
    if margin > 0:
        kind = "cursed" if abs(wdl) == 1 else "clock"
        return 0.5, kind, margin
    if abs(wdl) == 1:
        # A WDL of one is a result the fifty-move rule already draws; its DTZ
        # is over 100 by definition, so this branch is a table inconsistency.
        return 0.5, "cursed", margin
    stm_wins = dtz > 0
    white_wins = stm_wins == (board.turn == chess.WHITE)
    return (1.0 if white_wins else 0.0), ("win" if stm_wins else "loss"), margin


# ---- build ----------------------------------------------------------------

def _init(syzygy: str) -> None:
    global _tb
    _tb = chess.syzygy.open_tablebase(syzygy)


def _relabel_chunk(lines: list[str]):
    """New result text for each line, and the chunk's statistics."""
    stats = collections.Counter()
    matrix = collections.Counter()
    families = collections.defaultdict(collections.Counter)
    phases = collections.defaultdict(collections.Counter)
    margins = collections.Counter()
    out = []
    for line in lines:
        fen, result, _gid, _sid, _ply, phase = line.rstrip("\n").split(",")
        if men(fen.split(" ", 1)[0]) > MAX_MEN:
            stats["too_many_men"] += 1
            out.append(result)
            continue
        board = chess.Board(fen)
        points, kind, margin = verdict(board, _tb)
        stats[kind] += 1
        if points is None:
            out.append(result)
            continue
        stats["eligible"] += 1
        new = LABEL_TEXT[points]
        matrix[f"{result}->{new}"] += 1
        if -1 <= margin <= 1 and kind in ("win", "loss", "clock"):
            margins[str(margin)] += 1
        family = material_key(board)
        families[family]["eligible"] += 1
        phases[phase]["eligible"] += 1
        if new != result:
            stats["changed"] += 1
            families[family]["changed"] += 1
            phases[phase]["changed"] += 1
        out.append(new)
    return out, stats, matrix, families, phases, margins


def _chunks(path: Path, size: int):
    with path.open(encoding="utf-8", newline="") as source:
        head = [source.readline(), source.readline()]
        yield head
        chunk = []
        for line in source:
            chunk.append(line)
            if len(chunk) == size:
                yield chunk
                chunk = []
        if chunk:
            yield chunk


def _merge(total: dict, part: dict) -> None:
    for key, value in part.items():
        total[key].update(value)


def build(a_dir: Path, b_dir: Path, syzygy: str, jobs: int) -> int:
    a_manifest_path = a_dir / "manifest.json"
    a_manifest = json.loads(a_manifest_path.read_text(encoding="utf-8"))
    if a_manifest.get("schema") != extract.SCHEMA:
        raise SystemExit(f"{a_manifest_path} is not a {extract.SCHEMA} manifest")
    if "label_contract" in a_manifest:
        raise SystemExit(f"{a_dir} is already relabelled; derive B from a game-result corpus")
    targets = {split: b_dir / a_manifest["output"][split]["file"] for split in SPLITS}
    manifest_path = b_dir / "manifest.json"
    for target in (*targets.values(), manifest_path):
        if target.exists():
            raise SystemExit(f"refusing to overwrite a published dataset file: {target}")
    for split in SPLITS:
        source = a_dir / a_manifest["output"][split]["file"]
        actual = extract.sha256_file(source)
        if actual != a_manifest["output"][split]["sha256"]:
            raise SystemExit(f"{source} hashes {actual}, its manifest says otherwise")
    b_dir.mkdir(parents=True, exist_ok=True)

    report = {}
    output = {}
    staged = []
    with multiprocessing.Pool(jobs, initializer=_init, initargs=(syzygy,)) as pool:
        for split in SPLITS:
            source = a_dir / a_manifest["output"][split]["file"]
            chunks = _chunks(source, 20000)
            head = next(chunks)
            if head[0].strip() != f"# {extract.SCHEMA} {split}" or \
                    head[1].strip() != ",".join(extract.COLUMNS):
                raise SystemExit(f"{source}: not a {extract.SCHEMA} {split} file")
            stats = collections.Counter()
            matrix = collections.Counter()
            families = collections.defaultdict(collections.Counter)
            phases = collections.defaultdict(collections.Counter)
            margins = collections.Counter()
            temporary = targets[split].with_name(targets[split].name + ".tmp")
            rows = 0
            with temporary.open("w", encoding="utf-8", newline="\n") as out, \
                    source.open(encoding="utf-8", newline="") as again:
                out.write(again.readline())
                out.write(again.readline())
                for labels, s, m, f, p, g in pool.imap(_relabel_chunk, chunks):
                    stats.update(s)
                    matrix.update(m)
                    _merge(families, f)
                    _merge(phases, p)
                    margins.update(g)
                    for new in labels:
                        fen, _old, rest = again.readline().split(",", 2)
                        out.write(f"{fen},{new},{rest}")
                        rows += 1
                if again.readline():
                    raise SystemExit(f"{source}: the chunks did not cover the file")
            if rows != a_manifest["output"][split]["rows"]:
                raise SystemExit(f"{split}: wrote {rows} rows, A has "
                                 f"{a_manifest['output'][split]['rows']}")
            staged.append((temporary, targets[split]))
            output[split] = {"file": targets[split].name, "rows": rows,
                             "sha256": extract.sha256_file(temporary)}
            changed = sorted(((k, v) for k, v in families.items() if v["changed"]),
                             key=lambda kv: -kv[1]["changed"])
            report[split] = {
                "rows": rows,
                "counts": dict(stats),
                "label_matrix": dict(sorted(matrix.items())),
                "by_phase": {k: dict(v) for k, v in sorted(phases.items(), key=lambda kv: int(kv[0]))},
                "families_changed_top": {k: dict(v) for k, v in changed[:25]},
                "families_eligible": len(families),
                "dtz_margin_within_one": dict(margins),
            }
            print(f"{split}: {rows:,} rows, eligible {stats['eligible']:,}, "
                  f"changed {stats['changed']:,}, unknown castling {stats['castling']:,} "
                  f"missing {stats['missing']:,}")

    manifest = {key: a_manifest[key] for key in
                ("schema", "columns", "split_rule", "dedup_fields", "seed",
                 "parameters", "games_read", "games_by_split", "games_rejected",
                 "positions", "phase_quotas", "inputs")}
    manifest.update({
        "label": "the game's result from White's side, except that a row with at "
                 "most six men and no castling right takes Syzygy's verdict under "
                 "its rule-50 clock; exactly 1, 0.5 or 0",
        "label_contract": LABEL_CONTRACT,
        "verdict_rule": "Stockfish root_probe with the fifty-move rule: win when "
                        "dtz > 0 and dtz + clock <= 100, loss when dtz < 0 and "
                        "-dtz + clock <= 100, otherwise a draw",
        "derived_from": {"dir": str(a_dir.resolve()),
                         "manifest_sha256": extract.sha256_file(a_manifest_path),
                         "output": a_manifest["output"]},
        "tool": "tools/texel/relabel.py",
        "source_revision": extract.source_revision(),
        "syzygy": str(syzygy),
        "relabel": report,
        "built_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(timespec="seconds"),
        "output": output,
    })
    for temporary, target in staged:
        temporary.replace(target)
    manifest_tmp = manifest_path.with_name("manifest.json.tmp")
    manifest_tmp.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8", newline="\n")
    manifest_tmp.replace(manifest_path)
    print(f"Published corpus B under {b_dir}")
    return check(a_dir, b_dir, syzygy)


# ---- check ----------------------------------------------------------------

def check(a_dir: Path, b_dir: Path, syzygy: str | None = None, sample: int = 2000) -> int:
    """B is A except eligible labels; manifests true; a sample re-probed."""
    a_manifest = json.loads((a_dir / "manifest.json").read_text(encoding="utf-8"))
    b_manifest = json.loads((b_dir / "manifest.json").read_text(encoding="utf-8"))
    problems = []
    if b_manifest.get("label_contract") != LABEL_CONTRACT:
        problems.append("B's manifest names no syzygy label contract")
    if b_manifest["derived_from"]["manifest_sha256"] != extract.sha256_file(a_dir / "manifest.json"):
        problems.append("B was derived from a different A manifest")
    tb = chess.syzygy.open_tablebase(syzygy) if syzygy else None
    for split in SPLITS:
        a_path = a_dir / a_manifest["output"][split]["file"]
        b_path = b_dir / b_manifest["output"][split]["file"]
        for path, manifest in ((a_path, a_manifest), (b_path, b_manifest)):
            if extract.sha256_file(path) != manifest["output"][split]["sha256"]:
                problems.append(f"{path} does not hash as its manifest says")
        rows = changed = probed = 0
        before = len(problems)
        with a_path.open(encoding="utf-8", newline="") as a, \
                b_path.open(encoding="utf-8", newline="") as b:
            for index, (la, lb) in enumerate(zip(a, b)):
                if index < 2:
                    if la != lb:
                        problems.append(f"{split}: header line {index + 1} differs")
                    continue
                rows += 1
                fa, ra, rest_a = la.split(",", 2)
                fb, rb, rest_b = lb.split(",", 2)
                if fa != fb or rest_a != rest_b:
                    problems.append(f"{split} row {rows}: a column other than result differs")
                    break
                if rb not in TEXT_LABEL:
                    problems.append(f"{split} row {rows}: label {rb!r}")
                    break
                eligible_shape = men(fa.split(" ", 1)[0]) <= MAX_MEN
                if ra != rb:
                    changed += 1
                    if not eligible_shape:
                        problems.append(f"{split} row {rows}: a >6-man label changed")
                        break
                if tb is not None and eligible_shape and probed < sample and (rows - 1) % 7 == 0:
                    points, _kind, _margin = verdict(chess.Board(fa), tb)
                    expect = ra if points is None else LABEL_TEXT[points]
                    probed += 1
                    if expect != rb:
                        problems.append(f"{split} row {rows}: re-probe gives {expect}, B has {rb}")
                        break
            if len(problems) == before and (a.readline() or b.readline()):
                problems.append(f"{split}: the files differ in length")
        if len(problems) > before:
            print(f"check {split}: stopped at row {rows:,}")
            continue
        if rows != b_manifest["output"][split]["rows"]:
            problems.append(f"{split}: {rows} rows, manifest says {b_manifest['output'][split]['rows']}")
        if changed != b_manifest["relabel"][split]["counts"].get("changed", 0):
            problems.append(f"{split}: {changed} labels differ, manifest says "
                            f"{b_manifest['relabel'][split]['counts'].get('changed', 0)}")
        print(f"check {split}: {rows:,} rows, {changed:,} labels differ, {probed:,} re-probed")
    for problem in problems:
        print("FAIL:", problem, file=sys.stderr)
    if not problems:
        print("PASS: B is A row for row except eligible labels")
    return 1 if problems else 0


# ---- games ----------------------------------------------------------------

def _games_shard(task):
    path, shard, shards = task
    tb = _tb
    entries = {}
    with open(path, encoding="utf-8", errors="replace") as stream:
        index = 0
        while True:
            if index % shards != shard:
                if not chess.pgn.skip_game(stream):
                    break
                index += 1
                continue
            game = chess.pgn.read_game(stream)
            index += 1
            if game is None:
                break
            result = RESULT_POINTS.get(game.headers.get("Result", "*"))
            board = game.board()
            sid = extract.start_id(game)
            entry = None
            moves = iter(game.mainline_moves())
            ply = 0
            while True:
                if chess.popcount(board.occupied) <= MAX_MEN:
                    points, kind, _margin = verdict(board, tb)
                    entry = (points, kind, material_key(board), ply)
                    break
                move = next(moves, None)
                if move is None:
                    break
                board.push(move)
                ply += 1
            entries[sid] = (result, entry)
    return entries


def games(a_dir: Path, syzygy: str, out: Path, jobs: int) -> int:
    a_manifest = json.loads((a_dir / "manifest.json").read_text(encoding="utf-8"))
    paths = [item["path"] for item in a_manifest["inputs"]]
    shards = max(1, jobs // len(paths))
    tasks = [(path, shard, shards) for path in paths for shard in range(shards)]
    entries = {}
    with multiprocessing.Pool(jobs, initializer=_init, initargs=(syzygy,)) as pool:
        for part in pool.imap_unordered(_games_shard, tasks):
            entries.update(part)

    stats = collections.Counter()
    families = collections.defaultdict(collections.Counter)
    for result, entry in entries.values():
        stats["games"] += 1
        if entry is None:
            stats["never_six_men"] += 1
            continue
        points, kind, family, _ply = entry
        if points is None:
            stats[f"entry_{kind}"] += 1
            continue
        stats["adjudicable"] += 1
        stats[f"entry_{kind}"] += 1
        families[family]["reached"] += 1
        if points != result:
            stats["result_contradicted"] += 1
            stats[f"contradicted_{kind}"] += 1
            families[family]["contradicted"] += 1

    rows = collections.Counter()
    for split in SPLITS:
        path = a_dir / a_manifest["output"][split]["file"]
        with path.open(encoding="utf-8") as source:
            source.readline()
            source.readline()
            for line in source:
                fen, result, _gid, sid, ply, _phase = line.rstrip("\n").split(",")
                rows[f"{split}_rows"] += 1
                game_result, entry = entries.get(sid, (None, None))
                if game_result is None:
                    rows[f"{split}_no_game"] += 1
                    continue
                if entry is None or entry[0] is None or entry[0] == game_result:
                    continue
                rows[f"{split}_in_contradicted_games"] += 1
                if men(fen.split(" ", 1)[0]) > MAX_MEN:
                    rows[f"{split}_beyond_local"] += 1
    top = sorted(((k, v) for k, v in families.items() if v["contradicted"]),
                 key=lambda kv: -kv[1]["contradicted"])[:20]
    report = {"label_contract": LABEL_CONTRACT, "corpus": str(a_dir.resolve()),
              "games": dict(stats), "rows": dict(rows),
              "families_contradicted_top": {k: dict(v) for k, v in top}}
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8", newline="\n")
    print(json.dumps({"games": report["games"], "rows": report["rows"]}, indent=2))
    return 0


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="command", required=True)
    b = sub.add_parser("build")
    b.add_argument("a_dir", type=Path)
    b.add_argument("b_dir", type=Path)
    b.add_argument("--syzygy", required=True)
    b.add_argument("--jobs", type=int, default=max(1, (multiprocessing.cpu_count() or 2) - 2))
    c = sub.add_parser("check")
    c.add_argument("a_dir", type=Path)
    c.add_argument("b_dir", type=Path)
    c.add_argument("--syzygy", help="also re-probe a sample of B's eligible rows")
    g = sub.add_parser("games")
    g.add_argument("a_dir", type=Path)
    g.add_argument("--syzygy", required=True)
    g.add_argument("--out", type=Path, required=True)
    g.add_argument("--jobs", type=int, default=max(1, (multiprocessing.cpu_count() or 2) - 2))
    args = parser.parse_args(argv)
    if args.command == "build":
        return build(args.a_dir, args.b_dir, args.syzygy, args.jobs)
    if args.command == "check":
        return check(args.a_dir, args.b_dir, args.syzygy)
    return games(args.a_dir, args.syzygy, args.out, args.jobs)


if __name__ == "__main__":
    sys.exit(main())
