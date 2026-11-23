#!/usr/bin/env python3
"""The Texel fit driver: one corpus, one surface, the stage schedule, provenance.

    python tools/texel/fit.py --corpus DIR --out RUN_DIR [options]

It replaces tools/texel/phase911.ps1. What it guarantees (PLAN C.2.1.4):

* **Its inputs are what they claim.** The tree is clean and its revision
  recorded. The corpus is a basilisk-texel-rows-v1 manifest whose files hash
  as published, with no parse errors. The surface manifest is checked against
  the tuner's registry. Every input and output is hashed in ``run.json``.
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
  ``src/eval_params.h`` and both binaries rebuilt. The rebuilt tuner must hold
  that vector exactly, ``--verify`` must pass, and the new bench, the played
  scores' hash on validation and ``test_eval`` are recorded. A test that
  fails on the new weights is reported, never edited here: its fixture is
  fixed in its own commit.
* **The tree is left as it was found.** ``src/eval_params.h`` is restored
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
HEADER = ROOT / "src" / "eval_params.h"
SURFACE = ROOT / "tools" / "texel" / "surface_v1.tsv"
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
        if line.startswith(("rows:", "K:", "loss:")):
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


def stage_command(stage: str, tuner: Path, common: list[str], train: Path, validation: Path,
                  out: Path, K: float, args) -> tuple[list[str], dict]:
    """One stage's tuner command and the settings it runs with."""
    if stage == "kingsafety":
        settings = {"max_positions": args.ks_positions}
        return ([str(tuner), *common, "--tune-kingsafety", str(train), str(validation), str(out),
                 "--K", str(K), "--max-positions", str(args.ks_positions)], settings)
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
               "--K", str(K), "--epochs", str(settings["epochs"]), "--lr", str(settings["lr"]),
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


