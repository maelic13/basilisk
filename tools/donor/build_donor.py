#!/usr/bin/env python3
"""Rebuild the donor-comparison engines from pinned Stockfish sources (PLAN C.0, BAS-O09).

Four engines answer which classical evaluation is Phase C's donor, each
evaluation played in both searches:

  sf16-stock      Stockfish 16 (`sf_16`) playing its classical evaluation
  sf16-eval9587   sf_16's search with 9587eeeb's whole evaluation
  sf9587-stock    classical Stockfish 9587eeeb (`SF_classical`)
  sf9587-eval16   9587eeeb's search with sf_16's whole classical evaluation

Every build is the tagged Stockfish tree plus one committed patch from this
directory, compiled by calling clang++ directly (no make on this host), with
the same flags for the two engines of a host and no PGO. All four carry an
`evalraw` command that prints the raw value of `Eval::evaluate` for the
current position; `verify_donor.py` uses it to prove a hybrid's evaluation
equals its origin's.

A rebuilt executable is not byte-identical to an earlier one. The identity of
a build is its inputs (the Stockfish commit, the patch's SHA-256, the compiler
and flags) and its bench node count, all written to the manifest; a played
binary is pinned by its own SHA-256 in the launcher.

Usage:
  python tools/donor/build_donor.py --stockfish D:/code/stockfish [--only <name> ...]
                                    [--out tools/test_engines] [--jobs 8]
"""

from __future__ import annotations

import argparse
import concurrent.futures
import datetime
import hashlib
import io
import pathlib
import re
import shutil
import subprocess
import sys
import tarfile
import tempfile

HERE = pathlib.Path(__file__).resolve().parent
REPO = HERE.parents[1]

COMMITS = {
    "sf_16": "68e1e9b3811e16cad014b590d7443b9063b3eb52",
    "SF_classical": "9587eeeb5ed29f834d4f956b92e0e732877c47a7",
}
COMMON_FLAGS = ["-std=c++17", "-O3", "-DNDEBUG", "-DIS_64BIT", "-DUSE_POPCNT", "-DUSE_PEXT",
                "-mbmi2", "-mpopcnt"]
TAIL_FLAGS = ["-flto", "-static", "-pthread"]
# sf_16 compiles its network code even when it plays the classical evaluation;
# the network file itself is left out (NNUE_EMBEDDING_OFF).
SF16_FLAGS = ["-DUSE_AVX2", "-mavx2", "-DUSE_SSE41", "-msse4.1", "-DUSE_SSSE3", "-mssse3",
              "-DUSE_SSE2", "-msse2", "-DNNUE_EMBEDDING_OFF"]
HOSTS = {
    "sf16": {"tag": "sf_16", "flags": COMMON_FLAGS + SF16_FLAGS + TAIL_FLAGS,
             "globs": ["*.cpp", "syzygy/tbprobe.cpp", "nnue/*.cpp", "nnue/features/*.cpp"],
             "bench": "bench 16 1 13 default depth classical"},
    "9587": {"tag": "SF_classical", "flags": COMMON_FLAGS + TAIL_FLAGS,
             "globs": ["*.cpp", "syzygy/tbprobe.cpp"],
             "bench": "bench"},
}
BUILDS = {
    "sf16-stock": "sf16",
    "sf16-eval9587": "sf16",
    "sf9587-stock": "9587",
    "sf9587-eval16": "9587",
}
# The stock 9587eeeb build must reproduce the commit's own signature.
OFFICIAL_BENCH = {"sf9587-stock": 4746616}


class BuildError(Exception):
    pass


