#!/usr/bin/env python3
"""
Bake a Texel tuner weight dump back into src/EvalParams.h.

The tuner (`basilisk-texel --tune <group> ... <out.txt>`) writes a full weight
dump, one line per scalar:

    DisplayName  index  value

Only the *active* group's weights actually move; every other line repeats the
current header value. This script rewrites, in place, exactly the EvalParams.h
default initializers whose values DIFFER from the dump -- i.e. precisely the
tuned weights that moved -- and leaves everything else byte-identical. That
keeps the diff minimal and self-limiting: a dump from a `phase45` fit can only
touch phase45 members.

Member resolution comes from the EVAL_PARAM_LIST X-macro in EvalParams.h, so it
stays in sync automatically. Multi-line / 2-D members (only the PSTs:
`pst_mg[0]` ...) are refused unless --allow-pst is given, because their
initializers span many lines and need the dedicated path; for now no Phase-4
stage before 4.7 touches them.

Usage:
    python tools/texel/bake.py <dump.txt> [--header src/EvalParams.h] [--dry-run]
"""
import argparse
import re
import sys
from pathlib import Path


def parse_param_list(header_text):
    """Return ordered list of (display_name, member, length) from EVAL_PARAM_LIST."""
    # Grab the macro body: from 'EVAL_PARAM_LIST(X)' definition to the line that
    # has no trailing backslash.
    m = re.search(r"#define EVAL_PARAM_LIST\(X\)(.*?)(?<!\\)\n", header_text, re.S)
    if not m:
        sys.exit("Could not locate EVAL_PARAM_LIST(X) macro in header.")
    body = m.group(1)
    params = []
    for line in body.splitlines():
        xm = re.search(r"X\(\s*([A-Za-z0-9_]+)\s*,\s*([A-Za-z0-9_\[\]]+)\s*,\s*(\d+)\s*\)", line)
        if xm:
            params.append((xm.group(1), xm.group(2), int(xm.group(3))))
    return params


def parse_dump(dump_text):
    """Return {display_name: [values in index order]}."""
    out = {}
    for line in dump_text.splitlines():
        parts = line.split()
        if len(parts) != 3:
            continue
        name, idx, val = parts
        try:
            idx = int(idx); val = int(val)
        except ValueError:
            continue
        out.setdefault(name, {})[idx] = val
    result = {}
    for name, d in out.items():
        result[name] = [d[i] for i in range(len(d))]
    return result


def current_initializer(header_text, member):
    """Return (match_span, list_of_current_int_values) for a single-line member."""
    # Matches:  int <member> = 0;   or   int <member>[N] = { ... };
    pat = re.compile(
        r"\bint\s+" + re.escape(member) + r"\s*(?:\[\d+\])?\s*=\s*(.*?);",
        re.S,
    )
    m = pat.search(header_text)
    if not m:
        return None, None
    rhs = m.group(1)
    if "{" in rhs:
        inner = rhs[rhs.index("{") + 1: rhs.rindex("}")]
    else:
        inner = rhs
    vals = [int(x) for x in re.findall(r"-?\d+", inner)]
    return m.span(), vals


def format_initializer(member, length, values):
    if length == 1:
        return f"int {member} = {values[0]};"
    body = ", ".join(str(v) for v in values)
    # preserve the [N] dimension exactly
    return f"int {member}[{length}] = {{ {body} }};"


# --- 2-D members (the PSTs: pst_mg[6][64], pst_eg[6][64]) -------------------
# Piece-index -> comment label, matching the hand-written layout in EvalParams.h.
PST_ROW_COMMENTS = ["PAWN", "KNIGHT", "BISHOP", "ROOK", "QUEEN", "KING"]


def find_2d_span(header_text, base):
    """Return (span, [[row0...],[row1...]...]) for `int base[R][C] = { ... };`."""
    m = re.search(r"\bint\s+" + re.escape(base) + r"\s*\[(\d+)\]\[(\d+)\]\s*=\s*\{", header_text)
    if not m:
        return None, None, None, None
    rows, cols = int(m.group(1)), int(m.group(2))
    # Scan from the opening brace to its matching close, then require ';'.
    open_idx = header_text.index("{", m.start())
    depth = 0
    i = open_idx
    while i < len(header_text):
        if header_text[i] == "{":
            depth += 1
        elif header_text[i] == "}":
            depth -= 1
            if depth == 0:
                break
        i += 1
    end = header_text.index(";", i) + 1
    block = header_text[m.start():end]
    nums = [int(x) for x in re.findall(r"-?\d+", block) if True]
    # the first two ints in the match are the dimensions R and C -> drop them
    nums = nums[2:]
    flat = nums[: rows * cols]
    cur = [flat[r * cols:(r + 1) * cols] for r in range(rows)]
    return (m.start(), end), rows, cols, cur


