#!/usr/bin/env python3
"""Measure the evaluation scale ratios and the search-minus-static residual (B.0).

PLAN rule 2 converts a donor constant through a measured scale ratio, and
PLAN's *Scale conversion* paragraph asks B.0 for three numbers on one corpus:

1. Basilisk's static evaluation against modern Stockfish's search-facing
   evaluation (the value Stockfish's margins are compared with, in its own
   internal units);
2. Basilisk's static evaluation against the classical oracle's own HCE, in the
   oracle's internal units (classical Stockfish `PawnValueEg` = 206);
3. Basilisk's own search-minus-static residual at a fixed node budget, with
   the same residual for the oracle's search driving Basilisk's evaluation as
   a control.

Static sources:
  Basilisk   `basilisk-texel --dump-eval` (TEXEL_TRACE: full evaluation, the
             lazy path disabled), white's point of view, centipawns.
  bridge     the oracle with `Use Basilisk HCE=true`: Basilisk's evaluator as
             the search sees it (lazy path live), printed in pawns to 0.01.
  oracle     the oracle with `Use Basilisk HCE=false`: classical Stockfish's
             HCE, printed as Value / PawnValueEg to 0.01.
  stockfish  modern Stockfish's `eval`: the raw network output in internal
             units (side to move) and the final search-facing value, printed in
             centipawns through its WDL normalisation `100 * v / a(material)`,
             which this script inverts with the pinned `win_rate_params`.

Every value is taken with its sign made side-to-move relative and only its
magnitude enters a ratio. Positions in check are skipped (Stockfish refuses to
evaluate them). Nothing here is Elo; the output is a units measurement.

Usage:
  python tools/diag/scale_ratio.py --corpus tools/texel/data/holdout_phase911.csv \
      --sample 10000 --seed 20261006 --bench-fens tools/results/b0/bench_fens.epd \
      --texel build/b0-texel/basilisk-texel.exe \
      --oracle tools/test_engines/oracle-1.10.1.exe \
      --stockfish D:/chess/engines/stockfish/stockfish-19-windows-x86-64-universal.exe \
      --basilisk tools/test_engines/basilisk-a74-nps-pgo1-pext-pgo.exe \
      --residual-sample 2000 --residual-nodes 50000 \
      --out tools/results/b0/scale_ratio.json
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import pathlib
import random
import re
import statistics
import subprocess
import time

import chess

# Pinned Stockfish win-rate model (src/uci.cpp, sf_19 / 59aae690): a(material)
# is the centipawn normaliser, material = P + 3N + 3B + 5R + 9Q clamped to
# [17, 78] and divided by 58.
SF_AS = (-142.72052667, 372.35176398, -340.71073572, 415.23490212)
ORACLE_PAWN_VALUE_EG = 206

RE_SF_RAW = re.compile(r"NNUE evaluation\s+([+-]?\d+)\s+\(side to move, internal units\)")
RE_SF_FINAL = re.compile(r"Final evaluation\s+([+-]?\d+\.\d+)\s+\(white side\)")
RE_ORACLE_SF = re.compile(r"Original Stockfish HCE final evaluation:\s*([+-]?\d+\.\d+)")
RE_ORACLE_BRIDGE = re.compile(r"^Final evaluation:\s*([+-]?\d+\.\d+)")
RE_SCORE = re.compile(r"^info depth (\d+) .*?score (cp|mate) (-?\d+)")
RE_OPTION = re.compile(r"^option name (.*?) type ", re.IGNORECASE)


def sha256(path: str) -> str:
    digest = hashlib.sha256()
    with open(path, "rb") as stream:
        for block in iter(lambda: stream.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


def sf_normaliser(board: chess.Board) -> float:
    material = (len(board.pieces(chess.PAWN, True)) + len(board.pieces(chess.PAWN, False))
                + 3 * (len(board.pieces(chess.KNIGHT, True)) + len(board.pieces(chess.KNIGHT, False)))
                + 3 * (len(board.pieces(chess.BISHOP, True)) + len(board.pieces(chess.BISHOP, False)))
                + 5 * (len(board.pieces(chess.ROOK, True)) + len(board.pieces(chess.ROOK, False)))
                + 9 * (len(board.pieces(chess.QUEEN, True)) + len(board.pieces(chess.QUEEN, False))))
    m = min(max(material, 17), 78) / 58.0
    return ((SF_AS[0] * m + SF_AS[1]) * m + SF_AS[2]) * m + SF_AS[3]


def non_pawn_material(board: chess.Board) -> int:
    values = {chess.KNIGHT: 300, chess.BISHOP: 300, chess.ROOK: 500, chess.QUEEN: 900}
    return sum(values[pt] * (len(board.pieces(pt, True)) + len(board.pieces(pt, False)))
               for pt in values)


class Engine:
    def __init__(self, path: str, options=()):
        self.path = os.path.abspath(path)
        self.process = subprocess.Popen(
            [self.path], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT, text=True, bufsize=1,
            cwd=os.path.dirname(self.path))
        self.send("uci")
        startup = self.until(lambda line: line.strip() == "uciok")
        advertised = {m.group(1).casefold() for line in startup
                      if (m := RE_OPTION.match(line.strip()))}
        for name, value in options:
            if name.casefold() not in advertised:
                self.close()
                raise RuntimeError(f"{self.path} does not advertise option {name}")
            self.send(f"setoption name {name} value {value}")
        self.ready()

    def send(self, command: str):
        self.process.stdin.write(command + "\n")
        self.process.stdin.flush()

    def until(self, predicate, timeout=120):
        deadline = time.monotonic() + timeout
        seen = []
        while True:
            line = self.process.stdout.readline()
            if line == "":
                raise RuntimeError(f"{self.path} closed stdout")
            lowered = line.lower()
            if "no such option" in lowered or "unknown option" in lowered:
                raise RuntimeError(f"{self.path} rejected an option: {line.strip()}")
            seen.append(line.rstrip("\r\n"))
            if predicate(line):
                return seen
            if time.monotonic() > deadline:
                raise TimeoutError(f"{self.path} timed out")

    def ready(self):
        self.send("isready")
        return self.until(lambda line: line.strip() == "readyok")

    def eval_lines(self, fen: str):
        self.send(f"position fen {fen}")
        self.send("eval")
        return self.ready()

    def search_score(self, fen: str, nodes: int):
        """Last completed iteration's score, side to move; None for mate scores."""
        self.send("ucinewgame")
        self.ready()
        self.send(f"position fen {fen}")
        self.send(f"go nodes {nodes}")
        lines = self.until(lambda line: line.startswith("bestmove"), timeout=600)
        score = None
        for line in lines:
            if " lowerbound" in line or " upperbound" in line:
                continue
            m = RE_SCORE.match(line.strip())
            if m:
                score = None if m.group(2) == "mate" else int(m.group(3))
        return score

    def close(self):
        try:
            self.send("quit")
            self.process.wait(timeout=10)
        except (OSError, ValueError, subprocess.TimeoutExpired):
            self.process.kill()


