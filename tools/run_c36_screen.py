#!/usr/bin/env python3
"""Frozen launcher for C.3.6's own-forms screen (BAS-O18, BAS-O20).

The contract is analysis/c36_own_forms_2026-11-26.md (§4 to §6, Amendment 1)
and the registrations docs/experiments/BAS-O18.md and BAS-O20.md.

  check    every bit alone and mask 0: both builds, bench 13, test_eval,
           --verify on the whole validation split, the played hash; an added
           form's bit at seed 0 must serve mask 0's bench and played hash, and
           a nonzero value in its slots must move the played hash
           (tools/results/c36-check/).
  screen   the fit arms on one corpus, in parallel, each with its own build
           directory and `fit.py --no-bake`, so arms share this checkout:
           N (a null refit), C1 and C2 (the controls: Hanging, RookOnFile
           fixed at 0), the nineteen bits alone, U (the nineteen together), and
           any `--arm NAME:MASK` given (arm P). Each finished arm leaves its
           fit's run.json and its rows' played values (rows.lines).
  report   the measure (tools/diag/form_screen.py) of every arm against N,
           the method gates and the rule, written to <out>/report.json.

A finished arm (its run.json says fitted and rows.lines exists) is skipped;
an arm directory that holds anything else is refused, never overwritten.

Usage:
  python tools/run_c36_screen.py check [--jobs 4] [--dry-run]
  python tools/run_c36_screen.py screen --label A --corpus tools/texel/data/corpusA/rows \\
      --k-phase 3.026687,0.970080 [--init VECTOR] [--arms N,C1,...] [--arm P:MASK] \\
      [--jobs 8] [--dry-run]
  python tools/run_c36_screen.py report --label A --corpus ... --k-phase ...
"""

from __future__ import annotations

import argparse
import concurrent.futures
import json
import pathlib
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools" / "texel"))
sys.path.insert(0, str(ROOT / "tools" / "diag"))
import bake  # noqa: E402
import form_screen  # noqa: E402
import surface  # noqa: E402

EXE = ".exe" if sys.platform == "win32" else ""
PY = sys.executable
FIT = ROOT / "tools" / "texel" / "fit.py"
BUILD = ROOT / "build" / "c36"
RESULTS = ROOT / "tools" / "results"
CORPUS_A = ROOT / "tools" / "texel" / "data" / "corpusA" / "rows"
MASK0_BENCH = 3880333
MASK0_PLAYED = "fa8739a3fc205e63"

# The nineteen candidate bits (bit 15 is reserved), and which of them add a
# term beside the classical form, seeded at 0.
BITS = [b for b in range(20) if b != 15]
ADDED = {13, 14, 16, 17, 18, 19}
CONTROLS = {"C1": ["HangingMg", "HangingEg"], "C2": ["RookOnFileMg", "RookOnFileEg"]}
# C.3.4's fit, as tools/run_c34_fits.py ran it, less the bake.
FIT_ARGS = ["--stages", "kingsafety,linear,kingsafety,polish", "--lr", "0.5", "--epochs", "20000",
            "--patience", "50", "--min-delta", "1e-6", "--l2", "0", "--smooth", "0",
            "--max-positions", "0", "--ks-positions", "100000", "--rank-positions", "300000",
            "--min-support", "200", "--k-mode", "phase", "--no-bake", "--no-relevel-material"]


def bit_name(bit: int) -> str:
    return surface.OWN_FORMS[bit][0]


def run(command: list[str], log: pathlib.Path, cwd: pathlib.Path = ROOT) -> subprocess.CompletedProcess:
    done = subprocess.run(command, cwd=cwd, capture_output=True, text=True, check=False)
    log.parent.mkdir(parents=True, exist_ok=True)
    with log.open("a", encoding="utf-8") as f:
        f.write(f"$ {' '.join(command)}\n{done.stdout}\n--- stderr ---\n{done.stderr}\nexit {done.returncode}\n\n")
    return done


def must(done: subprocess.CompletedProcess, what: str) -> str:
    if done.returncode != 0:
        raise RuntimeError(f"{what} failed (exit {done.returncode}): {done.stderr.strip()[-400:]}")
    return done.stdout


