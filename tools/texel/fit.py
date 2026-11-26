#!/usr/bin/env python3
"""The Texel fit driver: one corpus, one surface, the stage schedule, provenance.

    python tools/texel/fit.py --corpus DIR --out RUN_DIR [options]

It replaces tools/texel/phase911.ps1. What it guarantees (PLAN C.2.1.4):

* **Its inputs are what they claim.** The tree is clean and its revision
  recorded. The corpus is a basilisk-texel-rows-v1 manifest whose files hash
  as published, with no parse errors. The surface manifest is checked against
  the tuner's registry. Every input and output is hashed in ``run.json``.
* **The corpus's surface** is ``surface.py rank``'s. Support is counted
  exactly on the rows the linear stages read (``--max-positions``), and every
  slot under ``--min-support`` is frozen; the rank is read on the first
  ``--rank-positions`` rows. A support-frozen slot in a table whose level is
  free, or a free slot absent from the rank sample, stops the run before K is
  fitted. ``run.json`` records the rows counted, the threshold, the frozen
  slots with their counts and the free linear count.
* **It builds its own binaries** from that tree, a traced tuner and a release
  engine, in ``--build-dir``, and never touches a production binary. The
  release engine must reproduce ``--expect-bench`` before anything is fitted.
* **The model is the engine's.** ``basilisk-texel --verify`` passes on
  validation before the fit and again on the baked build after it.
* **The seed** is the compiled header's vector, or ``--init FILE``: a
  complete vector whose root is this header (``bake.check_dump``), such as
  ``seed_sf16.py``'s.
* **K is fitted once**, on validation at the seed, and frozen for every stage;
  or ``--K`` gives it, so a matched arm reuses another corpus's K.
* **The stages run in order**, each from the vector the last one wrote
  (``--init``), never through a bake: by default king safety, the linear
  surface, king safety again, and a linear polish at a quarter of the rate
  and twice the patience. ``--min-delta`` reaches both linear stages. Each
  stage's settings are recorded. A stage the tuner calls unconverged (exit 3)
  stops the run.
* **The result is checked as built.** The final vector is baked into
  ``src/eval_rb_params.h`` and both binaries rebuilt. The rebuilt tuner must hold
  that vector exactly, ``--verify`` must pass, and the new bench, the played
  scores' hash on validation and ``test_eval`` are recorded. A test that
  fails on the new weights is reported, never edited here: its fixture is
  fixed in its own commit.
* **The tree is left as it was found.** ``src/eval_rb_params.h`` is restored
  byte for byte on success and on failure. The baked header and its diff stay
  in the run directory for review and commit.

The held-out reads: the loss at the frozen K by phase band, on validation,
and on the train rows the fit did not read (``--max-positions`` short of the
file). Every loss record carries the rows served at the lazy checkpoint, one
of the magnitude contract's reads (C-97).
"""

from __future__ import annotations

import argparse
import datetime
import difflib
import hashlib
import json
import os
import platform
import re
import shutil
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(Path(__file__).resolve().parent))
import bake  # noqa: E402
HEADER = ROOT / "src" / "eval_rb_params.h"
SURFACE = ROOT / "tools" / "texel" / "surface_rebuild_v1.tsv"
SCHEMA = "basilisk-texel-fit-v1"
ROWS_SCHEMA = "basilisk-texel-rows-v1"
EXE = ".exe" if os.name == "nt" else ""


class FitError(RuntimeError):
    pass


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest().upper()


class Run:
    """The run directory: a log per step and run.json, written as it goes."""

    def __init__(self, out: Path) -> None:
        if out.exists() and any(out.iterdir()):
            raise FitError(f"{out} exists and is not empty; a run never overwrites another")
        out.mkdir(parents=True, exist_ok=True)
        self.out = out
        self.record: dict = {"schema": SCHEMA, "started_utc": now(), "steps": []}
        self.step_no = 0

    def save(self) -> None:
        (self.out / "run.json").write_text(json.dumps(self.record, indent=2) + "\n",
                                          encoding="utf-8", newline="\n")

    def step(self, name: str, command: list[str], allowed=(0,), cwd: Path = ROOT) -> str:
        """Run one command, its output to its own log; its stdout is returned."""
        self.step_no += 1
        log = self.out / f"{self.step_no:02d}-{name}.log"
        print(f"[{self.step_no:02d}] {name}: {' '.join(command)}", flush=True)
        start = time.monotonic()
        done = subprocess.run(command, capture_output=True, text=True, cwd=cwd, check=False)
        log.write_text(f"$ {' '.join(command)}\n\n{done.stdout}\n--- stderr ---\n{done.stderr}",
                       encoding="utf-8", newline="\n")
        self.record["steps"].append({"name": name, "command": command, "exit": done.returncode,
                                     "seconds": round(time.monotonic() - start, 1),
                                     "log": log.name})
        self.save()
        if done.returncode not in allowed:
            raise FitError(f"{name} exited {done.returncode}; see {log}")
        return done.stdout