def load_corpus(path: str, sample: int, seed: int):
    rows = []
    with open(path, encoding="utf-8") as stream:
        for line in stream:
            line = line.strip()
            if not line:
                continue
            fen, _, result = line.partition(";")
            rows.append((fen.strip(), result.strip() or "0.5"))
    if sample and sample < len(rows):
        rows = random.Random(seed).sample(rows, sample)
    return rows


def texel_dump(texel: str, rows, workdir: pathlib.Path):
    csv_path = workdir / "scale_corpus.csv"
    with csv_path.open("w", encoding="utf-8", newline="\n") as stream:
        for fen, result in rows:
            stream.write(f"{fen};{result}\n")
    proc = subprocess.run([os.path.abspath(texel), "--dump-eval", str(csv_path)],
                          capture_output=True, text=True, check=True)
    values = []
    for line in proc.stdout.splitlines():
        parts = line.split()
        if len(parts) == 2 and re.fullmatch(r"-?\d+", parts[0]):
            values.append(int(parts[0]))
    if len(values) != len(rows):
        raise RuntimeError(f"texel dumped {len(values)} values for {len(rows)} rows")
    return values


def phase(board: chess.Board) -> str:
    npm = non_pawn_material(board)
    if npm >= 5800:
        return "opening"
    if npm <= 2400:
        return "endgame"
    return "middlegame"


