#!/usr/bin/env python3
"""The fit's surface: every evaluation slot classed free, fixed or excluded.

The manifest (basilisk-texel-surface-v1, ``tools/texel/surface_v1.tsv``) has one
row per slot of the tuner's registry, in its order. Each row gives:

* its class: **free** (a stage fits it), **fixed** (held at its value: dead,
  a gauge anchor, or a duplicate of another slot's feature) or **excluded**
  (no instrument can fit it);
* the stage that fits a free slot: ``linear`` (the gradient fit) or
  ``kingsafety`` (the finite-difference descent);
* an anchor: ``mean:<set>`` keeps a set's mean at the seed where only the
  set's shape is identified;
* a king-safety knob's bounds and first step;
* the reason.

The rules that class each slot are below, one place, each with its reason.
The tracked manifest is their output, and ``check`` fails when it drifts.

    python tools/texel/surface.py build  --tuner EXE [--out FILE]
    python tools/texel/surface.py check  --tuner EXE [--manifest FILE]
    python tools/texel/surface.py rank   --tuner EXE --corpus CSV [--support-positions N]
                                         [--rank-positions N] [--min-support N]
                                         [--out DERIVED.tsv]
    python tools/texel/surface.py check-vector --tuner EXE VECTOR

``rank`` reads a corpus on two row sets, named apart (C-52, the C.2.8
registration's Amendment 1):

* **The support rows**, the first ``--support-positions`` rows a fit uses (0:
  all of them; ``fit.py`` passes its linear stages' ``--max-positions``).
  Each free linear slot's support is the number of those rows whose
  coefficient is nonzero, ``--dump-coeffs``'s rows and definition, counted
  exactly by the tuner's streamed ``--count-support``. Every slot under
  ``--min-support`` is frozen, and its reason gives the count and the rows
  counted. Counted on a sample, the same threshold would be a far stricter
  one: 200 rows of 30,000 froze 240 slots that the whole split supports.
  ``--feature-support`` counts something else, raw traces on every row.
* **The rank sample**, the first ``--rank-positions`` rows: the dense
  coefficient matrix (``--dump-coeffs``) and the rank of the free columns
  under the mean anchors. It fails when a direction the data cannot see
  survives the anchors, and, by name, when a free slot has no row in the
  sample: a sample too small, not an identity.

**The guard.** ``rank`` fails, naming every slot and table, when a
support-frozen slot belongs to a table (a registry group of more than one
slot) that keeps a free linear member and no mean anchor. Such a table's
level can move while the frozen entry stays: run 1 of C.2.8 stranded 153 PST
entries that way. A frozen slot in a mean-anchored table is allowed and
printed. With ``--out`` and no failure it writes the corpus's manifest.

``check-vector`` validates a complete ``name index value`` vector: every slot
exactly once, the structural zeros zero. It no longer holds a table to PeSTO:
that test failed the correct Stockfish 16 seed and ties on any re-levelled
table. A seed's orientation is checked where a table is imported, by
``seed_sf16.py``'s chess facts.
"""

from __future__ import annotations

import argparse
import subprocess
import sys
import tempfile
from dataclasses import dataclass, field
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SCHEMA = "basilisk-texel-surface-v1"
HEADER = ("index", "group", "sub", "seed", "class", "stage", "anchor", "lo", "hi", "step", "reason")
DEFAULT_MANIFEST = ROOT / "tools" / "texel" / "surface_v1.tsv"

PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING = 1, 2, 3, 4, 5, 6
MOBILITY_GROUPS = [f"Mob{piece}{phase}" for piece in "NBRQ" for phase in ("Mg", "Eg")]
# The king-danger knobs: they shape the integer index the safety table is read
# at, so no linear trace reaches them. Bounds are the descent's, and each first
# step is a tenth of its range (at least 1), so a knob of range 8 and one of
# range 600 do not share one step.
KING_SAFETY_KNOBS = {
    "KsUnit": (0, 20), "KsCoordBonus": (0, 20), "KsSafeCheck": (0, 30),
    "KsWeakRing": (0, 20), "KsKingBlockers": (0, 20), "KsCentralKing": (0, 30),
    "KsShelterStorm": (0, 20), "KsNoqueenNum": (0, 8),
}
SAFETY_TABLE_BOUNDS = (0, 600)
IMB_THEIR_DEAD = {0, 2, 5, 9, 14, 20}  # t = i(i+1)/2 + i: W_i*B_i - B_i*W_i == 0
# The imbalance's counts are [bishop pair, pawns, knights, bishops, rooks,
# queens], term t = i(i+1)/2 + j for j <= i. The taper's phase is
# sum_i w_i (W_i + B_i) with w = 1, 1, 2, 4 for N, B, R, Q, so for any count X
#   (W_X - B_X) * phase = sum_i w_i (W_i + B_i)(W_X - B_X),
# and each (W_i + B_i)(W_X - B_X) is our(i, X) - their(i, X) or our + their.
# A piece's mg PST level (or BpMg) is therefore exactly a sum of cross terms
# while the phase is under its cap. One cross term per identity is held: the
# queen's, their(Q, X) for X = bishop pair, pawns, knights, bishops, rooks
# (t = 15..19), and our(Q, Q) (t = 20) for the queen.
PHASE_ANCHOR_THEIR = {15, 16, 17, 18, 19}
PHASE_ANCHOR_OUR = 20


@dataclass
class Row:
    index: int
    group: str
    sub: int
    seed: int
    klass: str = "free"
    stage: str = "linear"
    anchor: str = ""
    lo: str = ""
    hi: str = ""
    step: str = ""
    reason: str = ""

    def fixed(self, reason: str) -> None:
        self.klass, self.stage, self.anchor, self.reason = "fixed", "", "", reason

    def cells(self) -> list[str]:
        return [str(self.index), self.group, str(self.sub), str(self.seed), self.klass,
                self.stage, self.anchor, self.lo, self.hi, self.step, self.reason]


def registry(tuner: Path, init: Path | None = None) -> list[Row]:
    """The tuner's slots, in order, with the seed (or the --init vector's) values."""
    command = [str(tuner), "--list-params"] + (["--init", str(init)] if init else [])
    done = subprocess.run(command, capture_output=True, text=True, check=False)
    if done.returncode != 0:
        raise SystemExit(f"error: {' '.join(command)} failed:\n{done.stderr}")
    rows = []
    for line in done.stdout.splitlines():
        if line.startswith("Initial vector:") or line == "index\tgroup\tsub\tseed":
            continue
        index, group, sub, seed = line.split("\t")
        rows.append(Row(int(index), group, int(sub), int(seed)))
    if [row.index for row in rows] != list(range(len(rows))):
        raise SystemExit("error: the registry is not in index order")
    return rows