def clean_tree() -> str:
    dirty = subprocess.run(["git", "status", "--porcelain", "--untracked-files=no"], cwd=ROOT,
                           capture_output=True, text=True, check=True).stdout.strip()
    if dirty:
        raise SystemExit("error: the tree has uncommitted changes; the screen runs from a commit")
    return subprocess.run(["git", "rev-parse", "HEAD"], cwd=ROOT, capture_output=True, text=True,
                          check=True).stdout.strip()


def build_trees(tree: pathlib.Path, mask: int, log: pathlib.Path) -> tuple[pathlib.Path, pathlib.Path]:
    """The texel and release trees for one mask: fit.py's configuration."""
    for texel in (True, False):
        sub = tree / ("texel" if texel else "release")
        must(run(["cmake", "--preset", "release", "-B", str(sub), "-DCOMP=clang",
                  f"-DTEXEL={'ON' if texel else 'OFF'}", f"-DEVAL_OWN_FORMS={mask}"], log), f"configure {sub}")
        targets = ["basilisk-texel"] if texel else ["basilisk", "test_eval"]
        must(run(["cmake", "--build", str(sub), "--target", *targets], log), f"build {sub}")
    return tree / "texel" / f"basilisk-texel{EXE}", tree / "release" / f"basilisk{EXE}"


def bench(engine: pathlib.Path, log: pathlib.Path) -> int:
    for line in must(run([str(engine), "bench", "13"], log), "bench").splitlines():
        if line.startswith("Nodes searched"):
            return int(line.split(":")[1].strip().replace(",", ""))
    raise RuntimeError("bench printed no node count")


def played_hash(tuner: pathlib.Path, validation: pathlib.Path, log: pathlib.Path,
                init: pathlib.Path | None = None) -> str:
    command = [str(tuner)] + (["--init", str(init)] if init else []) + ["--played-hash", str(validation)]
    return must(run(command, log), "played hash").split()[-1]


def header_vector() -> list[tuple[str, int, int]]:
    text = (ROOT / "src" / "eval_rb_params.h").read_text(encoding="utf-8")
    return bake.header_vector(text, bake.parse_param_list(text))


def write_vector(path: pathlib.Path, vector: list[tuple[str, int, int]]) -> None:
    root = bake.vector_hash(header_vector())
    path.write_text(f"# {bake.VECTOR_SCHEMA}\n# root {root}\n"
                    + "".join(f"{g} {i} {v}\n" for g, i, v in vector), encoding="utf-8", newline="\n")


def read_vector(path: pathlib.Path) -> list[tuple[str, int, int]]:
    out = []
    for line in path.read_text(encoding="utf-8").splitlines():
        if line.startswith("#") or not line.strip():
            continue
        g, i, v = line.split()
        out.append((g, int(i), int(v)))
    return out


# ---- check -------------------------------------------------------------------

# The OCB form's allowed test_eval failure: the classical arm's parity
# coverage, 90% of the fixture's rows layer-silent, which the form leaves by
# design (research's ruling in C.3.6.3).
OCB_EXPECTED = "silent * 10 >= rows.size() * 9"


def failed_sections(text: str) -> list[str]:
    """test_eval's failed expectations, by the expression its FAIL line prints."""
    out = []
    for line in text.splitlines():
        line = line.strip()
        if line.startswith("FAIL: "):
            out.append(line[len("FAIL: "):].rsplit("  (line", 1)[0].strip())
    return out