def now() -> str:
    return datetime.datetime.now(datetime.timezone.utc).isoformat(timespec="seconds")


def git(*args: str) -> str:
    return subprocess.run(["git", *args], capture_output=True, text=True, cwd=ROOT,
                          check=True).stdout.strip()


def check_corpus(corpus: Path) -> dict:
    """The corpus manifest, its files rehashed, its parse errors zero."""
    manifest = json.loads((corpus / "manifest.json").read_text(encoding="utf-8"))
    if manifest.get("schema") != ROWS_SCHEMA:
        raise FitError(f"{corpus} is not a {ROWS_SCHEMA} corpus")
    if manifest["games_rejected"].get("parse_error", 0):
        raise FitError(f"{corpus}: {manifest['games_rejected']['parse_error']} games did not parse")
    for split in ("train", "validation"):
        published = manifest["output"][split]
        actual = sha256(corpus / published["file"])
        if actual != published["sha256"]:
            raise FitError(f"{corpus / published['file']} hashes {actual}, its manifest "
                           f"published {published['sha256']}")
    return manifest


def configure_and_build(run: Run, build: Path, texel: bool) -> Path:
    tree = build / ("texel" if texel else "release")
    command = ["cmake", "--preset", "release", "-B", str(tree), "-DCOMP=clang",
               f"-DTEXEL={'ON' if texel else 'OFF'}"]
    run.step(f"configure-{tree.name}", command)
    target = "basilisk-texel" if texel else "basilisk"
    extra = [] if texel else ["test_eval"]
    run.step(f"build-{tree.name}", ["cmake", "--build", str(tree), "--target", target, *extra])
    return tree


def bench(run: Run, engine: Path, name: str) -> int:
    out = run.step(name, [str(engine), "bench", "13"])
    for line in out.splitlines():
        if line.startswith("Nodes searched"):
            return int(line.split(":")[1].strip().replace(",", ""))
    raise FitError(f"{name}: no `Nodes searched` line")


def read_vector(path: Path) -> list[tuple[str, int, int]]:
    out = []
    for line in path.read_text(encoding="utf-8").splitlines():
        if line.startswith("#") or not line.strip():
            continue
        name, index, value = line.split()
        out.append((name, int(index), int(value)))
    return out


def registry_vector(run: Run, tuner: Path, name: str) -> list[tuple[str, int, int]]:
    out = run.step(name, [str(tuner), "--list-params"])
    rows = []
    for line in out.splitlines()[1:]:
        _, group, sub, seed = line.split("\t")
        rows.append((group, int(sub), int(seed)))
    return rows


ROWS_LINE = re.compile(r"(\d+) rows used \((\d+) at the lazy checkpoint\)")


def parse_loss(text: str) -> dict:
    report: dict = {"bands": {}}
    for line in text.splitlines():
        rows = ROWS_LINE.search(line)
        if rows and "lazy_rows" not in report:
            report["lazy_rows"] = int(rows.group(2))
            report["lazy_share"] = int(rows.group(2)) / int(rows.group(1)) if int(rows.group(1)) else None
        if line.startswith(("rows:", "K:", "K_eg:", "K_mg:", "loss:")):
            key, value = line.split(":")
            report[key] = float(value)
        elif line.startswith("band "):
            name = line.split()[1].rstrip(":")
            report["bands"][name] = line.split(":", 1)[1].strip()
    return report