def format_2d_block(base, rows, cols, row_values, comments):
    indent = "    "
    inner = indent + "    "
    lines = [f"{indent}int {base}[{rows}][{cols}] = {{"]
    per_line = 8 if cols % 8 == 0 else cols
    for r in range(rows):
        if r < len(comments):
            lines.append(f"{inner}// {comments[r]}")
        vals = row_values[r]
        chunks = []
        for s in range(0, cols, per_line):
            chunk = ", ".join(f"{v:4d}" for v in vals[s:s + per_line])
            chunks.append(chunk)
        body = (",\n" + inner + "  ").join(chunks)
        trailing = "," if r < rows - 1 else ""
        lines.append(f"{inner}{{ {body} }}{trailing}")
    lines.append(f"{indent}}};")
    return "\n".join(lines)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("dump")
    ap.add_argument("--header", default="src/EvalParams.h")
    ap.add_argument("--dry-run", action="store_true")
    ap.add_argument("--allow-pst", action="store_true",
                    help="permit rewriting the 2-D PST initializers (pst_mg/pst_eg)")
    args = ap.parse_args()

    header_path = Path(args.header)
    header_text = header_path.read_text(encoding="utf-8")
    params = parse_param_list(header_text)
    dump = parse_dump(Path(args.dump).read_text(encoding="utf-8"))

    changed = []          # (member, old, new) for scalar/1-D
    skipped_unchanged = 0
    new_text = header_text

    # Collect 2-D sub-rows (pst_mg[0]..) keyed by base array, in row order.
    twod = {}             # base -> {row_idx: values}
    for name, member, length in params:
        if name not in dump:
            continue
        new_vals = dump[name]
        if len(new_vals) != length:
            sys.exit(f"{name}: dump has {len(new_vals)} values, expected {length}.")
        mm = re.match(r"^([A-Za-z_]\w*)\[(\d+)\]$", member)
        if mm:
            twod.setdefault(mm.group(1), {})[int(mm.group(2))] = new_vals
            continue
        # scalar / 1-D
        span, cur_vals = current_initializer(new_text, member)
        if span is None:
            sys.exit(f"Could not find initializer for member '{member}'.")
        if cur_vals == new_vals:
            skipped_unchanged += 1
            continue
        repl = format_initializer(member, length, new_vals)
        new_text = new_text[:span[0]] + repl + new_text[span[1]:]
        changed.append((member, cur_vals, new_vals))

    # Handle 2-D bases (rewrite the whole block if any row moved).
    twod_changed = []
    for base, rowmap in twod.items():
        span, rows, cols, cur = find_2d_span(new_text, base)
        if span is None:
            sys.exit(f"Could not find 2-D initializer for '{base}'.")
        new_rows = [rowmap.get(r, cur[r]) for r in range(rows)]
        if new_rows == cur:
            skipped_unchanged += rows
            continue
        if not args.allow_pst:
            sys.exit(f"'{base}' changed but is 2-D; re-run with --allow-pst to rewrite it.")
        comments = PST_ROW_COMMENTS if base.startswith("pst_") else []
        repl = format_2d_block(base, rows, cols, new_rows, comments)
        new_text = new_text[:span[0]] + repl + new_text[span[1]:]
        moved = sum(1 for r in range(rows) if new_rows[r] != cur[r])
        twod_changed.append((base, moved, rows))

    print(f"{len(changed)} scalar/1-D members changed, "
          f"{len(twod_changed)} 2-D arrays changed, {skipped_unchanged} unchanged.")
    for member, old, new in changed:
        print(f"  {member}: {old} -> {new}")
    for base, moved, rows in twod_changed:
        print(f"  {base}: {moved}/{rows} rows changed (rewrote full block)")

    if args.dry_run:
        print("(dry run -- header not written)")
        return
    if changed or twod_changed:
        header_path.write_text(new_text, encoding="utf-8")
        print(f"Wrote {header_path}.")


if __name__ == "__main__":
    main()