def check_one(mask: int, label: str, dry: bool) -> dict:
    out = RESULTS / "c36-check" / label
    log = out / "check.log"
    rec: dict = {"mask": mask, "label": label}
    if dry:
        print(f"check {label}: mask {mask} -> {BUILD / ('check-' + label)}")
        return rec
    out.mkdir(parents=True, exist_ok=True)
    validation = CORPUS_A / "validation.csv"
    tuner, engine = build_trees(BUILD / f"check-{label}", mask, log)
    rec["bench"] = bench(engine, log)
    te = run([str(engine.parent / f"test_eval{EXE}")], log)
    rec["test_eval_exit"] = te.returncode
    rec["test_eval_last"] = te.stdout.strip().splitlines()[-1] if te.stdout.strip() else ""
    rec["test_eval_asymmetric"] = te.stdout.count("asymmetric") + te.stderr.count("asymmetric")
    rec["test_eval_failed"] = failed_sections(te.stdout + te.stderr)
    ver = run([str(tuner), "--verify", str(validation), "--max-positions", "0"], log)
    rec["verify"] = "PASS" if ver.returncode == 0 and "PASS" in ver.stdout else "FAIL"
    rec["played_hash"] = played_hash(tuner, validation, log)
    rec["own_forms"] = int(must(run([str(tuner), "--own-forms"], log), "own-forms").strip())
    if mask and (mask & ~sum(1 << b for b in ADDED)) == 0:
        # An added form at seed 0 is the shipped evaluation; a nonzero value moves it.
        rec["identity_at_seed"] = rec["bench"] == MASK0_BENCH and rec["played_hash"] == MASK0_PLAYED
        groups = {g for b in BITS if mask & (1 << b) for g in surface.OWN_FORMS[b][1]}
        vec = [(g, i, (50 if g in groups else v)) for g, i, v in header_vector()]
        moved = out / "nonzero.txt"
        write_vector(moved, vec)
        rec["played_hash_nonzero"] = played_hash(tuner, validation, log, moved)
        rec["live"] = rec["played_hash_nonzero"] != rec["played_hash"]
    (out / "check.json").write_text(json.dumps(rec, indent=2) + "\n", encoding="utf-8")
    print(f"check {label}: {json.dumps(rec)}", flush=True)
    return rec


def cmd_check(args) -> int:
    revision = clean_tree() if not args.dry_run else "dry-run"
    jobs = [(0, "m0")] + [(1 << b, f"b{b:02d}-{bit_name(b).lower()}") for b in BITS]
    results = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        futures = {pool.submit(check_one, m, label, args.dry_run): label for m, label in jobs}
        for f in concurrent.futures.as_completed(futures):
            try:
                results.append(f.result())
            except Exception as e:  # noqa: BLE001 - report every arm's failure
                results.append({"label": futures[f], "error": str(e)})
                print(f"check {futures[f]}: ERROR {e}", flush=True)
    if args.dry_run:
        return 0
    results.sort(key=lambda r: r.get("mask", -1))
    failures = []
    for r in results:
        if "error" in r:
            failures.append(f"{r['label']}: {r['error']}")
            continue
        if r["verify"] != "PASS":
            failures.append(f"{r['label']}: --verify {r['verify']}")
        if r["own_forms"] != r["mask"]:
            failures.append(f"{r['label']}: tuner reports mask {r['own_forms']}")
        if r["test_eval_asymmetric"]:
            failures.append(f"{r['label']}: colour asymmetry in test_eval")
        # test_eval must pass at every mask. The one allowed failure is the
        # OCB form's: the classical arm's parity coverage (90% of the
        # fixture's rows layer-silent), which that form leaves by design.
        if r["test_eval_exit"] != 0 and not (r["mask"] == 1 << 2 and r.get("test_eval_failed") == [OCB_EXPECTED]):
            failures.append(f"{r['label']}: test_eval exit {r['test_eval_exit']}: {r.get('test_eval_failed')}")
        if r["mask"] == 0 and (r["bench"] != MASK0_BENCH or r["played_hash"] != MASK0_PLAYED):
            failures.append(f"{r['label']}: mask 0 moved: bench {r['bench']}, hash {r['played_hash']}")
        if "identity_at_seed" in r and not (r["identity_at_seed"] and r["live"]):
            failures.append(f"{r['label']}: identity {r['identity_at_seed']}, live {r['live']}")
    summary = {"revision": revision, "results": results, "failures": failures}
    (RESULTS / "c36-check" / "summary.json").write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
    print(f"check: {len(results)} masks, {len(failures)} failures")
    for f in failures:
        print(f"  FAIL {f}")
    return 1 if failures else 0


# ---- screen ------------------------------------------------------------------