def fit(args, run: Run) -> None:
    rec = run.record
    # ---- Inputs -------------------------------------------------------------
    if git("status", "--porcelain", "--untracked-files=no"):
        raise FitError("the tree has uncommitted changes; a fit runs from a commit")
    rec["source_revision"] = git("rev-parse", "HEAD")
    rec["host"] = {"python": platform.python_version(), "platform": platform.platform()}
    corpus = check_corpus(args.corpus)
    rec["corpus"] = {"dir": str(args.corpus), "manifest_sha256": sha256(args.corpus / "manifest.json"),
                     "output": corpus["output"]}
    rec["surface"] = {"base": str(args.surface), "sha256": sha256(args.surface)}
    rec["header_sha256"] = sha256(HEADER)
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
    run.step("surface-check", [*surface_tool, "check", "--tuner", str(tuner), "--manifest", str(args.surface)])
    derived = run.out / "surface.tsv"
    run.step("surface-rank", [*surface_tool, "rank", "--tuner", str(tuner), "--manifest", str(args.surface),
                              "--corpus", str(args.corpus / "train.csv"), "--max-positions",
                              str(args.rank_positions), "--min-support", str(args.min_support),
                              "--out", str(derived)])
    rec["surface"]["derived"] = {"file": derived.name, "sha256": sha256(derived)}
    train = args.corpus / corpus["output"]["train"]["file"]
    validation = args.corpus / corpus["output"]["validation"]["file"]
    run.step("verify-seed", [str(tuner), "--verify", str(validation), "--max-positions", "0"])

    seed = run.out / "00-seed.txt"
    registry = registry_vector(run, tuner, "seed-vector")
    if args.init:
        vector = load_seed(args.init, HEADER.read_text(encoding="utf-8"))
        if [(n, i) for n, i, _ in vector] != [(n, i) for n, i, _ in registry]:
            raise FitError(f"{args.init} does not list the tuner's slots in its order")
        rec["seed"] = {"source": str(args.init.resolve()), "sha256": sha256(args.init)}
    else:
        vector = registry
        rec["seed"] = {"source": "the compiled header"}
    write_seed(seed, vector, HEADER.read_text(encoding="utf-8"))
    if args.K:
        loss = parse_loss(run.step("K-given", [str(tuner), "--init", str(seed), "--loss",
                                               str(validation), "--K", str(args.K)]))
        rec["K_source"] = "given (--K)"
    else:
        loss = parse_loss(run.step("K-fit", [str(tuner), "--init", str(seed), "--loss", str(validation)]))
        rec["K_source"] = "fitted on validation at the seed"
    K = loss["K"]
    rec["K"] = K
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
        command, settings = stage_command(stage, tuner, common, train, validation, out, K, args)
        run.step(f"stage-{number}-{stage}", command)
        rec["stages"].append({"stage": stage, "settings": settings, "vector": out.name,
                              "sha256": sha256(out)})
        run.save()
        previous = out
    final = previous

    # ---- Held-out reads -----------------------------------------------------
    rec["final_loss"] = parse_loss(run.step("loss-validation", [str(tuner), "--init", str(final), "--loss",
                                                                 str(validation), "--K", str(K)]))
    train_rows = corpus["output"]["train"]["rows"]
    if args.max_positions and train_rows > args.max_positions:
        rec["residuals_unused_train"] = parse_loss(run.step(
            "loss-unused-train", [str(tuner), "--init", str(final), "--loss", str(train), "--K", str(K),
                                  "--skip", str(args.max_positions)]))
    run.save()

    # ---- Bake, rebuild, check as built --------------------------------------
    run.step("bake", [sys.executable, str(ROOT / "tools" / "texel" / "bake.py"), str(final), "--allow-pst"])
    if os.environ.get("BASILISK_FIT_FAIL_AFTER_BAKE"):
        raise FitError("BASILISK_FIT_FAIL_AFTER_BAKE is set (the restore path's test)")
    baked = run.out / "eval_params.h"
    shutil.copyfile(HEADER, baked)
    rec["baked_header_sha256"] = sha256(baked)
    configure_and_build(run, args.build_dir, texel=True)
    configure_and_build(run, args.build_dir, texel=False)
    if registry_vector(run, tuner, "baked-vector") != read_vector(final):
        raise FitError("the rebuilt tuner does not hold the fitted vector")
    run.step("verify-baked", [str(tuner), "--verify", str(validation), "--max-positions", "0"])
    rec["bench_baked"] = bench(run, engine, "bench-baked")
    rec["played_hash_baked"] = run.step("played-baked", [str(tuner), "--played-hash",
                                                         str(validation)]).split()[-1]
    test_eval = run.step("test-eval-baked", [str(release_tree / f"test_eval{EXE}")], allowed=(0, 1))
    rec["test_eval_baked"] = test_eval.strip().splitlines()[-1] if test_eval.strip() else ""
    run.save()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--corpus", type=Path, required=True, help="a basilisk-texel-rows-v1 directory")
    parser.add_argument("--out", type=Path, required=True, help="the run directory, new or empty")
    parser.add_argument("--surface", type=Path, default=SURFACE)
    parser.add_argument("--build-dir", type=Path, default=ROOT / "build" / "fit")
    parser.add_argument("--expect-bench", type=int, default=0,
                        help="the seed's bench 13, checked before anything is fitted")
    parser.add_argument("--stages", default="kingsafety,linear,kingsafety,polish")
    parser.add_argument("--init", type=Path,
                        help="the seed vector (complete, rooted at this header); default the header's")
    parser.add_argument("--K", type=float, default=0.0,
                        help="freeze this K instead of fitting it at the seed")
    parser.add_argument("--min-delta", type=float, default=0.0,
                        help="a gain counts for the linear stages' patience only above this")
    parser.add_argument("--epochs", type=int, default=400)
    parser.add_argument("--lr", type=float, default=1.0)
    parser.add_argument("--patience", type=int, default=20)
    parser.add_argument("--l2", type=float, default=0.0)
    parser.add_argument("--smooth", type=float, default=0.0)
    parser.add_argument("--max-positions", type=int, default=5_000_000)
    parser.add_argument("--ks-positions", type=int, default=100_000)
    parser.add_argument("--rank-positions", type=int, default=30_000)
    parser.add_argument("--min-support", type=int, default=200)
    args = parser.parse_args()
    args.corpus = args.corpus.resolve()
    args.surface = args.surface.resolve()
    args.build_dir = args.build_dir.resolve()
    if args.init:
        args.init = args.init.resolve()
        if not args.init.is_file():
            parser.error(f"seed vector not found: {args.init}")
    if args.K < 0 or args.min_delta < 0:
        parser.error("--K and --min-delta cannot be negative")

    try:
        run = Run(args.out.resolve())
    except FitError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 2
    original = HEADER.read_bytes()
    status = 0
    try:
        stages(args.stages)
        fit(args, run)
        run.record["result"] = "fitted"
    except (FitError, subprocess.CalledProcessError, OSError, ValueError, KeyError) as exc:
        run.record["result"] = f"failed: {exc}"
        print(f"ERROR: {exc}", file=sys.stderr)
        status = 1
    finally:
        if HEADER.read_bytes() != original:
            baked = HEADER.read_bytes()
            HEADER.write_bytes(original)
            diff = difflib.unified_diff(original.decode("utf-8").splitlines(keepends=True),
                                        baked.decode("utf-8").splitlines(keepends=True),
                                        "a/src/eval_params.h", "b/src/eval_params.h")
            (run.out / "eval_params.diff").write_text("".join(diff), encoding="utf-8", newline="")
        run.record["header_restored"] = HEADER.read_bytes() == original
        run.record["finished_utc"] = now()
        run.save()
    if not run.record["header_restored"]:
        print("ERROR: src/eval_params.h could not be restored", file=sys.stderr)
        return 2
    print(f"{run.record['result']}; src/eval_params.h restored; {run.out / 'run.json'}")
    return status


if __name__ == "__main__":
    raise SystemExit(main())
