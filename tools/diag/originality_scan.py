"""Originality scan: shared numeric runs, scaled numeric tables and shared
token runs between a candidate source tree and donor trees.

usage: python tools/diag/originality_scan.py [--exclude DIR ...] <out_dir> <cand_tree> <donor_name>=<donor_tree> ...

A tree is a directory holding `src/`, exported with `git archive <rev> src`.
Outputs `<out_dir>/<donor_name>.json` per donor and `<out_dir>/summary.json`
(also printed). `--exclude DIR` skips a subdirectory (a path prefix relative
to the tree, for example `src/vendored`) in every tree; Basilisk's vendored
Fathom lives under `external/`, outside `src/`, so nothing needs excluding.

Method source: Rarog's `tools/diag/originality_scan.py`, described with its
positive control in
`D:/code/rarog/analysis/originality_review_2.6.0_2026-10-22.md`
(section *Method*, in the sibling repository). Numeric runs, scaled tables
and token runs, their thresholds (numeric runs of 5+, scaled tables of 6+
values within +-1, raw token runs of 20+, identifier-normalised runs of 40+)
and the JSON layout are unchanged.

Changes for Basilisk, which is C++ where Rarog is Rust:

- Source extensions are .cpp .h .hpp .c .cc .cxx (no .rs).
- Token runs were computed only for .rs files, so only for Rust donors.
  They now run on every C++ source file, for every donor, with the same
  thresholds. The Rust-only tokens `=>` and `@` are gone from the token
  pattern and `<=>` is added.
- Character literals ('a', '\\n') are stripped with comments and strings, so
  an apostrophe cannot confuse the number or token patterns.
- Numeric literals take C++ forms: digit separators (1'000), binary (0b101),
  and u/l/ul/ull/f suffixes, instead of Rust's `_` separators and
  u8..i128/usize suffixes (`_` is still tolerated).
- The tool is a module with `run(...)` and `main(argv)` so tests can call it.
- Files are read in binary and decoded as UTF-8 with replacement, so the
  result does not depend on the platform's default encoding or line endings.
- `--exclude` was added (optional).
"""
import argparse
import bisect
import json
import os
import re
import sys
from collections import defaultdict

SRC_EXT = {".cpp", ".h", ".hpp", ".c", ".cc", ".cxx"}

# Thresholds (same as the Rarog review).
NUM_RUN_MIN = 5
TOKEN_RAW_K = 20
TOKEN_NORM_K = 40
SCALED_MIN_LEN = 6

COMMENT_RE = re.compile(r"//[^\n]*|/\*.*?\*/", re.S)
STRING_RE = re.compile(r'"(?:\\.|[^"\\\n])*"')
CHAR_RE = re.compile(r"(?<![A-Za-z_0-9'])'(?:\\[^'\n]*|[^'\\\n])'")
NUM_RE = re.compile(
    r"(?<![A-Za-z_0-9.'])((?:-[ \t]*)?"
    r"(?:0[xX][0-9A-Fa-f_']+|0[bB][01_']+|\d[\d_']*(?:\.\d[\d_']*)?(?:[eE][-+]?\d+)?))"
    r"(?:[uU]?[lL]{0,2}|[fF])(?![A-Za-z_0-9])")
S_PAIR_RE = re.compile(r"\bS\(\s*(-?\d+)\s*,\s*(-?\d+)\s*\)")
TOKEN_RE = re.compile(
    r"[A-Za-z_][A-Za-z_0-9]*|0[xX][0-9A-Fa-f_']+|\d[\d_']*(?:\.\d+)?"
    r"|::|->|<=>|<<=|>>=|<=|>=|==|!=|&&|\|\||<<|>>|\+=|-=|\*=|/=|[{}()\[\];,.:+\-*/%&|^!<>=?#~]")


def read_text(path):
    with open(path, "rb") as f:
        return f.read().decode("utf-8", errors="replace").replace("\r\n", "\n")


def files(tree, exclude=()):
    for root, dirs, names in os.walk(tree):
        dirs.sort()
        for n in sorted(names):
            if os.path.splitext(n)[1] in SRC_EXT:
                p = os.path.join(root, n)
                rel = os.path.relpath(p, tree).replace("\\", "/")
                if any(rel == e or rel.startswith(e.rstrip("/") + "/") for e in exclude):
                    continue
                yield rel, p


def strip(text):
    # Keep line structure: replace comments, strings and character literals
    # by same-length blanks.
    def blank(m):
        return re.sub(r"[^\n]", " ", m.group(0))
    text = COMMENT_RE.sub(blank, text)
    text = STRING_RE.sub(blank, text)
    return CHAR_RE.sub(blank, text)