def arm_masks(args) -> dict[str, int]:
    union = sum(1 << b for b in BITS)
    arms = {"N": 0, "C1": 0, "C2": 0}
    arms.update({f"b{b:02d}": 1 << b for b in BITS})
    arms["U"] = union
    for spec in args.arm or []:
        name, mask = spec.split(":")
        if int(mask) & ~union:
            raise SystemExit(f"error: arm {name}'s mask {mask} has a bit outside the nineteen")
        arms[name] = int(mask)
    if args.arms:
        wanted = args.arms.split(",")
        missing = [a for a in wanted if a not in arms]
        if missing:
            raise SystemExit(f"error: unknown arms {missing}")
        arms = {a: arms[a] for a in wanted}
    return arms


def arm_done(out: pathlib.Path) -> bool:
    rec = out / "fit" / "run.json"
    if not rec.exists() or not (out / "rows.lines").exists():
        return False
    return json.loads(rec.read_text(encoding="utf-8")).get("result") == "fitted"


def screen_one(label: str, name: str, mask: int, args, dry: bool) -> dict:
    out = RESULTS / f"c36-screen-{label}" / name
    # A bit's arm and the null arm reuse the check step's trees (the same
    # configuration, so fit.py's own configure and build do nothing); the
    # controls, U and P have their own. Two arms never share a tree.
    if name == "N":
        tree = BUILD / "check-m0"
    elif name.startswith("b") and name[1:].isdigit():
        tree = BUILD / f"check-{name}-{bit_name(int(name[1:])).lower()}"
    else:
        tree = BUILD / f"screen-{label}-{name}"
    if arm_done(out):
        print(f"screen {label}/{name}: finished, skipped", flush=True)
        return {"arm": name, "skipped": True}
    if out.exists() and any(out.iterdir()):
        raise RuntimeError(f"{out} holds an unfinished arm; move it aside by hand")
    command_preview = f"mask {mask}, tree {tree}, out {out}"
    if dry:
        print(f"screen {label}/{name}: {command_preview}")
        return {"arm": name}
    out.mkdir(parents=True, exist_ok=True)
    log = out / "launcher.log"
    tuner, engine = build_trees(tree, mask, log)
    expect = bench(engine, log)
    # The arm's surface: the rules at its mask; a control fixes its slots too.
    base_surface = out / "surface-base.tsv"
    must(run([PY, str(ROOT / "tools" / "texel" / "surface.py"), "build", "--tuner", str(tuner),
              "--out", str(base_surface)], log), "surface build")
    init_vector = read_vector(pathlib.Path(args.init)) if args.init else header_vector()
    fix = []
    if name in CONTROLS:
        groups = CONTROLS[name]
        init_vector = [(g, i, (0 if g in groups else v)) for g, i, v in init_vector]
        fix = ["--fix-groups", ",".join(groups)]
    init_path = out / "init.txt"
    write_vector(init_path, init_vector)
    fit_out = out / "fit"
    command = [PY, str(FIT), "--corpus", str(args.corpus), "--out", str(fit_out), "--surface", str(base_surface),
               "--build-dir", str(tree), "--own-forms", str(mask), "--expect-bench", str(expect),
               "--init", str(init_path), "--K-phase", args.k_phase, *fix, *FIT_ARGS]
    must(run(command, log), "fit")
    rec = json.loads((fit_out / "run.json").read_text(encoding="utf-8"))
    final = fit_out / rec["stages"][-1]["vector"]
    validation = pathlib.Path(args.corpus) / "validation.csv"
    must(run([str(tuner), "--init", str(final), "--dump-row-lines", str(validation), str(out / "rows.lines")],
             log), "dump-row-lines")
    summary = {"arm": name, "mask": mask, "bench_seed": expect, "seed_loss": rec["seed_loss"]["loss"],
               "final_loss": rec["final_loss"]["loss"], "played_hash_final": rec["played_hash_final"]}
    (out / "arm.json").write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
    print(f"screen {label}/{name}: {json.dumps(summary)}", flush=True)
    return summary