def load_seed(path: Path, header_text: str) -> list[tuple[str, int, int]]:
    """A seed vector file, checked to be complete and rooted at this header."""
    text = path.read_text(encoding="utf-8")
    errors = bake.check_dump(text, bake.parse_param_list(header_text), header_text)
    if errors:
        raise FitError(f"{path} is not a seed for this header: " + "; ".join(errors))
    return read_vector(path)


def write_seed(path: Path, vector: list[tuple[str, int, int]], header_text: str) -> None:
    """The run's seed, rooted at the header's defaults.

    The tuner carries an `--init` file's `# root` line into every vector it
    writes and otherwise roots the chain at the vector it was given. A seed
    from `--init` is not the header's vector, so without the line the fitted
    vector would not trace back to this header and `bake.py` would refuse it.
    """
    root = bake.vector_hash(bake.header_vector(header_text, bake.parse_param_list(header_text)))
    path.write_text(f"# {bake.VECTOR_SCHEMA}\n# root {root}\n"
                    + "".join(f"{n} {i} {v}\n" for n, i, v in vector),
                    encoding="utf-8", newline="\n")


def rank_command(surface_tool: list[str], tuner: Path, manifest: Path, train: Path, derived: Path,
                 args) -> list[str]:
    """`surface.py rank` on the linear stages' rows (the support rows) and the
    rank sample, named apart."""
    return [*surface_tool, "rank", "--tuner", str(tuner), "--manifest", str(manifest),
            "--corpus", str(train), "--support-positions", str(args.max_positions),
            "--rank-positions", str(args.rank_positions), "--min-support", str(args.min_support),
            "--out", str(derived)]


FROZEN_REASON = re.compile(r"frozen: (\d+) rows of support, under (\d+), on (\d+) rows of ")


def support_record(derived: Path) -> dict:
    """The derived manifest's support freeze, for ``run.json``."""
    lines = [line.split("\t") for line in derived.read_text(encoding="utf-8").splitlines()[1:]]
    head, rows = lines[0], lines[1:]
    col = {name: k for k, name in enumerate(head)}
    frozen, counted, threshold = [], set(), set()
    for row in rows:
        match = FROZEN_REASON.match(row[col["reason"]])
        if match:
            frozen.append({"slot": f"{row[col['group']]}[{row[col['sub']]}]", "support": int(match.group(1))})
            threshold.add(int(match.group(2)))
            counted.add(int(match.group(3)))
    if len(counted) > 1 or len(threshold) > 1:
        raise FitError(f"{derived} froze on more than one row count or threshold")
    return {"rows_counted": counted.pop() if counted else None,
            "min_support": threshold.pop() if threshold else None,
            "frozen": frozen,
            "free_linear": sum(1 for row in rows
                               if row[col["class"]] == "free" and row[col["stage"]] == "linear")}


def stage_command(stage: str, tuner: Path, common: list[str], train: Path, validation: Path,
                  out: Path, K: float | tuple[float, float], args) -> tuple[list[str], dict]:
    """One stage's tuner command and the settings it runs with."""
    k_args = ["--K-phase", f"{K[0]:.6f},{K[1]:.6f}"] if isinstance(K, tuple) else ["--K", str(K)]
    if stage == "kingsafety":
        settings = {"max_positions": args.ks_positions}
        return ([str(tuner), *common, "--tune-kingsafety", str(train), str(validation), str(out),
                 *k_args, "--max-positions", str(args.ks_positions)], settings)
    settings = {
        "lr": args.lr / 4 if stage == "polish" else args.lr,
        "patience": args.patience * 2 if stage == "polish" else args.patience,
        "epochs": args.epochs,
        "min_delta": args.min_delta,
        "max_positions": args.max_positions,
        "l2": args.l2,
        "smooth": args.smooth,
    }
    command = [str(tuner), *common, "--tune", "surface", str(train), str(validation), str(out),
               *k_args, "--epochs", str(settings["epochs"]), "--lr", str(settings["lr"]),
               "--patience", str(settings["patience"]), "--max-positions", str(settings["max_positions"]),
               "--l2", str(settings["l2"]), "--smooth", str(settings["smooth"])]
    if settings["min_delta"] > 0:
        command += ["--min-delta", str(settings["min_delta"])]
    return command, settings


def stages(spec: str) -> list[str]:
    names = [s.strip() for s in spec.split(",") if s.strip()]
    for name in names:
        if name not in ("kingsafety", "linear", "polish"):
            raise FitError(f"unknown stage {name!r}")
    return names


