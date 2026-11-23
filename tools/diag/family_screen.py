#!/usr/bin/env python3
"""The evaluation family screen, both ways (PLAN C.2.9; C.0's prototype, BAS-E58).

On game positions with results, it asks two questions per evaluation family,
by phase band, for each donor evaluation:

* **donor into Basilisk:** how much does the donor family's mg/eg pair add to
  Basilisk's whole score in predicting the result?
  ``logit = a*B + b_mg*w_mg*T_mg + b_eg*w_eg*T_eg`` against ``logit = k*B``;
* **Basilisk into the donor:** how much does Basilisk's family add to the
  donor's whole score? ``logit = c*D + b*F`` against ``logit = k*D``.

The gain is the drop in cross-entropy, with a 95% interval from a bootstrap
over games (positions of one game move together). Every evaluator gets its
own K. Rows where Basilisk serves an override or a draw verdict (the rows a
Texel fit leaves out) and rows where the donor answers from a specialised
endgame (all its terms zero) are separated and only counted: they would
saturate the logistic on a few unconverted wins.

Basilisk's families are sums of the traced tuner's terms: each slot's
coefficient (``basilisk-texel --dump-coeffs``) times its weight, by the slot
groups in ``FAMILIES``. The join between the tuner's rows and the sample is
by file line (``--dump-row-lines``) and is checked by the served score: a
row whose played score differs from ``--dump-eval``'s refuses the read. The
donor's term tables must sum to their printed totals. Either check failing
is the known-bad input this tool refuses.

    python tools/diag/family_screen.py collect --tuner build/texel/basilisk-texel \\
        --rows tools/texel/data/corpusA/rows/validation.csv --positions 60000 \\
        --donor D95=tools/test_engines/donor-sf9587-stock.exe \\
        --donor D16=tools/test_engines/donor-sf16-stock.exe --out tools/results/screen
    python tools/diag/family_screen.py screen --data tools/results/screen/dataset.npz \\
        --out tools/results/screen/report.json
"""

from __future__ import annotations

import argparse
import concurrent.futures
import csv
import json
import random
import re
import subprocess
import sys
import tempfile
from pathlib import Path

import numpy as np

SCHEMA = "basilisk-family-screen-v1"
DONOR_TERMS = ["Material", "Imbalance", "Pawns", "Knights", "Bishops", "Rooks", "Queens", "Mobility",
               "King safety", "Threats", "Passed", "Space", "Winnable"]
# Basilisk's families, by the tuner's slot groups (exact names or prefixes).
FAMILIES = {
    "material": ["MgVal", "EgVal"],
    "pst": ["Pst"],
    "imbalance": ["Imb", "BpMg", "BpEg", "BishopPairPawns"],
    "pawns": ["Doubled", "Isolated", "ConnectedMg", "ConnectedEg", "ConnectedRank", "Backward",
              "WeakUnopposed", "BlockedPawn", "PawnMajority", "PawnIslands"],
    "passed": ["PassedMg", "PassedEg", "PassFree", "PassSafe", "PassedPathSafe", "PassedBlockDefended",
               "PassedKingBlock", "RookBehindPasser", "EnemyRookPasser"],
    "pieces": ["ReachableOutpost", "KnightOutpost", "BadBishop", "MinorBehindPawn", "KingProtector",
               "RookClosed", "RookQueenFile", "ConnectedRooks", "RookOpen", "RookSemi", "Rook7th",
               "QueenInfiltration", "Trapped"],
    "mobility": ["Mob"],
    "threats": ["Threat", "WeakQueenProt", "Restricted"],
    "king": ["Ks", "SafetyTable", "Shelter", "Storm", "ProxBase", "MinorKingRing", "RookKingRing"],
    "space": ["Space"],
    "tempo": ["Tempo"],
}
BANDS = [("all", 0, 24), ("phase 0-9", 0, 9), ("phase 10-17", 10, 17), ("phase 18-24", 18, 24)]
TERM_LINE = re.compile(r"[+-]?\d+\.\d+")
FINAL_LINE = re.compile(r"(?:Final|Classical) evaluation:?\s+([+-]?\d+\.\d+)\s+\(white side\)")
SUM_TOLERANCE = 0.08  # pawns: 13 terms each rounded to 0.01 as printed


class Refused(Exception):
    pass


def family_of(group: str) -> str | None:
    for family, prefixes in FAMILIES.items():
        if any(group == p or group.startswith(p) for p in prefixes):
            return family
    return None


# ---------------------------------------------------------------- collection