def classify(rows: list[Row]) -> list[Row]:
    """The rules. Each slot is free linear unless a rule below says otherwise."""
    for row in rows:
        g, s = row.group, row.sub
        row.reason = "a traced linear term"
        if g in ("MgVal", "EgVal"):
            if s in (0, KING):
                row.fixed("structural zero: no piece type, and the king has no material value")
            else:
                row.fixed("gauge anchor: a piece's material and the level of its PST are one "
                          "direction; the PSTs are free, so material is held (C-19)")
        elif g in ("PstMgPawn", "PstEgPawn") and (s < 8 or s >= 56):
            row.fixed("dead: no pawn stands on the first or the eighth rank")
        elif g in ("PstMgKing", "PstEgKing"):
            row.anchor = f"mean:{g}"
            row.reason = ("a traced linear term; each side's king reads the table once with "
                          "opposite signs, so the table's level is invisible and its mean is held")
        elif g in MOBILITY_GROUPS:
            row.anchor = f"mean:{g}"
            row.reason = ("a traced linear term; every such piece reads exactly one entry, so the "
                          "table's level is its piece's PST level and its mean is held")
        elif g in ("PassedMg", "PassedEg", "ConnectedRankMg", "ConnectedRankEg") and s in (0, 7):
            row.fixed("dead: a pawn's relative rank is never 0 or 7")
        elif g in ("PassedMg", "PassedEg") and s == 6:
            row.fixed("collinear: a pawn on its seventh rank is always passed, so this is the "
                      "pawn PST's seventh-rank squares moved together (found by `rank`)")
        elif g in ("ThreatByMinorMg", "ThreatByMinorEg", "ThreatByRookMg", "ThreatByRookEg") \
                and s in (0, KING):
            row.fixed("dead: indexed by the threatened piece, never none or a king")
        elif g == "ImbLinear" and s == 0:
            row.fixed("collinear: the bishop-pair indicator, added to mg and eg alike, is "
                      "BpMg plus BpEg moved together (C-20)")
        elif g == "ImbLinear":
            row.fixed("collinear: a piece count added to mg and eg alike is that piece's "
                      "material, which its PST level already carries (C-20)")
        elif g == "ImbOur" and s == 0:
            row.fixed("collinear: the bishop pair squared is the bishop pair, ImbLinear[0] (C-20)")
        elif g == "ImbOur" and s in (6, 9):
            row.fixed("collinear while no side has three bishops, which needs a promotion: "
                      "bishops times the pair is twice the pair, and bishops squared is twice "
                      "the pair plus the count, so both say BpMg/BpEg again (found by `rank`)")
        elif g == "ImbOur" and s == 1:
            row.fixed("collinear: own pawns times the bishop pair is BishopPairPawnsMg plus "
                      "BishopPairPawnsEg moved together (C-20)")
        elif g in ("Rook7thMg", "Rook7thEg"):
            row.fixed("collinear: a rook on its seventh rank, with no other condition, is the "
                      "rook PST's seventh-rank squares moved together (found by `rank`)")
        elif (g == "ImbTheir" and s in PHASE_ANCHOR_THEIR) or (g == "ImbOur" and s == PHASE_ANCHOR_OUR):
            row.fixed("gauge anchor: the taper's phase is a weighted piece count, so a piece's "
                      "(or the bishop pair's) middlegame level is a phase-weighted sum of the "
                      "imbalance's cross terms with it; the queen's cross term is held, one "
                      "per identity (found by `rank`)")
        elif g == "ImbTheir" and s in IMB_THEIR_DEAD:
            row.fixed("dead: W_i*B_i - B_i*W_i is zero in every position (C-20)")
        elif g in ("KsUnit", "KsSafeCheck"):
            if KNIGHT <= s <= QUEEN:
                lo, hi = KING_SAFETY_KNOBS[g]
                knob(row, lo, hi, "a king-danger knob, read for a knight, bishop, rook or queen")
            else:
                row.fixed("never read: only a knight, bishop, rook or queen is indexed")
        elif g in KING_SAFETY_KNOBS:
            lo, hi = KING_SAFETY_KNOBS[g]
            knob(row, lo, hi, "a king-danger knob; it shapes the index, so no linear trace reaches it")
        elif g == "KsNoqueenDen":
            row.fixed("gauge anchor: only KsNoqueenNum / KsNoqueenDen matters, so the "
                      "denominator is held (C-19)")
        elif g == "SafetyTable":
            if s == 0:
                row.fixed("gauge anchor: each side's king reads the table once with opposite "
                          "signs, so a constant shift of it is invisible; entry 0 is held")
            else:
                knob(row, *SAFETY_TABLE_BOUNDS,
                     "the king-danger table, fitted with its index by descent and kept "
                     "non-decreasing; entry 1 is a knob too (C-18)")
        elif g.startswith(("Shelter", "Storm")):
            row.reason = ("a traced linear term, the king's pawn shelter and storm; no stage "
                          "fitted it before (C-21)")
    return rows