def fit(args, run: Run, header: Path, surface: Path, k_mode: str, relevel: bool) -> None:
    rec = run.record
    # ---- Inputs -------------------------------------------------------------
    if git("status", "--porcelain", "--untracked-files=no"):
        raise FitError("the tree has uncommitted changes; a fit runs from a commit")
    rec["source_revision"] = git("rev-parse", "HEAD")
    rec["host"] = {"python": platform.python_version(), "platform": platform.platform()}
    rec["k_mode"] = k_mode
    rec["relevel_material"] = relevel
    corpus = check_corpus(args.corpus)
    rec["corpus"] = {"dir": str(args.corpus), "manifest_sha256": sha256(args.corpus / "manifest.json"),
                     "output": corpus["output"]}
    rec["surface"] = {"base": str(surface), "sha256": sha256(surface)}
    rec["header"] = str(header.relative_to(ROOT))
    rec["header_sha256"] = sha256(header)
    run.save()

    texel_tree = configure_and_build(run, args.build_dir, texel=True)
    release_tree = configure_and_build(run, args.build_dir, texel=False)
    tuner = texel_tree / f"basilisk-texel{EXE}"
    engine = release_tree / f"basilisk{EXE}"
    rec["binaries"] = {"tuner_sha256": sha256(tuner), "engine_sha256": sha256(engine)}
    seed_bench = bench(run, engine, "bench-seed")
    rec["bench_seed"] = seed_bench
    if args.expect_bench and seed_bench != args.expect_bench:
        raise FitError(f"the release build benches {seed_bench}, not {args.expect_bench}")
    run.save()

    surface_tool = [sys.executable, str(ROOT / "tools" / "texel" / "surface.py")]
    run.step("surface-check", [*surface_tool, "check", "--tuner", str(tuner), "--manifest", str(surface)])
    derived = run.out / "surface.tsv"
    run.step("surface-rank", rank_command(surface_tool, tuner, surface, args.corpus / "train.csv",
                                          derived, args))
    rec["surface"]["derived"] = {"file": derived.name, "sha256": sha256(derived)}
    rec["surface"]["support"] = support_record(derived)
    train = args.corpus / corpus["output"]["train"]["file"]
    validation = args.corpus / corpus["output"]["validation"]["file"]
    run.step("verify-seed", [str(tuner), "--verify", str(validation), "--max-positions", "0"])

    seed = run.out / "00-seed.txt"
    registry = registry_vector(run, tuner, "seed-vector")
    header_text = header.read_text(encoding="utf-8")
    if args.init:
        vector = load_seed(args.init, header_text)
        if [(n, i) for n, i, _ in vector] != [(n, i) for n, i, _ in registry]:
            raise FitError(f"{args.init} does not list the tuner's slots in its order")
        rec["seed"] = {"source": str(args.init.resolve()), "sha256": sha256(args.init)}
    else:
        vector = registry
        rec["seed"] = {"source": "the compiled header"}
    write_seed(seed, vector, header_text)
    if k_mode == "phase":
        if args.K_phase:
            loss = parse_loss(run.step("K-given", [str(tuner), "--init", str(seed), "--loss",
                                                   str(validation), "--K-phase", args.K_phase]))
            rec["K_source"] = "given (--K-phase)"
        else:
            loss = parse_loss(run.step("K-fit", [str(tuner), "--init", str(seed), "--loss",
                                                 str(validation), "--fit-k-phase"]))
            rec["K_source"] = "fitted linear in phase on validation at the seed"
        K_val: float | tuple[float, float] = (loss["K_eg"], loss["K_mg"])
        rec["K_eg"], rec["K_mg"] = K_val
        k_loss_args = ["--K-phase", f"{K_val[0]:.6f},{K_val[1]:.6f}"]
    else:
        if args.K:
            loss = parse_loss(run.step("K-given", [str(tuner), "--init", str(seed), "--loss",
                                                   str(validation), "--K", str(args.K)]))
            rec["K_source"] = "given (--K)"
        else:
            loss = parse_loss(run.step("K-fit", [str(tuner), "--init", str(seed), "--loss", str(validation)]))
            rec["K_source"] = "fitted on validation at the seed"
        K_val = loss["K"]
        rec["K"] = K_val
        k_loss_args = ["--K", str(K_val)]
    rec["seed_loss"] = loss
    rec["played_hash_seed"] = run.step("played-seed", [str(tuner), "--init", str(seed),
                                                       "--played-hash", str(validation)]).split()[-1]
    run.save()

    # ---- Stages -------------------------------------------------------------
    previous = seed
    rec["stages"] = []
    for number, stage in enumerate(stages(args.stages), start=1):
        out = run.out / f"{number:02d}-{stage}.txt"
        common = ["--init", str(previous), "--surface", str(derived)]
        command, settings = stage_command(stage, tuner, common, train, validation, out, K_val, args)
        run.step(f"stage-{number}-{stage}", command)
        rec["stages"].append({"stage": stage, "settings": settings, "vector": out.name,
                              "sha256": sha256(out)})
        run.save()
        previous = out
    final = previous

    # ---- Held-out reads -----------------------------------------------------
    rec["final_loss"] = parse_loss(run.step("loss-validation", [str(tuner), "--init", str(final), "--loss",
                                                                 str(validation), *k_loss_args]))
    train_rows = corpus["output"]["train"]["rows"]
    if args.max_positions and train_rows > args.max_positions:
        rec["residuals_unused_train"] = parse_loss(run.step(
            "loss-unused-train", [str(tuner), "--init", str(final), "--loss", str(train), *k_loss_args,
                                  "--skip", str(args.max_positions)]))
    run.save()

    # ---- Bake, rebuild, check as built --------------------------------------
    played_final = run.step("played-final", [str(tuner), "--init", str(final), "--played-hash",
                                             str(validation)]).split()[-1]
    rec["played_hash_final"] = played_final
    bake_source = final
    if relevel:
        root_hash, seed_hash = bake.dump_root_and_seed(final.read_text(encoding="utf-8"))
        releveled_vec, shifts = bake.relevel_material(read_vector(final))
        releveled = run.out / "05-releveled.txt"
        releveled.write_text(bake.format_vector(releveled_vec, root_hash, seed_hash),
                             encoding="utf-8", newline="\n")
        rec["relevel_shifts"] = shifts
        rec["releveled_vector"] = {"vector": releveled.name, "sha256": sha256(releveled)}
        played_releveled = run.step("played-releveled", [str(tuner), "--init", str(releveled),
                                                         "--played-hash", str(validation)]).split()[-1]
        rec["played_hash_releveled"] = played_releveled
        if played_releveled != played_final:
            raise FitError(f"material re-levelling changed played hash: {played_final} -> {played_releveled}")
        bake_source = releveled

    run.step("bake", [sys.executable, str(ROOT / "tools" / "texel" / "bake.py"), str(bake_source),
                      "--header", str(header), "--allow-pst"])
    if os.environ.get("BASILISK_FIT_FAIL_AFTER_BAKE"):
        raise FitError("BASILISK_FIT_FAIL_AFTER_BAKE is set (the restore path's test)")
    baked = run.out / header.name
    shutil.copyfile(header, baked)
    rec["baked_header_sha256"] = sha256(baked)
    configure_and_build(run, args.build_dir, texel=True)
    configure_and_build(run, args.build_dir, texel=False)
    if registry_vector(run, tuner, "baked-vector") != read_vector(bake_source):
        raise FitError("the rebuilt tuner does not hold the fitted vector")
    run.step("verify-baked", [str(tuner), "--verify", str(validation), "--max-positions", "0"])
    rec["bench_baked"] = bench(run, engine, "bench-baked")
    rec["played_hash_baked"] = run.step("played-baked", [str(tuner), "--played-hash",
                                                         str(validation)]).split()[-1]
    if rec["played_hash_baked"] != played_final:
        raise FitError(f"baked played hash {rec['played_hash_baked']} != pre-bake {played_final}")
    test_eval = run.step("test-eval-baked", [str(release_tree / f"test_eval{EXE}")], allowed=(0, 1))
    rec["test_eval_baked"] = test_eval.strip().splitlines()[-1] if test_eval.strip() else ""
    run.save()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--corpus", type=Path, required=True, help="a basilisk-texel-rows-v1 directory")
    parser.add_argument("--out", type=Path, required=True, help="the run directory, new or empty")
    parser.add_argument("--surface", type=Path, default=None)
    parser.add_argument("--build-dir", type=Path, default=ROOT / "build" / "fit")
    parser.add_argument("--rebuild", action="store_true",
                        help="accepted for the C.3.4 launchers; the evaluation has one form since C.3.5")
    parser.add_argument("--relevel-material", action=argparse.BooleanOptionalAction, default=True,
                        help="fold PST live-square means into MgVal/EgVal at bake (default on)")
    parser.add_argument("--expect-bench", type=int, default=0,
                        help="the seed's bench 13, checked before anything is fitted")
    parser.add_argument("--stages", default="kingsafety,linear,kingsafety,polish")
    parser.add_argument("--init", type=Path,
                        help="the seed vector (complete, rooted at this header); default the header's")
    parser.add_argument("--k-mode", choices=("single", "phase"), default=None,
                        help="single constant K (default) or linear K(phase)")
    parser.add_argument("--K", type=float, default=0.0,
                        help="freeze this K instead of fitting it at the seed")
    parser.add_argument("--K-phase", default="",
                        help="freeze linear K(phase) as K_eg,K_mg instead of fitting it")
    parser.add_argument("--min-delta", type=float, default=0.0,
                        help="a gain counts for the linear stages' patience only above this")
    parser.add_argument("--epochs", type=int, default=400)
    parser.add_argument("--lr", type=float, default=1.0)
    parser.add_argument("--patience", type=int, default=20)
    parser.add_argument("--l2", type=float, default=0.0)
    parser.add_argument("--smooth", type=float, default=0.0)
    parser.add_argument("--max-positions", type=int, default=5_000_000)
    parser.add_argument("--ks-positions", type=int, default=100_000)
    parser.add_argument("--rank-positions", type=int, default=300_000,
                        help="the rank sample; support is counted on the linear stages' rows")
    parser.add_argument("--min-support", type=int, default=200)
    args = parser.parse_args()
    args.corpus = args.corpus.resolve()
    header = HEADER
    surface = (args.surface if args.surface is not None else SURFACE).resolve()
    args.build_dir = args.build_dir.resolve()
    if args.init:
        args.init = args.init.resolve()
        if not args.init.is_file():
            parser.error(f"seed vector not found: {args.init}")
    if args.K < 0 or args.min_delta < 0:
        parser.error("--K and --min-delta cannot be negative")
    if args.K and (args.K_phase or args.k_mode == "phase"):
        parser.error("--K cannot be combined with --K-phase or --k-mode phase")
    if args.K_phase and args.k_mode == "single":
        parser.error("--K-phase cannot be combined with --k-mode single")
    k_mode = "phase" if (args.k_mode == "phase" or args.K_phase) else "single"
    relevel = args.relevel_material

    try:
        run = Run(args.out.resolve())
    except FitError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 2
    original = header.read_bytes()
    status = 0
    try:
        stages(args.stages)
        fit(args, run, header, surface, k_mode, relevel)
        run.record["result"] = "fitted"
    except (FitError, subprocess.CalledProcessError, OSError, ValueError, KeyError) as exc:
        run.record["result"] = f"failed: {exc}"
        print(f"ERROR: {exc}", file=sys.stderr)
        status = 1
    finally:
        if header.read_bytes() != original:
            baked = header.read_bytes()
            header.write_bytes(original)
            rel = str(header.relative_to(ROOT)).replace("\\", "/")
            diff = difflib.unified_diff(original.decode("utf-8").splitlines(keepends=True),
                                        baked.decode("utf-8").splitlines(keepends=True),
                                        f"a/{rel}", f"b/{rel}")
            (run.out / f"{header.stem}.diff").write_text("".join(diff), encoding="utf-8", newline="")
        run.record["header_restored"] = header.read_bytes() == original
        run.record["finished_utc"] = now()
        run.save()
    if not run.record["header_restored"]:
        print(f"ERROR: {header.relative_to(ROOT)} could not be restored", file=sys.stderr)
        return 2
    print(f"{run.record['result']}; {header.relative_to(ROOT)} restored; {run.out / 'run.json'}")
    return status


if __name__ == "__main__":
    raise SystemExit(main())