def read_rows(path: Path) -> tuple[list[str], list[tuple[int, list[str]]]]:
    """A rows-v1 file's header lines and its data rows with their file line."""
    head, rows = [], []
    with path.open(encoding="utf-8") as f:
        for number, line in enumerate(f, start=1):
            line = line.rstrip("\r\n")
            if not line:
                continue
            if line.startswith("#") or line.startswith("fen,"):
                head.append(line)
                continue
            rows.append((number, line.split(",")))
    return head, rows


def sample_rows(rows, count: int, seed: int):
    picked = sorted(random.Random(seed).sample(range(len(rows)), min(count, len(rows))))
    return [rows[i][1] for i in picked]


def run(command: list[str]) -> str:
    done = subprocess.run(command, capture_output=True, text=True, check=False)
    if done.returncode != 0:
        raise Refused(f"{' '.join(command)} exited {done.returncode}:\n{done.stdout}{done.stderr}")
    return done.stdout


def registry(tuner: str) -> tuple[list[str], np.ndarray]:
    groups, values = [], []
    for line in run([tuner, "--list-params"]).splitlines():
        parts = line.split("\t")
        if len(parts) == 4 and parts[0].isdigit():
            groups.append(parts[1])
            values.append(float(parts[3]))
    return groups, np.array(values)


def join_basilisk(sample: Path, n: int, lines_text: str, eval_text: str, width: int, coeffs: np.ndarray):
    """Map the tuner's coefficient rows onto the sample's rows, checked by the
    score: a row the release serves in full must score what --dump-eval's full
    evaluation gives (a lazy row cannot, by design). Returns (used flag, the
    served score B, coefficient row index, lazy flag)."""
    served = [int(l.split()[0]) for l in eval_text.splitlines()
              if len(l.split()) == 2 and l.split()[0].lstrip("-").isdigit()]
    if len(served) != n:
        raise Refused(f"--dump-eval gave {len(served)} scores for {n} rows")
    _, data = read_rows(sample)
    line_to_row = {line: i for i, (line, _) in enumerate(data)}
    used = np.zeros(n, bool)
    lazy = np.zeros(n, bool)
    index = np.full(n, -1)
    score = np.array(served, float)
    pairs = [l.split() for l in lines_text.splitlines() if l.strip()]
    if len(pairs) != coeffs.shape[0]:
        raise Refused(f"{len(pairs)} row lines for {coeffs.shape[0]} coefficient rows")
    for k, (line, played, is_lazy) in enumerate(pairs):
        row = line_to_row.get(int(line))
        if row is None:
            raise Refused(f"the tuner names line {line}, which holds no sample row")
        if is_lazy == "0" and served[row] != int(played):
            raise Refused(f"line {line}: the tuner's played score {played} is not the evaluation "
                          f"{served[row]}; the rows are misaligned")
        used[row], index[row], lazy[row] = True, k, is_lazy == "1"
        score[row] = int(played)
    if used.sum() and (used & ~lazy).sum() < 0.5 * used.sum():
        raise Refused("under half the used rows are served in full, so the join is barely checked")
    return used, score, index, lazy


def parse_donor(text: str) -> tuple[dict[str, tuple[float, float]], float | None]:
    terms, final = {}, None
    for line in text.splitlines():
        if "|" in line:
            cells = [c.strip() for c in line.strip().strip("|").split("|")]
            numbers = TERM_LINE.findall(cells[-1]) if cells else []
            if cells and cells[0] in DONOR_TERMS + ["Total"] and len(numbers) == 2:
                terms[cells[0]] = (float(numbers[0]), float(numbers[1]))
        match = FINAL_LINE.search(line)
        if match and (final is None or line.strip().startswith("Final")):
            final = float(match.group(1))
    return terms, final


def check_donor_sum(terms) -> None:
    if "Total" not in terms:
        return
    for half in (0, 1):
        total = sum(terms[t][half] for t in DONOR_TERMS if t in terms)
        if abs(total - terms["Total"][half]) > SUM_TOLERANCE:
            raise Refused(f"a donor table sums to {total:.2f}, not its printed total {terms['Total'][half]:.2f}")


def donor_shard(exe: str, fens: list[str]) -> list[tuple[dict, float | None]]:
    process = subprocess.Popen([exe], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT, text=True, bufsize=1)

    def send(text):
        process.stdin.write(text + "\n")
        process.stdin.flush()

    def until(token):
        lines = []
        while True:
            line = process.stdout.readline()
            if line == "":
                raise Refused(f"{exe} died")
            if line.strip() == token:
                return "\n".join(lines)
            lines.append(line)

    send("uci")
    until("uciok")
    send("setoption name Use NNUE value false")
    send("isready")
    until("readyok")
    out = []
    for fen in fens:
        send("position fen " + fen)
        send("eval")
        send("isready")
        terms, final = parse_donor(until("readyok"))
        check_donor_sum(terms)
        out.append((terms, final))
    send("quit")
    process.wait(timeout=30)
    return out