def mean_abs(values):
    return statistics.fmean(abs(v) for v in values) if values else 0.0


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--corpus", required=True)
    parser.add_argument("--sample", type=int, default=10000)
    parser.add_argument("--seed", type=int, default=20261006)
    parser.add_argument("--bench-fens", default=None)
    parser.add_argument("--texel", required=True)
    parser.add_argument("--oracle", required=True)
    parser.add_argument("--stockfish", required=True)
    parser.add_argument("--basilisk", default=None, help="release binary for the residual")
    parser.add_argument("--residual-sample", type=int, default=2000)
    parser.add_argument("--residual-nodes", type=int, default=50000)
    parser.add_argument("--out", required=True)
    args = parser.parse_args()

    out_path = pathlib.Path(args.out)
    out_path.parent.mkdir(parents=True, exist_ok=True)

    rows = []
    if args.bench_fens:
        for line in pathlib.Path(args.bench_fens).read_text(encoding="utf-8").splitlines():
            if line.strip() and not line.startswith("#"):
                rows.append((line.strip(), "0.5"))
    bench_count = len(rows)
    rows += load_corpus(args.corpus, args.sample, args.seed)

    kept = []
    skipped_check = 0
    for index, (fen, result) in enumerate(rows):
        board = chess.Board(fen)
        if board.is_check():
            skipped_check += 1
            continue
        kept.append({"index": index, "fen": fen, "result": result,
                     "set": "bench" if index < bench_count else "corpus",
                     "phase": phase(board), "stm": "w" if board.turn else "b",
                     "sf_a": sf_normaliser(board)})
    print(f"{len(kept)} positions ({skipped_check} in check skipped)")

    texel_values = texel_dump(args.texel, [(p["fen"], p["result"]) for p in kept],
                              out_path.parent)
    for p, v in zip(kept, texel_values):
        p["basilisk_white"] = v
        p["basilisk"] = v if p["stm"] == "w" else -v

    oracle_sf = Engine(args.oracle, [("Use Basilisk HCE", "false")])
    oracle_bridge = Engine(args.oracle, [("Use Basilisk HCE", "true")])
    stockfish = Engine(args.stockfish)
    try:
        for i, p in enumerate(kept, 1):
            text = "\n".join(oracle_sf.eval_lines(p["fen"]))
            m = RE_ORACLE_SF.search(text)
            if not m:
                raise RuntimeError(f"oracle (SF HCE) printed no evaluation for {p['fen']}")
            white_pawns = float(m.group(1))
            p["oracle_sf_internal_white"] = round(white_pawns * ORACLE_PAWN_VALUE_EG)
            text = "\n".join(oracle_bridge.eval_lines(p["fen"]))
            m = next((RE_ORACLE_BRIDGE.match(line.strip()) for line in text.splitlines()
                      if RE_ORACLE_BRIDGE.match(line.strip())), None)
            if not m:
                raise RuntimeError(f"oracle (bridge) printed no evaluation for {p['fen']}")
            p["bridge_cp_white"] = round(float(m.group(1)) * 100)
            text = "\n".join(stockfish.eval_lines(p["fen"]))
            raw = RE_SF_RAW.search(text)
            final = RE_SF_FINAL.search(text)
            if not raw or not final:
                raise RuntimeError(f"stockfish printed no evaluation for {p['fen']}")
            p["sf_raw_internal_stm"] = int(raw.group(1))
            p["sf_final_cp_white"] = round(float(final.group(1)) * 100)
            p["sf_final_internal_white"] = round(p["sf_final_cp_white"] * p["sf_a"] / 100.0)
            if i % 1000 == 0:
                print(f"  evals {i}/{len(kept)}")
    finally:
        oracle_sf.close()
        oracle_bridge.close()
        stockfish.close()

    def summarise(subset):
        bas = [p["basilisk"] for p in subset]
        bridge = [p["bridge_cp_white"] for p in subset]
        osf = [p["oracle_sf_internal_white"] for p in subset]
        sfr = [p["sf_raw_internal_stm"] for p in subset]
        sff = [p["sf_final_internal_white"] for p in subset]
        out = {
            "n": len(subset),
            "mean_abs_basilisk_cp": mean_abs(bas),
            "mean_abs_bridge_cp": mean_abs(bridge),
            "mean_abs_oracle_hce_internal": mean_abs(osf),
            "mean_abs_sf_raw_internal": mean_abs(sfr),
            "mean_abs_sf_final_internal": mean_abs(sff),
        }
        if out["mean_abs_sf_final_internal"]:
            out["ratio_basilisk_to_sf_final"] = out["mean_abs_basilisk_cp"] / out["mean_abs_sf_final_internal"]
            out["ratio_basilisk_to_sf_raw"] = out["mean_abs_basilisk_cp"] / out["mean_abs_sf_raw_internal"]
        if out["mean_abs_oracle_hce_internal"]:
            out["ratio_basilisk_to_oracle_hce"] = out["mean_abs_basilisk_cp"] / out["mean_abs_oracle_hce_internal"]
        # per-position ratio distribution against the search-facing Stockfish value
        ratios = [abs(p["basilisk"]) / abs(p["sf_final_internal_white"])
                  for p in subset if abs(p["sf_final_internal_white"]) >= 20 and abs(p["basilisk"]) >= 10]
        if ratios:
            ratios.sort()
            out["per_position_ratio_sf_final"] = {
                "n": len(ratios), "median": statistics.median(ratios),
                "q1": ratios[len(ratios) // 4], "q3": ratios[3 * len(ratios) // 4]}
        # sign agreement between Basilisk (white POV) and Stockfish final (white POV)
        agree = sum(1 for p in subset if (p["basilisk_white"] > 0) == (p["sf_final_internal_white"] > 0)
                    and p["basilisk_white"] != 0 and p["sf_final_internal_white"] != 0)
        out["sign_agreement_sf_final"] = agree / len(subset) if subset else 0.0
        lazy_diff = [abs(p["basilisk_white"] - p["bridge_cp_white"]) for p in subset]
        out["bridge_vs_texel_mean_abs_diff_cp"] = statistics.fmean(lazy_diff) if lazy_diff else 0.0
        out["bridge_vs_texel_max_abs_diff_cp"] = max(lazy_diff) if lazy_diff else 0
        return out

    report = {
        "schema": "basilisk-scale-ratio-v1",
        "corpus": args.corpus, "corpus_sha256": sha256(args.corpus),
        "sample": args.sample, "seed": args.seed, "bench_fens": args.bench_fens,
        "binaries": {name: {"path": os.path.abspath(p), "sha256": sha256(p)}
                     for name, p in (("texel", args.texel), ("oracle", args.oracle),
                                     ("stockfish", args.stockfish),
                                     ("basilisk", args.basilisk)) if p},
        "oracle_pawn_value_eg": ORACLE_PAWN_VALUE_EG,
        "stockfish_win_rate_as": SF_AS,
        "skipped_in_check": skipped_check,
        "summary": {
            "all": summarise(kept),
            "bench": summarise([p for p in kept if p["set"] == "bench"]),
            "corpus": summarise([p for p in kept if p["set"] == "corpus"]),
            "opening": summarise([p for p in kept if p["phase"] == "opening"]),
            "middlegame": summarise([p for p in kept if p["phase"] == "middlegame"]),
            "endgame": summarise([p for p in kept if p["phase"] == "endgame"]),
        },
    }

    if args.basilisk and args.residual_sample > 0:
        corpus_positions = [p for p in kept if p["set"] == "corpus"]
        residual_set = random.Random(args.seed + 1).sample(
            corpus_positions, min(args.residual_sample, len(corpus_positions)))
        basilisk = Engine(args.basilisk, [("Hash", "64"), ("Threads", "1")])
        oracle = Engine(args.oracle, [("Hash", "64"), ("Threads", "1"), ("Use Basilisk HCE", "true")])
        residual_rows = []
        try:
            for i, p in enumerate(residual_set, 1):
                sb = basilisk.search_score(p["fen"], args.residual_nodes)
                so = oracle.search_score(p["fen"], args.residual_nodes)
                residual_rows.append({"fen": p["fen"], "static_stm": p["basilisk"],
                                      "basilisk_score": sb, "oracle_score": so})
                if i % 500 == 0:
                    print(f"  residual {i}/{len(residual_set)}")
        finally:
            basilisk.close()
            oracle.close()
        usable = [r for r in residual_rows if r["basilisk_score"] is not None and r["oracle_score"] is not None]
        bas_res = [abs(r["basilisk_score"] - r["static_stm"]) for r in usable]
        ora_res = [abs(r["oracle_score"] - r["static_stm"]) for r in usable]
        report["residual"] = {
            "nodes": args.residual_nodes, "sampled": len(residual_set), "usable": len(usable),
            "mean_abs_static_cp": mean_abs([r["static_stm"] for r in usable]),
            "basilisk_mean_abs_residual_cp": statistics.fmean(bas_res) if bas_res else 0.0,
            "basilisk_median_abs_residual_cp": statistics.median(bas_res) if bas_res else 0.0,
            "oracle_search_mean_abs_residual_cp": statistics.fmean(ora_res) if ora_res else 0.0,
            "oracle_search_median_abs_residual_cp": statistics.median(ora_res) if ora_res else 0.0,
            "basilisk_vs_oracle_mean_abs_score_diff_cp": statistics.fmean(
                abs(r["basilisk_score"] - r["oracle_score"]) for r in usable) if usable else 0.0,
            "rows": residual_rows,
        }

    report["positions"] = kept
    with out_path.open("w", encoding="utf-8") as stream:
        json.dump(report, stream, indent=1)
        stream.write("\n")
    printable = {k: v for k, v in report["summary"].items()}
    print(json.dumps(printable, indent=1))
    if "residual" in report:
        print(json.dumps({k: v for k, v in report["residual"].items() if k != "rows"}, indent=1))


if __name__ == "__main__":
    main()
