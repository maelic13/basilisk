"""Allocator, copy and lock calls reachable from the search's per-node code.

The allocation guard (tests/test_allocation_guard.cpp) counts allocations at
run time over whole searches; this reads a binary instead. It disassembles the
engine with llvm-objdump, builds the direct call graph (calls and tail jumps
to other functions, imports included) and walks it from the per-node roots:
`negamax` at PV and non-PV nodes, quiescence and the evaluation. The root's
own instantiation is left out: it runs once per root move per iteration.

Every reachable allocator, release, memcpy-family or lock function is a hit,
once per direct caller, with the call path that reaches it. Optimised code
inlines its callees, so each call site on the path is also given its inline
chain from the binary's line tables (llvm-symbolizer --inlines); read the
shipped code with them, a `-DPROFILE_SYMBOLS=ON` PGO build, whose code
generation is unchanged. A runtime guard is invisible here (code reached only
at the root, or once per table), so the paths known to be guarded are listed
in KNOWN with their reasons; each matches the whole path with its inline
chains, not the function alone, so a new caller of the same allocator still
fails. Error paths (throws, aborts) are not followed. Indirect calls cannot
be followed; their count in the reachable code is reported so a reader knows
what the walk could not see.

    python tools/diag/hot_path_calls.py build/b82-psym/basilisk.exe
    python tools/diag/hot_path_calls.py <exe> --json out.json

Exit status: 0 when every hit is KNOWN or --allow'ed, 1 when one is not, 2
when the binary has no symbol for a root.
"""
from __future__ import annotations

import argparse
import json
import re
import shutil
import subprocess
import sys
from collections import deque
from pathlib import Path

ROOTS = (r"^int Searcher::negamax<\(NodeType\)[12]>\(", r"^int Searcher::quiescence<",
         r"^Evaluator::evaluate_full\(")
# Allocation, release, bulk copies and locks. memset is left out: zeroing a
# buffer is no allocation and the kernel clears fixed records with it.
FORBIDDEN = (r"operator new", r"operator delete", r"^(__imp_)?_?(malloc|calloc|realloc|free)$",
             r"^(__imp_)?_aligned_(malloc|free)$", r"^(__imp_)?mem(cpy|move)$",
             r"mutex::(try_)?lock\(", r"(EnterCriticalSection|AcquireSRWLock\w*|pthread_mutex_lock)$")
# Error paths, not followed: what they reach runs only when something throws
# or the runtime aborts.
TERMINAL = (r"__throw_", r"^(__imp_)?__cxa_(allocate_exception|throw|free_exception|rethrow)",
            r"^(__imp_)?__cxa_guard_abort", r"__abort_message", r"^(__imp_)?_Unwind_",
            r"std::terminate", r"^(__imp_)?abort$")
# Paths that reach a forbidden function but never run per node. Each is
# searched in the path written `a -> b [x <- y] -> c`, where the bracket is the
# inline chain at the call site, innermost first, in the symbolizer's short
# names; a pattern accepts the frames in either order because a frame is a
# call in one build and inlined in the next.
KNOWN = (
    (r"make_move\b.*UndoInfo|UndoInfo.*\bmake_move\b",
     "the undo stack's growth path: a search's board is a copy that reserves "
     "the line's headroom first, so it never grows during a search"),
    (r"\bordering_score\b.*mutex::lock\(|mutex::lock\(.*\bordering_score\b",
     "the shared root table's lock, read only when the picker has root ordering "
     "(at the root)"),
    (r"kpk_strong_wins\b.*__call_once\b|__call_once\b.*kpk_strong_wins\b",
     "the KPK bitbase's one-time build; later calls take call_once's flag check"),
    (r"Syzygy::probe_\w+\(.*(init_table\b|probe_table\b.*mutex::lock\()",
     "a tablebase's lazy first load, once per table: Fathom locks and opens it "
     "only while its ready flag is unset"),
)

FUNCTION_RE = re.compile(r"^([0-9a-f]+) <(.+)>:$")
ADDRESS_RE = re.compile(r"^\s*([0-9a-f]+):")
CALL_RE = re.compile(r"\b(call|callq|jmp|jmpq)\b\s+(.*)$")
TARGET_RE = re.compile(r"<([^<>]+(?:<[^<>]*(?:<[^<>]*>[^<>]*)*>[^<>]*)*)>\s*$")
INDIRECT_RE = re.compile(r"\b(call|callq)\b\s+\*%")


def find_tool(explicit: str | None, name: str) -> str:
    """An LLVM tool: the one named, or the first on PATH."""
    if explicit:
        return explicit
    for candidate in (name, name + ".exe"):
        path = shutil.which(candidate)
        if path:
            return path
    raise SystemExit(f"{name} not found; pass its path")


def base_name(target: str) -> str:
    """A call target without its `+0x..` offset."""
    return re.sub(r"\+0x[0-9a-f]+$", "", target)


def parse(disassembly: str):
    """Each function's direct callees, the address of each call site, and
    each function's count of indirect calls."""
    edges: dict[str, set[str]] = {}
    sites: dict[tuple[str, str], list[str]] = {}
    indirect: dict[str, int] = {}
    current = None
    for line in disassembly.splitlines():
        m = FUNCTION_RE.match(line)
        if m:
            current = m.group(2)
            edges.setdefault(current, set())
            indirect.setdefault(current, 0)
            continue
        if current is None:
            continue
        c = CALL_RE.search(line)
        if not c:
            continue
        if INDIRECT_RE.search(line) and "<" not in line:
            indirect[current] += 1
            continue
        t = TARGET_RE.search(line)
        if not t:
            if c.group(1).startswith("call"):
                indirect[current] += 1
            continue
        target = base_name(t.group(1))
        if target in ("__guard_dispatch_icall_fptr", "__guard_check_icall_fptr"):
            indirect[current] += 1
            continue
        if target != current:
            edges[current].add(target)
            a = ADDRESS_RE.match(line)
            if a:
                sites.setdefault((current, target), []).append("0x" + a.group(1))
    return edges, sites, indirect