_LS = {}


def line_of(text, pos):
    ent = _LS.get(id(text))
    if ent is None or ent[0] is not text:
        ent = (text, [m.start() for m in re.finditer("\n", text)])
        _LS[id(text)] = ent
    return bisect.bisect_left(ent[1], pos) + 1


def parse_num(s):
    s = s.replace(" ", "").replace("\t", "").replace("_", "").replace("'", "")
    neg = s.startswith("-")
    body = s[1:] if neg else s
    low = body.lower()
    if low.startswith("0x"):
        v = int(body, 16)
    elif low.startswith("0b"):
        v = int(body, 2)
    elif "." in body or "e" in low:
        v = float(body)
    else:
        v = int(body)
    return -v if neg else v


def num_streams(tree, exclude=()):
    """Per file: list of (value, line). Files also give S(mg,eg)
    component streams so a split mg/eg table can match."""
    out = {}
    for rel, p in files(tree, exclude):
        text = strip(read_text(p))
        seq = [(parse_num(m.group(1)), line_of(text, m.start())) for m in NUM_RE.finditer(text)]
        out[rel] = seq
        pairs = [(m, line_of(text, m.start())) for m in S_PAIR_RE.finditer(text)]
        if pairs:
            out[rel + "#mg"] = [(int(m.group(1)), ln) for m, ln in pairs]
            out[rel + "#eg"] = [(int(m.group(2)), ln) for m, ln in pairs]
    return out


def trivial(vals):
    s = set(vals)
    if s <= {0, 1, -1, 2}:
        return True
    diffs = {vals[i + 1] - vals[i] for i in range(len(vals) - 1)}
    if len(diffs) == 1 and abs(next(iter(diffs))) <= 1:
        return True  # 1,2,3,4,... or constant
    if all(isinstance(v, int) and v > 0 and (v & (v - 1)) == 0 for v in vals):
        return True  # powers of two
    return False


def common_runs(a, b, k):
    """Maximal common runs (length >= k) between value lists a and b."""
    av = [x[0] for x in a]
    bv = [x[0] for x in b]
    index = defaultdict(list)
    for j in range(len(bv) - k + 1):
        index[tuple(bv[j:j + k])].append(j)
    runs = []
    seen = set()
    for i in range(len(av) - k + 1):
        for j in index.get(tuple(av[i:i + k]), ()):
            if (i - 1, j - 1) in seen and i > 0 and j > 0 and av[i - 1] == bv[j - 1]:
                seen.add((i, j))
                continue
            seen.add((i, j))
            n = k
            while i + n < len(av) and j + n < len(bv) and av[i + n] == bv[j + n]:
                n += 1
            runs.append((i, j, n))
    return runs


def bracket_arrays(tree, exclude=()):
    """Numeric arrays written as [ ... ] or { ... } with >= 4 integer items."""
    out = []
    arr_re = re.compile(r"[\[{]\s*((?:-?\d+\s*,\s*){3,}-?\d+)\s*,?\s*[\]}]")
    for rel, p in files(tree, exclude):
        text = strip(read_text(p))
        for m in arr_re.finditer(text):
            vals = [int(x) for x in re.findall(r"-?\d+", m.group(1))]
            out.append((rel, line_of(text, m.start()), vals))
    return out


def scaled_matches(cand_arrays, donor_streams, min_len=SCALED_MIN_LEN):
    """Candidate arrays that equal a donor window times a constant k
    (0.25..4, k != 1) to within rounding (|err| <= 1 on every element)."""
    hits = []
    for rel, ln, A in cand_arrays:
        if len(A) < min_len or len(set(A)) < 4 or trivial(A):
            continue
        for drel, seq in donor_streams.items():
            B = [x[0] for x in seq]
            if len(B) < len(A) or not all(isinstance(v, int) for v in B):
                continue
            for j in range(len(B) - len(A) + 1):
                W = B[j:j + len(A)]
                if len(set(W)) < 4:
                    continue
                den = sum(w * w for w in W)
                if den == 0:
                    continue
                k = sum(a * w for a, w in zip(A, W)) / den
                if not (0.25 <= k <= 4) or abs(k - 1) < 1e-9:
                    continue
                if all(abs(a - k * w) <= 1.0 for a, w in zip(A, W)):
                    hits.append((rel, ln, A, drel, seq[j][1], W, round(k, 4)))
    return hits


def tokens(tree, normalize, exclude=()):
    out = {}
    for rel, p in files(tree, exclude):
        text = strip(read_text(p))
        toks = []
        for m in TOKEN_RE.finditer(text):
            t = m.group(0)
            if normalize and (t[0].isalpha() or t[0] == "_"):
                t = "ID"
            toks.append((t, line_of(text, m.start())))
        out[rel] = toks
    return out