def knob(row: Row, lo: int, hi: int, reason: str) -> None:
    row.stage, row.reason = "kingsafety", reason
    row.lo, row.hi, row.step = str(lo), str(hi), str(max(1, (hi - lo) // 10))


def write_manifest(rows: list[Row], path: Path, note: str = "") -> None:
    lines = [f"# {SCHEMA}" + (f" {note}" if note else ""), "\t".join(HEADER)]
    lines += ["\t".join(row.cells()) for row in rows]
    path.write_text("\n".join(lines) + "\n", encoding="utf-8", newline="\n")


def read_manifest(path: Path) -> list[Row]:
    lines = path.read_text(encoding="utf-8").splitlines()
    if not lines or not lines[0].startswith(f"# {SCHEMA}"):
        raise SystemExit(f"error: {path} is not a {SCHEMA} manifest")
    if lines[1] != "\t".join(HEADER):
        raise SystemExit(f"error: {path} has the wrong header")
    rows = []
    for line in lines[2:]:
        c = line.split("\t")
        if len(c) != len(HEADER):
            raise SystemExit(f"error: {path}: a row has {len(c)} columns")
        rows.append(Row(int(c[0]), c[1], int(c[2]), int(c[3]), c[4], c[5], c[6], c[7], c[8], c[9], c[10]))
    return rows


def invariants(rows: list[Row]) -> list[str]:
    """What every manifest must hold, the tracked one and every derived one."""
    errors = []
    anchors: dict[str, list[Row]] = {}
    for row in rows:
        where = f"{row.group}[{row.sub}]"
        if row.klass not in ("free", "fixed", "excluded"):
            errors.append(f"{where}: class {row.klass!r}")
        if (row.klass == "free") != (row.stage in ("linear", "kingsafety")):
            errors.append(f"{where}: a free slot needs a stage, and only a free slot has one")
        if not row.reason:
            errors.append(f"{where}: no reason")
        if row.stage == "kingsafety":
            lo, hi, step = int(row.lo), int(row.hi), int(row.step)
            if not (lo <= row.seed <= hi) or step < 1:
                errors.append(f"{where}: seed {row.seed} outside [{lo}, {hi}] or step {step}")
        if row.anchor:
            if not row.anchor.startswith("mean:"):
                errors.append(f"{where}: anchor {row.anchor!r}")
            anchors.setdefault(row.anchor, []).append(row)
        if row.klass == "fixed" and row.reason.startswith(("dead", "structural zero")) and row.seed != 0:
            errors.append(f"{where}: a dead slot holds {row.seed}, not 0")
    for name, members in anchors.items():
        if any(member.stage != "linear" for member in members) or len(members) < 2:
            errors.append(f"{name}: an anchor's members must be two or more free linear slots")
    return errors


def summary(rows: list[Row]) -> str:
    counts: dict[str, int] = {}
    for row in rows:
        key = row.klass if row.klass != "free" else f"free {row.stage}"
        counts[key] = counts.get(key, 0) + 1
    anchors = len({row.anchor for row in rows if row.anchor})
    return ", ".join(f"{n} {k}" for k, n in sorted(counts.items())) + f"; {anchors} mean anchors"


def cmd_build(args) -> int:
    rows = classify(registry(args.tuner))
    errors = invariants(rows)
    if errors:
        print("\n".join(errors), file=sys.stderr)
        return 1
    write_manifest(rows, args.out)
    print(f"{args.out}: {len(rows)} slots: {summary(rows)}")
    return 0


def cmd_check(args) -> int:
    expected = classify(registry(args.tuner))
    actual = read_manifest(args.manifest)
    errors = invariants(actual)
    if len(actual) != len(expected):
        errors.append(f"{len(actual)} rows for {len(expected)} slots")
    for want, have in zip(expected, actual):
        if want.cells() != have.cells():
            errors.append(f"row {want.index}: the rules give {want.cells()}, the manifest {have.cells()}")
            if len(errors) > 10:
                break
    if errors:
        print("\n".join(errors), file=sys.stderr)
        print(f"FAIL: {args.manifest} does not match the registry and the rules; "
              f"rebuild it with `surface.py build`.", file=sys.stderr)
        return 1
    print(f"PASS: {args.manifest}: {len(actual)} slots: {summary(actual)}")
    return 0


def constrained_basis(free: list[int], rows: list[Row]):
    """An orthonormal basis of the free linear directions the anchors allow:
    each mean anchor removes the direction that moves its whole set alike."""
    import numpy as np
    position = {slot: k for k, slot in enumerate(free)}
    sets: dict[str, list[int]] = {}
    for slot in free:
        if rows[slot].anchor:
            sets.setdefault(rows[slot].anchor, []).append(position[slot])
    constraints = np.zeros((len(sets), len(free)))
    for r, members in enumerate(sets.values()):
        constraints[r, members] = 1.0
    if not len(sets):
        return np.eye(len(free))
    _, sing, vt = np.linalg.svd(constraints)
    return vt[len(sets):].T


@dataclass
class RankReport:
    support: object
    frozen: list[int]
    free: list[int]
    derived: list[Row]
    relative: object
    null: int
    directions: list[tuple[float, list[tuple[str, float]]]]
    support_rows: int = 0
    absent: list[int] = field(default_factory=list)
    stranded: list[tuple[int, str]] = field(default_factory=list)
    anchored: dict[int, bool] = field(default_factory=dict)


def stranded_slots(derived: list[Row], frozen: list[int]) -> tuple[list[tuple[int, str]], dict[int, bool]]:
    """The guard: each support-frozen slot whose table (a group of more than
    one slot) keeps a free linear member and no mean anchor, with its group;
    and, for every frozen slot, whether its table's level is held."""
    groups: dict[str, list[int]] = {}
    for row in derived:
        groups.setdefault(row.group, []).append(row.index)
    stranded, anchored = [], {}
    for slot in frozen:
        members = groups[derived[slot].group]
        free = [m for m in members if derived[m].klass == "free" and derived[m].stage == "linear"]
        held = any(derived[m].anchor for m in free)
        anchored[slot] = held
        if len(members) > 1 and free and not held:
            stranded.append((slot, derived[slot].group))
    return stranded, anchored


def analyse(matrix, rows: list[Row], min_support: int, weak: float, corpus: str,
            support=None, support_rows: int | None = None) -> RankReport:
    """Support-based freezing and the rank of the free linear columns under
    the mean anchors. `matrix` is the rank sample's coefficients (rows x
    slots). `support` is each slot's count on the support rows; without it the
    sample is its own support rows."""
    import numpy as np
    if support is None:
        support, support_rows = np.count_nonzero(matrix, axis=0), matrix.shape[0]
    support = np.asarray(support)
    linear = [row.index for row in rows if row.stage == "linear"]
    frozen = [slot for slot in linear if support[slot] < min_support]
    frozen_set = set(frozen)
    free = [slot for slot in linear if slot not in frozen_set]
    # A frozen member leaves its anchor's set; a set of one is no anchor at all.
    derived = [Row(**vars(row)) for row in rows]
    for slot in frozen:
        derived[slot].fixed(f"frozen: {support[slot]} rows of support, under {min_support}, "
                            f"on {support_rows} rows of {corpus}")
    by_anchor: dict[str, list[int]] = {}
    for slot in free:
        if derived[slot].anchor:
            by_anchor.setdefault(derived[slot].anchor, []).append(slot)
    for members in by_anchor.values():
        if len(members) < 2:
            derived[members[0]].anchor = ""
    stranded, anchored = stranded_slots(derived, frozen)
    # A free slot with no row in the sample is a sample too small, named apart
    # from an identity; the rank is read over the rest.
    in_sample = np.count_nonzero(matrix, axis=0)
    absent = [slot for slot in free if in_sample[slot] == 0]
    present = [slot for slot in free if in_sample[slot] > 0]
    basis = constrained_basis(present, derived)
    reduced = matrix[:, present] @ basis
    scale = np.linalg.norm(reduced, axis=0)
    scale[scale == 0] = 1.0
    reduced = reduced / scale
    # Exact null directions sit at rounding level; a direction well above that
    # but still tiny is an identity the data nearly obeys, named but not fatal.
    exact = max(reduced.shape) * np.finfo(np.float64).eps * 16
    if reduced.shape[0] > 2 * reduced.shape[1]:
        # A tall sample: the triangular factor has the same singular values
        # and right vectors, without the sample-sized left factor.
        reduced = np.linalg.qr(reduced, mode="r")
    _, sing, vt = np.linalg.svd(reduced, full_matrices=False)
    relative = sing / sing[0] if len(sing) and sing[0] > 0 else np.zeros(reduced.shape[1])
    if len(relative) < reduced.shape[1]:  # fewer rows than columns: the rest are null
        relative = np.concatenate([relative, np.zeros(reduced.shape[1] - len(relative))])
    null = int(np.sum(relative <= exact))
    directions = []
    for k in range(1, int(np.sum(relative <= weak)) + 1):
        if k > vt.shape[0]:
            break
        direction = basis @ (vt[-k] / scale)
        top = np.argsort(-np.abs(direction))[:6]
        directions.append((float(relative[-k]),
                           [(f"{rows[present[t]].group}[{rows[present[t]].sub}]", float(direction[t]))
                            for t in top]))
    return RankReport(support, frozen, free, derived, relative, null, directions,
                      int(support_rows), absent, stranded, anchored)


def read_support(path: Path, width: int) -> tuple[int, list[int]]:
    """`basilisk-texel --count-support`'s file: the rows counted, then each
    slot's count, every slot exactly once and in order."""
    lines = path.read_text(encoding="utf-8").split()
    if len(lines) != 2 + 2 * width or lines[0] != "rows":
        raise SystemExit(f"error: {path} is not a support count for {width} slots")
    pairs = [(int(lines[k]), int(lines[k + 1])) for k in range(2, len(lines), 2)]
    if [index for index, _ in pairs] != list(range(width)):
        raise SystemExit(f"error: {path} does not list the slots in order")
    return int(lines[1]), [count for _, count in pairs]


def tuner_step(command: list[str]) -> None:
    done = subprocess.run(command, capture_output=True, text=True, check=False)
    if done.returncode != 0:
        raise SystemExit(f"error: {' '.join(command)}:\n{done.stdout}{done.stderr}")


def cmd_rank(args) -> int:
    import numpy as np
    rows = read_manifest(args.manifest)
    with tempfile.TemporaryDirectory() as tmp:
        counted = Path(tmp) / "support.txt"
        command = [str(args.tuner), "--count-support", str(args.corpus), str(counted)]
        if args.support_positions:
            command += ["--max-positions", str(args.support_positions)]
        tuner_step(command)
        support_rows, support = read_support(counted, len(rows))
        dump = Path(tmp) / "coeffs.f64"
        command = [str(args.tuner), "--dump-coeffs", str(args.corpus), str(dump)]
        if args.rank_positions:
            command += ["--max-positions", str(args.rank_positions)]
        tuner_step(command)
        matrix = np.fromfile(dump, dtype="<f8").reshape(-1, len(rows))
    print(f"support rows: {support_rows:,} of {args.corpus}; rank sample: {matrix.shape[0]:,} rows")
    report = analyse(matrix, rows, args.min_support, args.weak, args.corpus.name, support, support_rows)
    linear = len(report.free) + len(report.frozen)
    print(f"free linear slots: {linear}; under {args.min_support} rows of support: {len(report.frozen)}; "
          f"free: {len(report.free)}")
    for slot in report.frozen:
        held = "its table's level held" if report.anchored[slot] else "no anchor"
        print(f"  frozen {rows[slot].group}[{rows[slot].sub}]: {report.support[slot]} rows ({held})")
    failed = False
    for slot, group in report.stranded:
        print(f"FAIL: {rows[slot].group}[{rows[slot].sub}] is frozen in {group}, whose level is free: "
              f"the table can move away from it", file=sys.stderr)
        failed = True
    for slot in report.absent:
        print(f"FAIL: {rows[slot].group}[{rows[slot].sub}] is free but has no row in the "
              f"{matrix.shape[0]:,}-row rank sample: the sample is too small", file=sys.stderr)
        failed = True
    width = len(report.relative)
    print(f"rank of the free columns under the anchors: {width - report.null} of {width}; "
          f"smallest relative singular values {', '.join(f'{v:.3g}' for v in report.relative[-6:])}")
    for k, (relative, top) in enumerate(report.directions, start=1):
        kind = "null" if k <= report.null else "weak"
        names = ", ".join(f"{name} {weight:+.2f}" for name, weight in top)
        print(f"  {kind} direction (relative {relative:.2g}): {names}",
              file=sys.stderr if kind == "null" else sys.stdout)
    if report.null:
        print(f"FAIL: {report.null} directions of the free surface are invisible to the data.",
              file=sys.stderr)
        failed = True
    if failed:
        return 1
    if args.out:
        errors = invariants(report.derived)
        if errors:
            print("\n".join(errors), file=sys.stderr)
            return 1
        write_manifest(report.derived, args.out, f"derived from {DEFAULT_MANIFEST.name} on "
                                                 f"{args.corpus.name}, min support {args.min_support} "
                                                 f"on {support_rows} rows")
        print(f"wrote {args.out}: {summary(report.derived)}")
    return 0


def cmd_check_vector(args) -> int:
    rows = classify(registry(args.tuner, args.vector))  # the tuner refuses a partial vector
    errors = [f"{row.group}[{row.sub}] = {row.seed}, a dead slot"
              for row in rows if row.klass == "fixed"
              and row.reason.startswith(("dead", "structural zero")) and row.seed != 0]
    if errors:
        print("\n".join(errors), file=sys.stderr)
        print(f"FAIL: {args.vector}", file=sys.stderr)
        return 1
    print(f"PASS: {args.vector}: all {len(rows)} slots")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    for name in ("build", "check", "rank", "check-vector"):
        cmd = commands.add_parser(name)
        cmd.add_argument("--tuner", type=Path, required=True, help="the basilisk-texel binary")
        if name == "build":
            cmd.add_argument("--out", type=Path, default=DEFAULT_MANIFEST)
        if name in ("check", "rank"):
            cmd.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
        if name == "rank":
            cmd.add_argument("--corpus", type=Path, required=True)
            cmd.add_argument("--support-positions", type=int, default=0,
                             help="the support rows: the first N rows a fit uses, 0 for all")
            cmd.add_argument("--rank-positions", type=int, default=300_000,
                             help="the rank sample: the first N rows a fit uses")
            cmd.add_argument("--min-support", type=int, default=200)
            cmd.add_argument("--weak", type=float, default=1e-6,
                             help="name every direction with a relative singular value under this")
            cmd.add_argument("--out", type=Path)
        if name == "check-vector":
            cmd.add_argument("vector", type=Path)
    args = parser.parse_args()
    return {"build": cmd_build, "check": cmd_check, "rank": cmd_rank,
            "check-vector": cmd_check_vector}[args.command](args)


if __name__ == "__main__":
    raise SystemExit(main())