def collect(args) -> int:
    tuner = str(Path(args.tuner).resolve())
    head, rows = read_rows(args.rows)
    sample = sample_rows(rows, args.positions, args.seed)
    args.out.mkdir(parents=True, exist_ok=True)
    sample_path = args.out / "sample.csv"
    sample_path.write_text("\n".join(head + [",".join(r) for r in sample]) + "\n", encoding="utf-8")
    n = len(sample)
    groups, weights = registry(tuner)
    with tempfile.TemporaryDirectory() as tmp:
        dump, lines = Path(tmp) / "c.f64", Path(tmp) / "lines.txt"
        run([tuner, "--dump-coeffs", str(sample_path), str(dump)])
        run([tuner, "--dump-row-lines", str(sample_path), str(lines)])
        coeffs = np.fromfile(dump, dtype="<f8").reshape(-1, len(groups))
        used, served, index, lazy = join_basilisk(sample_path, n, lines.read_text(), run(
            [tuner, "--dump-eval", str(sample_path)]), len(groups), coeffs)
    names = list(FAMILIES)
    families = np.zeros((n, len(names)))
    member = np.array([family_of(g) for g in groups], dtype=object)
    for j, name in enumerate(names):
        cols = member == name
        families[used, j] = coeffs[index[used]][:, cols] @ weights[cols]
    record = {"schema": SCHEMA, "rows": str(args.rows), "seed": args.seed, "positions": n,
              "families": names, "unassigned_slots": sorted({g for g, m in zip(groups, member) if m is None})}
    fens = [r[0] for r in sample]
    donors = {}
    for spec in args.donor:
        label, exe = spec.split("=", 1)
        exe = str(Path(exe).resolve())
        shards = [fens[k::args.workers] for k in range(args.workers)]
        with concurrent.futures.ThreadPoolExecutor(args.workers) as pool:
            parts = list(pool.map(lambda s: donor_shard(exe, s), shards))
        merged = [None] * n
        for k, part in enumerate(parts):
            for j, item in enumerate(part):
                merged[k + j * args.workers] = item
        terms = np.full((n, len(DONOR_TERMS), 2), np.nan)
        total = np.full(n, np.nan)
        for i, (t, final) in enumerate(merged):
            for j, name in enumerate(DONOR_TERMS):
                if name in t:
                    terms[i, j] = t[name]
            total[i] = np.nan if final is None else final
        donors[label] = (terms, total)
        record[f"donor_{label}"] = exe
    np.savez_compressed(
        args.out / "dataset.npz",
        result=np.array([float(r[1]) for r in sample]), game=np.array([r[2] for r in sample]),
        phase=np.array([int(r[5]) for r in sample]), served=served, used=used, lazy=lazy, families=families,
        family_names=np.array(names), donor_labels=np.array(list(donors)),
        **{f"{k}_terms": v[0] for k, v in donors.items()}, **{f"{k}_total": v[1] for k, v in donors.items()})
    (args.out / "collect.json").write_text(json.dumps(record, indent=1) + "\n", encoding="utf-8")
    print(f"{n} rows; {int(used.sum())} used by the fit; donors {', '.join(donors)}; "
          f"unassigned slot groups: {record['unassigned_slots'] or 'none'}")
    return 0


# ------------------------------------------------------------------ analysis

def ce_rows(z, y):
    return np.logaddexp(0, z) - y * z


def irls(X, y, ridge=1e-8, iters=50):
    sd = X.std(axis=0)
    sd[sd == 0] = 1.0
    Xs = X / sd
    w = np.zeros(X.shape[1])
    last = ce_rows(Xs @ w, y).mean()
    for _ in range(iters):
        p = 1 / (1 + np.exp(-(Xs @ w)))
        g = Xs.T @ (p - y) / len(y) + ridge * w
        H = (Xs * (p * (1 - p))[:, None]).T @ Xs / len(y) + ridge * np.eye(len(w))
        step = np.linalg.solve(H, g)
        t = 1.0
        while True:
            w2 = w - t * step
            value = ce_rows(Xs @ w2, y).mean()
            if value <= last + 1e-12 or t < 1e-4:
                break
            t /= 2
        w = w2
        if abs(last - value) < 1e-12:
            last = value
            break
        last = value
    return w / sd, last