def sha256(path: pathlib.Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest().upper()


def run(args: list[str], cwd: pathlib.Path | None = None, stdin: str | None = None) -> subprocess.CompletedProcess:
    return subprocess.run(args, cwd=cwd, input=stdin, capture_output=True, text=True)


def extract_tree(stockfish: pathlib.Path, commit: str, into: pathlib.Path) -> None:
    """The tagged `src/` with its blobs as stored (no end-of-line conversion)."""
    done = subprocess.run(["git", "-C", str(stockfish), "-c", "core.autocrlf=false", "archive",
                           "--format=tar", commit, "src"], capture_output=True)
    if done.returncode != 0:
        raise BuildError(f"git archive {commit} failed: {done.stderr.decode(errors='replace').strip()}")
    with tarfile.open(fileobj=io.BytesIO(done.stdout)) as archive:
        archive.extractall(into, filter="data")


def compile_one(source: pathlib.Path, obj: pathlib.Path, flags: list[str], src: pathlib.Path) -> str:
    done = run(["clang++", *flags, "-c", str(source), "-o", str(obj)], cwd=src)
    return "" if done.returncode == 0 else f"{source.name}: {done.stderr.strip()[-2000:]}"


def bench_nodes(exe: pathlib.Path, command: str) -> int:
    done = run([str(exe)], stdin=command + "\nquit\n")
    match = re.search(r"Nodes searched\s*:\s*(\d+)", done.stdout + done.stderr)
    if done.returncode != 0 or not match:
        raise BuildError(f"{exe.name}: `{command}` printed no node count (exit {done.returncode}).")
    return int(match.group(1))


def build(name: str, stockfish: pathlib.Path, out: pathlib.Path, jobs: int) -> dict:
    host = HOSTS[BUILDS[name]]
    commit = COMMITS[host["tag"]]
    resolved = run(["git", "-C", str(stockfish), "rev-parse", f"{host['tag']}^{{commit}}"]).stdout.strip()
    if resolved != commit:
        raise BuildError(f"{stockfish} resolves {host['tag']} to {resolved or 'nothing'}, not {commit}.")
    patch = HERE / f"{name}.patch"
    if not patch.exists():
        raise BuildError(f"no patch {patch}.")
    if b"\r\n" in patch.read_bytes():
        raise BuildError(f"{patch.name} has CRLF line endings; it is applied byte-exact (.gitattributes -text).")

    # Outside the repository on purpose: `git apply` inside a work tree skips
    # paths outside the current directory without saying so.
    work = pathlib.Path(tempfile.mkdtemp(prefix=f"basilisk-donor-{name}-"))
    try:
        extract_tree(stockfish, commit, work)
        src = work / "src"
        applied = run(["git", "apply", "-p1", "--whitespace=nowarn", str(patch)], cwd=work)
        if applied.returncode != 0:
            raise BuildError(f"{patch.name} did not apply: {applied.stderr.strip()}")
        sources = sorted({path for glob in host["globs"] for path in src.glob(glob)})
        objects = work / "obj"
        objects.mkdir()
        pairs = [(s, objects / ("_".join(s.relative_to(src).parts) + ".o")) for s in sources]
        with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as pool:
            failures = [f for f in pool.map(lambda p: compile_one(p[0], p[1], host["flags"], src), pairs) if f]
        if failures:
            raise BuildError(f"{name}: compile failed:\n" + "\n".join(failures))
        exe = work / f"{name}.exe"
        linked = run(["clang++", *host["flags"], *[str(o) for _, o in pairs], "-o", str(exe)])
        if linked.returncode != 0:
            raise BuildError(f"{name}: link failed: {linked.stderr.strip()[-2000:]}")
        first, second = bench_nodes(exe, host["bench"]), bench_nodes(exe, host["bench"])
        if first != second:
            raise BuildError(f"{name}: `{host['bench']}` is not deterministic ({first} then {second}).")
        if name in OFFICIAL_BENCH and first != OFFICIAL_BENCH[name]:
            raise BuildError(f"{name}: bench {first}, not the commit's {OFFICIAL_BENCH[name]}.")
        out.mkdir(parents=True, exist_ok=True)
        target = out / f"donor-{name}.exe"
        shutil.copy2(exe, target)
        compiler = run(["clang++", "--version"]).stdout.splitlines()[0]
        record = {"name": name, "artifact": str(target), "binary_sha256": sha256(target),
                  "stockfish": f"{host['tag']} ({commit})", "patch": patch.name, "patch_sha256": sha256(patch),
                  "compiler": compiler, "flags": " ".join(host["flags"]),
                  "sources": len(sources), "bench_command": host["bench"], "bench_nodes": first}
        lines = [f"{key + ':':<16}{value}" for key, value in record.items()]
        lines.append(f"{'built_utc:':<16}{datetime.datetime.now(datetime.timezone.utc):%Y-%m-%d %H:%M:%SZ}")
        target.with_suffix(".manifest.txt").write_text("\n".join(lines) + "\n", encoding="utf-8")
        return record
    finally:
        shutil.rmtree(work, ignore_errors=True)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--stockfish", required=True, type=pathlib.Path,
                        help="a Stockfish clone holding the tags sf_16 and SF_classical; never modified")
    parser.add_argument("--only", nargs="+", choices=sorted(BUILDS), default=sorted(BUILDS))
    parser.add_argument("--out", type=pathlib.Path, default=REPO / "tools" / "test_engines")
    parser.add_argument("--jobs", type=int, default=8)
    args = parser.parse_args()
    try:
        for name in args.only:
            record = build(name, args.stockfish, args.out, args.jobs)
            print(f"{name}: bench {record['bench_nodes']}, {record['binary_sha256']}")
    except BuildError as error:
        print(error, file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
