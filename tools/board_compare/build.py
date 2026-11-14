#!/usr/bin/env python3
"""Build the other engines' cross-engine-board-v2 harnesses from pinned commits.

Each engine is cloned from its local checkout into tools/results/board-compare/
(never built in place: the checkouts are other people's working trees), checked
out at the pinned commit, patched with the adapter recorded beside this file,
and built with the contract's flags (CONTRACT.md, *Builds*). The bench binary
is copied to tools/results/board-compare/bin/<engine>-board-v2.exe beside a
manifest of its source, toolchain, flags and hash, and its preflight is run.

    python tools/board_compare/build.py [--engine rarog] [--engine reckless]

The pins and the adapter patches are the record; a rebuild on another commit
is another arm and must say so.
"""

from __future__ import annotations

import argparse
import hashlib
import os
import shutil
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
WORK = REPO / "tools" / "results" / "board-compare"
POSITIONS = REPO / "tests" / "data" / "board_micro_positions.fen"

ENGINES = {
    "rarog": {
        "checkout": Path("D:/code/rarog"),
        "commit": "611307e",
        "patch": HERE / "rarog" / "board_xv2.diff",
        "bench": "board_xv2",
        "cargo": ["--locked"],
        "rustflags": "-C target-cpu=native --cfg rarog_pext",
        "env": {},
    },
    "reckless": {
        "checkout": Path("D:/code/Reckless"),
        "commit": "8d7d1dd",
        "patch": HERE / "reckless" / "reckless-board-adapter.patch",
        "bench": "board_v2",
        "cargo": ["--locked", "--no-default-features", "--features", "board-bench"],
        "rustflags": "-C target-cpu=native",
        # Its build script downloads the network when none is present; the
        # bench never loads it, so point it at the local copy instead.
        "env": {"EVALFILE": "D:/code/Reckless/networks/v60-7f587dfb.nnue"},
    },
}


def run(cmd: list[str], cwd: Path, env: dict | None = None) -> str:
    out = subprocess.run(cmd, cwd=cwd, env=env, capture_output=True, text=True)
    if out.returncode != 0:
        raise SystemExit(f"{' '.join(cmd)} failed in {cwd}:\n{out.stdout}\n{out.stderr}")
    return out.stdout


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest().upper()


def build(name: str, spec: dict) -> None:
    src = WORK / "src" / name
    if src.exists():
        # Git marks its object files read-only, which Windows will not delete.
        def writable_then_retry(func, path, _exc):
            os.chmod(path, 0o700)
            func(path)
        shutil.rmtree(src, onerror=writable_then_retry)
    src.parent.mkdir(parents=True, exist_ok=True)
    run(["git", "clone", "--quiet", "--no-hardlinks", str(spec["checkout"]), str(src)], WORK)
    run(["git", "checkout", "--quiet", "--detach", spec["commit"]], src)
    run(["git", "apply", "--check", str(spec["patch"])], src)
    run(["git", "apply", str(spec["patch"])], src)
    revision = run(["git", "rev-parse", "HEAD"], src).strip()

    env = dict(os.environ, RUSTFLAGS=spec["rustflags"], **spec["env"])
    target = WORK / "target" / name
    run(["cargo", "bench", *spec["cargo"], "--bench", spec["bench"], "--no-run",
               "--message-format=short", "--target-dir", str(target)], src, env)
    # cargo prints the bench executable it built on stderr; find the newest.
    candidates = sorted(target.rglob(f"{spec['bench']}-*.exe"), key=lambda p: p.stat().st_mtime)
    if not candidates:
        raise SystemExit(f"no {spec['bench']} executable under {target}")
    exe = candidates[-1]
    out = WORK / "bin" / f"{name}-board-v2.exe"
    out.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(exe, out)

    preflight = subprocess.run([str(out), str(POSITIONS), "--preflight-only"],
                               capture_output=True, text=True)
    rustc = run(["rustc", "-vV"], src)
    manifest = [
        f"engine: {name}",
        f"checkout: {spec['checkout']}",
        f"revision: {revision}",
        f"patch: {spec['patch'].relative_to(REPO).as_posix()}",
        f"patch_sha256: {sha256(spec['patch'])}",
        f"rustflags: {spec['rustflags']}",
        f"cargo: bench {' '.join(spec['cargo'])} --bench {spec['bench']}",
        *(f"env {k}: {v}" for k, v in spec["env"].items()),
        *(f"rustc {line}" for line in rustc.splitlines() if line.split(":")[0] in
          ("release", "LLVM version", "host")),
        f"binary_sha256: {sha256(out)}",
        f"preflight_exit: {preflight.returncode}",
    ]
    out.with_suffix(".manifest.txt").write_text("\n".join(manifest) + "\n" + preflight.stdout,
                                                encoding="utf-8")
    status = "PASS" if preflight.returncode == 0 and "preflight: PASS" in preflight.stdout else "FAIL"
    print(f"{name}: {out} ({sha256(out)[:16]}...), preflight {status}")
    if status != "PASS":
        print(preflight.stdout + preflight.stderr, file=sys.stderr)
        raise SystemExit(1)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--engine", action="append", choices=sorted(ENGINES))
    args = parser.parse_args()
    for name in args.engine or sorted(ENGINES):
        build(name, ENGINES[name])
    return 0


if __name__ == "__main__":
    sys.exit(main())
