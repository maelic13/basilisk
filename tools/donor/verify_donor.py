#!/usr/bin/env python3
"""Prove a donor-comparison hybrid's evaluation equals its origin's (PLAN C.0, BAS-O09).

Drives two engines built by `build_donor.py` over the same positions with the
`evalraw` command and requires the raw evaluations to be equal on every
position that is not in check. A third engine may be named as a control: the
candidate must differ from it on a large share of positions, or the equality
proves nothing (both could be the host's evaluation).

Positions come from a file (one FEN a line, or a CSV with a `fen` column) and
from a generator of random legal low-material positions, which reaches the
specialised endgame functions that game positions rarely do.

Usage:
  python tools/donor/verify_donor.py --candidate <hybrid.exe> --reference <origin.exe>
      [--control <host-stock.exe>] [--fens <file>] [--synthetic 1000] [--seed 1]
      [--json <report.json>]

Exit status 0 only with zero mismatches, an equal in-check set and, when a
control is named, at least half the positions differing from it.
"""

from __future__ import annotations

import argparse
import csv
import json
import pathlib
import random
import subprocess
import sys

import chess

# Material classes with a dedicated value or scaling function in classical
# Stockfish, and their neighbours. Upper case is the strong side.
SIGNATURES = [
    "KPk", "KBNk", "KQk", "KRk", "KNNk", "KNNkp", "KRkp", "KRkb", "KRkn", "KQkp", "KQkr",
    "KBPk", "KBPkb", "KBPPkb", "KBPkn", "KRPkr", "KRPkb", "KRPPkrp", "KQkrp", "KPkp", "KPPk",
    "KNPk", "KNPkb", "KBBk", "KQkb", "KQkn", "KRBkr", "KRNkr", "KQPkq", "KBNkp", "KRPkrp",
    "KBPPkbp", "KQRkq", "KNkp", "KBkp", "KRRkr", "KQkqp", "KPPkp", "KBPkp", "KRkbp",
]
# Positions written before their answers are read. An anonymous pipe holds
# about 4 KB on Windows: a block's answers must fit in it, or the engine
# blocks on its output while this script blocks on the engine's input.
CHUNK = 64


def synthetic(per_signature: int, seed: int) -> list[str]:
    """Random legal positions per material class: either colour strong, either to move."""
    rng = random.Random(seed)
    fens: list[str] = []
    for signature in SIGNATURES:
        made = 0
        while made < per_signature:
            board = chess.Board(None)
            swap = rng.random() < 0.5
            squares = rng.sample(range(64), len(signature))
            legal = True
            for letter, square in zip(signature, squares):
                colour = chess.WHITE if letter.isupper() else chess.BLACK
                if swap:
                    colour = not colour
                kind = chess.Piece.from_symbol(letter.upper()).piece_type
                if kind == chess.PAWN and chess.square_rank(square) in (0, 7):
                    legal = False
                    break
                board.set_piece_at(square, chess.Piece(kind, colour))
            if not legal:
                continue
            board.turn = rng.random() < 0.5
            board.halfmove_clock = rng.randint(0, 60)
            board.fullmove_number = 40 + board.halfmove_clock // 2
            if not board.is_valid():
                continue
            fens.append(board.fen())
            made += 1
    return fens


def read_fens(path: pathlib.Path) -> list[str]:
    text = path.read_text(encoding="utf-8-sig")
    if path.suffix.lower() == ".csv":
        return [row["fen"] for row in csv.DictReader(text.splitlines())]
    return [line.strip() for line in text.splitlines() if line.strip()]


class Engine:
    def __init__(self, path: pathlib.Path) -> None:
        self.path = path
        self.process = subprocess.Popen([str(path)], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                        text=True, bufsize=1)
        self._send("uci")
        self._wait("uciok")
        self._send("isready")
        self._wait("readyok")

    def _send(self, text: str) -> None:
        self.process.stdin.write(text + "\n")
        self.process.stdin.flush()

    def _wait(self, token: str) -> None:
        while True:
            line = self.process.stdout.readline()
            if not line:
                raise RuntimeError(f"{self.path.name} closed its output before `{token}`.")
            if line.strip() == token:
                return

    def evalraw(self, fens: list[str]) -> list[str]:
        """One answer per FEN: an integer as text, or `incheck`."""
        answers: list[str] = []
        for start in range(0, len(fens), CHUNK):
            block = fens[start:start + CHUNK]
            self.process.stdin.write("".join(f"position fen {fen}\nevalraw\n" for fen in block))
            self.process.stdin.flush()
            while len(answers) < start + len(block):
                line = self.process.stdout.readline()
                if not line:
                    raise RuntimeError(f"{self.path.name} closed its output during evalraw.")
                if line.startswith("evalraw "):
                    answers.append(line.split()[1])
        return answers

    def close(self) -> None:
        try:
            self._send("quit")
            self.process.wait(timeout=10)
        except Exception:
            self.process.kill()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--candidate", required=True, type=pathlib.Path)
    parser.add_argument("--reference", required=True, type=pathlib.Path)
    parser.add_argument("--control", type=pathlib.Path)
    parser.add_argument("--fens", type=pathlib.Path)
    parser.add_argument("--synthetic", type=int, default=1000, help="positions per material class; 0 for none")
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--json", type=pathlib.Path)
    args = parser.parse_args()

    groups: list[tuple[str, list[str]]] = []
    if args.fens:
        groups.append(("file", list(dict.fromkeys(read_fens(args.fens)))))
    if args.synthetic > 0:
        groups.append(("synthetic", synthetic(args.synthetic, args.seed)))
    if not groups:
        parser.error("no positions: give --fens or --synthetic")

    engines = {"candidate": Engine(args.candidate), "reference": Engine(args.reference)}
    if args.control:
        engines["control"] = Engine(args.control)
    report: dict = {"candidate": str(args.candidate), "reference": str(args.reference),
                    "control": str(args.control) if args.control else None, "seed": args.seed, "groups": {}}
    failed = False
    try:
        for label, fens in groups:
            answers = {name: engine.evalraw(fens) for name, engine in engines.items()}
            compared = in_check = check_disagree = mismatches = differs_control = 0
            examples: list[dict] = []
            for index, fen in enumerate(fens):
                cand, ref = answers["candidate"][index], answers["reference"][index]
                if cand == "incheck" or ref == "incheck":
                    in_check += 1
                    check_disagree += cand != ref
                    continue
                compared += 1
                if cand != ref:
                    mismatches += 1
                    if len(examples) < 5:
                        examples.append({"fen": fen, "candidate": cand, "reference": ref})
                if args.control and answers["control"][index] != cand:
                    differs_control += 1
            share = differs_control / compared if compared and args.control else None
            report["groups"][label] = {"positions": len(fens), "compared": compared, "in_check": in_check,
                                       "in_check_disagreements": check_disagree, "mismatches": mismatches,
                                       "differs_from_control": share, "examples": examples}
            print(f"{label}: {len(fens)} positions, {compared} compared, {in_check} in check, "
                  f"{mismatches} mismatches"
                  + (f", {share:.1%} differ from the control" if share is not None else ""))
            failed |= mismatches > 0 or check_disagree > 0 or compared == 0
            failed |= share is not None and share < 0.5
    finally:
        for engine in engines.values():
            engine.close()
    report["verdict"] = "FAIL" if failed else "PASS"
    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(report["verdict"])
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