def token_runs(cand, donor, k):
    index = defaultdict(list)
    for drel, toks in donor.items():
        tv = [t[0] for t in toks]
        for j in range(len(tv) - k + 1):
            index[hash(tuple(tv[j:j + k]))].append((drel, j))
    res = []
    for crel, toks in cand.items():
        cv = [t[0] for t in toks]
        i = 0
        while i <= len(cv) - k:
            best = None
            for drel, j in index.get(hash(tuple(cv[i:i + k])), ()):
                dv = [t[0] for t in donor[drel]]
                if dv[j:j + k] != cv[i:i + k]:
                    continue
                n = k
                while i + n < len(cv) and j + n < len(dv) and cv[i + n] == dv[j + n]:
                    n += 1
                if best is None or n > best[2]:
                    best = (drel, j, n)
            if best:
                drel, j, n = best
                dtoks = donor[drel]
                res.append({"cand": crel, "cand_lines": [toks[i][1], toks[i + n - 1][1]],
                            "donor": drel, "donor_lines": [dtoks[j][1], dtoks[j + n - 1][1]],
                            "tokens": n})
                i += n
            else:
                i += 1
    return res


def run(out_dir, cand_tree, donors, exclude=()):
    os.makedirs(out_dir, exist_ok=True)
    cand_nums = num_streams(cand_tree, exclude)
    cand_arrays = bracket_arrays(cand_tree, exclude)
    cand_tok_raw = tokens(cand_tree, False, exclude)
    cand_tok_norm = tokens(cand_tree, True, exclude)
    summary = {"cand_files": len(cand_nums), "cand_numbers": sum(len(v) for v in cand_nums.values()),
               "cand_arrays": len(cand_arrays), "donors": {}}
    for name, tree in donors.items():
        dn = num_streams(tree, exclude)
        runs = []
        for crel, cseq in cand_nums.items():
            for drel, dseq in dn.items():
                for i, j, n in common_runs(cseq, dseq, NUM_RUN_MIN):
                    vals = [x[0] for x in cseq[i:i + n]]
                    runs.append({"cand": crel, "cand_line": cseq[i][1], "donor": drel, "donor_line": dseq[j][1],
                                 "len": n, "trivial": trivial(vals), "vals": vals[:40]})
        runs.sort(key=lambda r: (-r["len"], r["cand"]))
        scaled = scaled_matches(cand_arrays, dn)
        rec = {"numeric_runs_nontrivial": [r for r in runs if not r["trivial"]],
               "numeric_runs_trivial_count": sum(r["trivial"] for r in runs),
               "scaled": [dict(zip(["cand", "cand_line", "cand_vals", "donor", "donor_line", "donor_vals", "k"], h)) for h in scaled]}
        dt_raw = tokens(tree, False, exclude)
        dt_norm = tokens(tree, True, exclude)
        rec["token_runs_raw_k20"] = sorted(token_runs(cand_tok_raw, dt_raw, TOKEN_RAW_K), key=lambda r: -r["tokens"])
        rec["token_runs_norm_k40"] = sorted(token_runs(cand_tok_norm, dt_norm, TOKEN_NORM_K), key=lambda r: -r["tokens"])
        rec["donor_tokens"] = sum(len(v) for v in dt_raw.values())
        rec["donor_numbers"] = sum(len(v) for k, v in dn.items() if "#" not in k)
        with open(os.path.join(out_dir, name + ".json"), "w") as f:
            json.dump(rec, f, indent=1)
        summary["donors"][name] = {k: (len(v) if isinstance(v, list) else v) for k, v in rec.items()}
    summary["cand_tokens"] = sum(len(v) for v in cand_tok_raw.values())
    with open(os.path.join(out_dir, "summary.json"), "w") as f:
        json.dump(summary, f, indent=1)
    return summary


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("out_dir")
    ap.add_argument("cand_tree")
    ap.add_argument("donors", nargs="*", metavar="NAME=TREE")
    ap.add_argument("--exclude", action="append", default=[], metavar="DIR",
                    help="skip this subdirectory (path prefix inside each tree)")
    a = ap.parse_args(argv)
    donors = {}
    for d in a.donors:
        if "=" not in d:
            ap.error("donor must be NAME=TREE: " + d)
        name, tree = d.split("=", 1)
        donors[name] = tree
    summary = run(a.out_dir, a.cand_tree, donors, a.exclude)
    print(json.dumps(summary, indent=1))
    return 0


if __name__ == "__main__":
    sys.exit(main())