def cmd_screen(args) -> int:
    if not args.dry_run:
        clean_tree()
    arms = arm_masks(args)
    failures = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        futures = {pool.submit(screen_one, args.label, n, m, args, args.dry_run): n for n, m in arms.items()}
        for f in concurrent.futures.as_completed(futures):
            try:
                f.result()
            except Exception as e:  # noqa: BLE001 - report every arm's failure
                failures.append(f"{futures[f]}: {e}")
                print(f"screen {args.label}/{futures[f]}: ERROR {e}", flush=True)
    print(f"screen {args.label}: {len(arms)} arms, {len(failures)} failures")
    return 1 if failures else 0


# ---- report ------------------------------------------------------------------

def cmd_report(args) -> int:
    base = RESULTS / f"c36-screen-{args.label}"
    k_eg, k_mg = (float(x) for x in args.k_phase.split(","))
    rows = form_screen.read_rows(pathlib.Path(args.corpus) / "validation.csv")
    arms = json.loads((base / "N" / "arm.json").read_text(encoding="utf-8"))
    ref_lines = form_screen.read_lines(base / "N" / "rows.lines")
    scale = args.loss_scale
    delta_min = 2.0e-5 * scale
    report: dict = {"label": args.label, "delta_min": delta_min, "scale": scale, "arms": {}}
    l_base, l_n = arms["seed_loss"], arms["final_loss"]
    report["gate1"] = {"loss_base": l_base, "loss_N": l_n, "diff": abs(l_n - l_base),
                       "limit": delta_min / 2, "passes": abs(l_n - l_base) <= delta_min / 2}
    for arm_dir in sorted(p for p in base.iterdir() if p.is_dir() and p.name != "N"):
        meta_path = arm_dir / "arm.json"
        if not meta_path.exists():
            report["arms"][arm_dir.name] = {"missing": True}
            continue
        meta = json.loads(meta_path.read_text(encoding="utf-8"))
        r = form_screen.screen(rows, ref_lines, form_screen.read_lines(arm_dir / "rows.lines"), k_eg, k_mg,
                               args.resamples, args.seed, l_n, meta["final_loss"])
        r["mask"] = meta["mask"]
        r["passes"] = form_screen.verdict(r, delta_min)
        report["arms"][arm_dir.name] = r
    controls = [report["arms"].get(c, {}) for c in CONTROLS]
    report["gate2"] = {"control_costs": [-c.get("delta", 0.0) for c in controls],
                       "passes": any(-c.get("delta", 0.0) > delta_min for c in controls)}
    report["passing"] = sorted((a for a, r in report["arms"].items()
                                if a.startswith("b") and r.get("passes")),
                               key=lambda a: -report["arms"][a]["delta"])
    (base / "report.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({k: report[k] for k in ("gate1", "gate2", "passing", "delta_min")}, indent=2))
    for a, r in sorted(report["arms"].items()):
        if "delta" in r:
            print(f"  {a:5s} mask {r['mask']:8d}  delta {r['delta']:+.3e}  ci [{r['ci95'][0]:+.3e}, "
                  f"{r['ci95'][1]:+.3e}]  {'PASS' if r['passes'] else ''}")
    return 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    c = sub.add_parser("check")
    c.add_argument("--jobs", type=int, default=4)
    c.add_argument("--dry-run", action="store_true")
    for name in ("screen", "report"):
        s = sub.add_parser(name)
        s.add_argument("--label", required=True)
        s.add_argument("--corpus", required=True)
        s.add_argument("--k-phase", required=True)
        if name == "screen":
            s.add_argument("--init")
            s.add_argument("--arms")
            s.add_argument("--arm", action="append")
            s.add_argument("--jobs", type=int, default=8)
            s.add_argument("--dry-run", action="store_true")
        else:
            s.add_argument("--loss-scale", type=float, default=1.0,
                           help="L_C(R) / L_A(shipped) on corpus C (BAS-O20); 1 on corpus A")
            s.add_argument("--resamples", type=int, default=2000)
            s.add_argument("--seed", type=int, default=360001)
    args = ap.parse_args()
    return {"check": cmd_check, "screen": cmd_screen, "report": cmd_report}[args.cmd](args)


if __name__ == "__main__":
    sys.exit(main())
