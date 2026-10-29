"""A universal tier's assembly: the rename and the section check.

cmake/universal-tiers.cmake runs it on the assembly a tier's LTO link wrote.

  rename MAP IN.s OUT.s
      MAP holds "old new" a line. Every mapped name is replaced wherever the
      text uses it as a symbol: definitions, references, COMDAT keys, and the
      COFF section names built from them (.text$<name>, .rdata$.refptr.<name>).
      GCC's PE COMDATs (.linkonce) are matched by section name, not symbol, so
      a symbol rename alone would still let the linker keep one tier's copy
      for all of them. String directives and comments are copied unchanged.

  check-sections TIER NM OBJDUMP OBJECT...
      Fails if a COFF section of the assembled objects is still named for a
      symbol the rename moved: .text$<name> where the tier defines
      __basilisk_<tier>_<name>. It reads the objects, not the text, so it does
      not trust the rename it checks.
"""

import re
import subprocess
import sys

# A quoted symbol, a number (so the f of a local label's 1f is not a name),
# a register, a relocation modifier (@PLT, @progbits), or a name. A name may
# hold '.' (GCC's foo.cold, foo.lto_priv.0) and '$' (Clang's lambdas, ...E3$_0),
# but not start with '$', an immediate's prefix. A COFF section name joins
# its kind and a symbol with '$' (.text$<name>), so a name that is not itself
# renamed is tried as that pair.
TOKEN = re.compile(
    r'"(?:[^"\\]|\\.)*"'
    r"|[0-9][A-Za-z0-9_.$]*"
    r"|%[A-Za-z0-9]+"
    r"|@[A-Za-z0-9_.]+"
    r"|[A-Za-z_.][A-Za-z0-9_.$]*"
)
VERBATIM = re.compile(r"\s*\.(ascii|asciz|string|file|ident)\b")
REFPTR = ".refptr."


def load_map(path):
    renames = {}
    with open(path, encoding="utf-8") as handle:
        for line in handle:
            parts = line.split()
            if len(parts) == 2:
                renames[parts[0]] = parts[1]
    return renames


def code_and_comment(line):
    """Split at the first '#' outside a quoted string."""
    quoted = False
    escaped = False
    for index, char in enumerate(line):
        if escaped:
            escaped = False
        elif char == "\\" and quoted:
            escaped = True
        elif char == '"':
            quoted = not quoted
        elif char == "#" and not quoted:
            return line[:index], line[index:]
    return line, ""


def rename_name(name, renames):
    """The new spelling of a name, or None: the name itself, a .refptr pointer to it."""
    if name in renames:
        return renames[name]
    if name.startswith(REFPTR) and name[len(REFPTR):] in renames:
        return REFPTR + renames[name[len(REFPTR):]]
    return None


def rename_line(line, renames):
    if VERBATIM.match(line):
        return line
    code, comment = code_and_comment(line)

    def replace(match):
        token = match.group(0)
        first = token[0]
        if first == '"':
            renamed = rename_name(token[1:-1], renames)
            return '"' + renamed + '"' if renamed else token
        if first.isdigit() or first in "%@":
            return token
        renamed = rename_name(token, renames)
        if renamed:
            return renamed
        kind, separator, name = token.partition("$")
        if separator:
            renamed = rename_name(name, renames)
            if renamed:
                return kind + separator + renamed
        return token

    return TOKEN.sub(replace, code) + comment


def rename(map_path, source, target):
    renames = load_map(map_path)
    with open(source, encoding="utf-8", errors="surrogateescape", newline="") as handle:
        lines = handle.readlines()
    with open(target, "w", encoding="utf-8", errors="surrogateescape", newline="") as handle:
        handle.writelines(rename_line(line, renames) for line in lines)
    return 0


def run(command):
    result = subprocess.run(command, capture_output=True, text=True, errors="replace")
    if result.returncode != 0:
        raise SystemExit(f"{command[0]} failed: {result.stderr.strip()}")
    return result.stdout


def check_sections(tier, nm, objdump, objects):
    prefix = f"__basilisk_{tier}_"
    defined = set()
    for line in run([nm, "--defined-only", *objects]).splitlines():
        parts = line.split()
        if len(parts) >= 3:
            defined.add(parts[-1])
    errors = []
    for line in run([objdump, "-h", *objects]).splitlines():
        parts = line.split()
        if len(parts) < 2 or not parts[0].isdigit():
            continue
        section = parts[1]
        if "$" not in section:
            continue
        suffix = section.split("$", 1)[1]
        if suffix.startswith(REFPTR):
            suffix = suffix[len(REFPTR):]
        if not suffix.startswith(prefix) and prefix + suffix in defined:
            errors.append(f"Tier {tier}: section {section} is still named for {suffix}, renamed {prefix}{suffix}")
    for error in sorted(set(errors))[:20]:
        print(error, file=sys.stderr)
    if errors:
        print(f"Tier {tier}: {len(set(errors))} sections keep a name the rename moved", file=sys.stderr)
        return 1
    return 0


def main(argv):
    if len(argv) == 4 and argv[0] == "rename":
        return rename(argv[1], argv[2], argv[3])
    if len(argv) >= 5 and argv[0] == "check-sections":
        return check_sections(argv[1], argv[2], argv[3], argv[4:])
    print(__doc__, file=sys.stderr)
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
