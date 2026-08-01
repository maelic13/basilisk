"""Summarise a weather-factory tuner/state.json for the shell.

BASILISK_SPSA_SCHEDULE_V1 — part of the Basilisk overlay (see spsa.py).

`tools/spsa.ps1` needs two things out of a saved run before it launches: the
horizon the schedule was frozen with, and how far the run got. It cannot read
the file itself — PowerShell's ConvertFrom-Json rejects the schema outright
("keys with different casing") because SPSA has both `a` and `A`. So this
prints flat `key=value` lines instead, one per line, and the shell parses that.

    python describe_state.py tuner/state.json

Missing/!unreadable file -> exit 1 with nothing on stdout, so the caller can
simply treat "no output" as "no usable state".
"""

import json
import sys


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: describe_state.py <state.json>", file=sys.stderr)
        return 2
    try:
        with open(sys.argv[1]) as handle:
            state = json.load(handle)
    except (OSError, ValueError) as exc:
        print(f"could not read state: {exc}", file=sys.stderr)
        return 1

    params = state.get("spsa_params", {})
    games = state.get("t", 0)
    # Keys are deliberately NOT `a` / `A`: the caller is PowerShell, whose
    # hashtables are case-insensitive, so those two would silently collide on
    # the way back in — the same defect that makes ConvertFrom-Json reject this
    # file. Emit unambiguous names and let the caller relabel for display.
    out = {
        "gain_a": params.get("a", ""),
        "probe_c": params.get("c", ""),
        "damp_A": params.get("A", ""),
        "alpha": params.get("alpha", ""),
        "gamma": params.get("gamma", ""),
        # N and r_end are absent in pre-9.1 states; report 0 so the caller can
        # say "this run predates the horizon and cannot stop itself".
        "N": params.get("N", 0),
        "r_end": params.get("r_end", 0),
        "games": games,
        "params": len(state.get("uci_params", [])),
    }
    for key, value in out.items():
        print(f"{key}={value}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