def boot_gain(base_loss, model_loss, games, reps, seed):
    """Mean gain and a 95% interval, resampling games."""
    _, g = np.unique(games, return_inverse=True)
    d = np.bincount(g, weights=base_loss - model_loss)
    c = np.bincount(g).astype(float)
    rng = np.random.default_rng(seed)
    picks = [rng.integers(0, len(d), len(d)) for _ in range(reps)]
    draws = [d[p].sum() / c[p].sum() for p in picks]
    lo, hi = np.percentile(draws, [2.5, 97.5])
    return float((base_loss - model_loss).mean()), float(lo), float(hi)


def screen_cell(base, extra, y, games, reps, seed):
    """Gain of adding `extra` columns to a single fitted-K score `base`."""
    wb, _ = irls(base[:, None], y)
    loss0 = ce_rows(base * wb[0], y)
    X = np.column_stack([base, *extra])
    w, _ = irls(X, y)
    loss1 = ce_rows(X @ w, y)
    gain, lo, hi = boot_gain(loss0, loss1, games, reps, seed)
    return {"gain_e4": round(1e4 * gain, 2), "ci_e4": [round(1e4 * lo, 2), round(1e4 * hi, 2)],
            "base_coef_ratio": round(float(w[0] / wb[0]), 3)}


def screen(args) -> int:
    data = np.load(args.data, allow_pickle=False)
    y, games, phase = data["result"], data["game"], data["phase"]
    if not np.all(np.isin(y, (0.0, 0.5, 1.0))):
        raise Refused("results must be game outcomes {0, 0.5, 1}")
    served, used, F = data["served"], data["used"], data["families"]
    names = [str(n) for n in data["family_names"]]
    report = {"schema": SCHEMA, "rows": int(len(y)), "override_rows": int((~used).sum()), "donors": {}}
    wm, we = phase / 24.0, 1 - phase / 24.0
    for label in [str(l) for l in data["donor_labels"]]:
        T, D = data[f"{label}_terms"], data[f"{label}_total"]
        special = np.all(np.nan_to_num(np.abs(T)).sum(axis=2) == 0, axis=1) | np.isnan(D)
        regular = used & ~special
        out = {"specialised_rows": int(special.sum()), "regular_rows": int(regular.sum()), "bands": {}}
        for band, lo, hi in BANDS:
            m = regular & (phase >= lo) & (phase <= hi)
            if m.sum() < args.min_rows:
                continue
            cell = {"rows": int(m.sum()), "donor_into_basilisk": {}, "basilisk_into_donor": {}}
            for j, term in enumerate(DONOR_TERMS):
                cols = [c for c in (T[m, j, 0] * wm[m], T[m, j, 1] * we[m]) if np.std(c) > 0]
                if cols:
                    cell["donor_into_basilisk"][term] = screen_cell(served[m], cols, y[m], games[m],
                                                                    args.reps, args.seed)
            for j, name in enumerate(names):
                col = F[m, j]
                if np.std(col) > 0:
                    cell["basilisk_into_donor"][name] = screen_cell(D[m], [col], y[m], games[m],
                                                                    args.reps, args.seed)
            out["bands"][band] = cell
        report["donors"][label] = out
    args.out.write_text(json.dumps(report, indent=1) + "\n", encoding="utf-8")
    for label, out in report["donors"].items():
        for band, cell in out["bands"].items():
            for way in ("donor_into_basilisk", "basilisk_into_donor"):
                ranked = sorted(cell[way].items(), key=lambda kv: -kv[1]["gain_e4"])
                print(f"{label} {band} {way}: " + "  ".join(
                    f"{k} {v['gain_e4']:.1f}[{v['ci_e4'][0]:.1f},{v['ci_e4'][1]:.1f}]" for k, v in ranked))
    return 0


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="command", required=True)
    c = sub.add_parser("collect")
    c.add_argument("--tuner", required=True)
    c.add_argument("--rows", type=Path, required=True)
    c.add_argument("--positions", type=int, default=60000)
    c.add_argument("--seed", type=int, default=20261124)
    c.add_argument("--donor", action="append", required=True, help="LABEL=ENGINE, repeatable")
    c.add_argument("--workers", type=int, default=8)
    c.add_argument("--out", type=Path, required=True)
    s = sub.add_parser("screen")
    s.add_argument("--data", type=Path, required=True)
    s.add_argument("--out", type=Path, required=True)
    s.add_argument("--reps", type=int, default=300)
    s.add_argument("--seed", type=int, default=1)
    s.add_argument("--min-rows", type=int, default=500)
    args = parser.parse_args(argv)
    try:
        return collect(args) if args.command == "collect" else screen(args)
    except Refused as refusal:
        print(f"REFUSED: {refusal}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