def walk(edges: dict[str, set[str]], roots: list[str], forbidden: list[re.Pattern],
         terminal: list[re.Pattern]) -> tuple[list[list[str]], set[str]]:
    """Breadth-first from the roots. Each (caller, forbidden callee) pair is a
    hit, with the shortest path to the caller, so two callers of one allocator
    are two hits."""
    parent: dict[str, str | None] = {r: None for r in roots}
    queue = deque(roots)
    hits: list[list[str]] = []

    def path_to(at: str | None) -> list[str]:
        path = []
        while at is not None:
            path.append(at)
            at = parent[at]
        return list(reversed(path))

    while queue:
        node = queue.popleft()
        if any(p.search(node) for p in terminal):
            continue
        for callee in sorted(edges.get(node, ())):
            if any(p.search(callee) for p in forbidden):
                # Not descended: the forbidden function's own callees are not
                # per-node code's.
                hits.append(path_to(node) + [callee])
            elif callee not in parent:
                parent[callee] = node
                queue.append(callee)
    return hits, set(parent)


def inline_chains(symbolizer: str, binary: Path, addresses: list[str]) -> dict[str, list[str]]:
    """The inline chain at each address, innermost function first; empty where
    the binary has no line tables."""
    if not addresses:
        return {}
    proc = subprocess.run([symbolizer, "--inlines", f"--obj={binary}"],
                          input="\n".join(addresses) + "\n", capture_output=True, text=True,
                          errors="replace")
    chains: dict[str, list[str]] = {}
    blocks = [b for b in proc.stdout.split("\n\n") if b.strip()]
    for address, block in zip(addresses, blocks):
        lines = block.strip().splitlines()
        functions = lines[0::2]
        locations = lines[1::2]
        if all(loc.startswith("??") for loc in locations):
            chains[address] = []
        else:
            chains[address] = functions
    return chains


def render(path: list[str], sites, chains) -> str:
    """The path as `a -> b [x <- y] -> c`, the bracket being the inline chain
    at the first call site of each step, where it adds a frame."""
    out = [path[0]]
    for caller, callee in zip(path, path[1:]):
        site = sites.get((caller, callee), [None])[0]
        chain = chains.get(site, []) if site else []
        inner = [f for f in chain if f != caller]
        out.append(f"{callee}" if not inner else f"{callee} [{' <- '.join(inner)}]")
    return " -> ".join(out)


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("binary", type=Path)
    ap.add_argument("--objdump", default=None)
    ap.add_argument("--symbolizer", default=None)
    ap.add_argument("--allow", action="append", default=[], metavar="REGEX",
                    help="a path (as printed) this run accepts; its reason belongs in the "
                         "record that cites the run")
    ap.add_argument("--json", type=Path, default=None)
    args = ap.parse_args(argv)

    proc = subprocess.run([find_tool(args.objdump, "llvm-objdump"), "-d", "-C",
                           "--no-show-raw-insn", str(args.binary)],
                          capture_output=True, text=True, errors="replace")
    if proc.returncode != 0:
        raise SystemExit(f"llvm-objdump failed: {proc.stderr.strip()[:400]}")
    edges, sites, indirect = parse(proc.stdout)
    roots = [name for name in edges if any(re.search(r, name) for r in ROOTS)]
    missing = [r for r in ROOTS if not any(re.search(r, name) for name in roots)]
    if missing:
        print(f"no symbol for root {missing}: build with symbols", file=sys.stderr)
        return 2
    hits, reached = walk(edges, roots, [re.compile(p) for p in FORBIDDEN],
                         [re.compile(p) for p in TERMINAL])
    wanted = sorted({sites[(a, b)][0] for path in hits for a, b in zip(path, path[1:])
                     if (a, b) in sites})
    chains = inline_chains(find_tool(args.symbolizer, "llvm-symbolizer"), args.binary, wanted)
    line_tables = any(chains.values())

    known = [(re.compile(r), why) for r, why in KNOWN]
    known += [(re.compile(a), "allowed on the command line") for a in args.allow]
    records = []
    for path in sorted(hits):
        text = render(path, sites, chains)
        reason = next((why for pattern, why in known if pattern.search(text)), None)
        records.append({"path": path, "rendered": text, "reason": reason})
    unexpected = sum(r["reason"] is None for r in records)
    unresolved = sum(indirect.get(name, 0) for name in reached)

    print(f"roots {len(roots)}, reachable functions {len(reached)}, indirect calls in them "
          f"{unresolved}, line tables {'yes' if line_tables else 'no (inlined frames unseen)'}")
    for r in records:
        print(f"  {'known' if r['reason'] else 'FORBIDDEN'}: {r['path'][-2]} -> {r['path'][-1]}")
        if r["reason"]:
            print(f"    {r['reason']}")
        print(f"    via {r['rendered']}")
    print(f"{len(records)} hits, {unexpected} not known")
    if args.json:
        args.json.write_text(json.dumps({
            "binary": str(args.binary), "roots": roots, "reachable": len(reached),
            "indirect_calls": unresolved, "line_tables": line_tables, "hits": records,
            "unexpected": unexpected,
        }, indent=2), encoding="utf-8")
    return 1 if unexpected else 0


if __name__ == "__main__":
    sys.exit(main())
