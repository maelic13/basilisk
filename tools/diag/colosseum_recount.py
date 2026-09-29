"""Recount a Colosseum run's official paired sample from its PGN.

The journal identifies sides even in a null pair where both engine names are
the same. Exit 1 when the independent pentanomial disagrees with the runner.
"""

from __future__ import annotations

import argparse
import collections
import json
import math
import pathlib
import re

TAG = re.compile(r'^\[(?P<name>\w+)\s+"(?P<value>.*)"\]\s*$')
PAIR_SCORES = (0.0, 0.25, 0.5, 0.75, 1.0)


def read_games(pgn: pathlib.Path) -> list[dict[str, str]]:
    games: list[dict[str, str]] = []
    current: dict[str, str] = {}
    in_headers = False
    for line in pgn.read_text(encoding="utf-8", errors="replace").splitlines():
        match = TAG.match(line)
        if match:
            if not in_headers:
                if current:
                    games.append(current)
                current = {}
                in_headers = True
            current[match.group("name")] = match.group("value")
        elif not line.strip():
            in_headers = False
    if current:
        games.append(current)
    return games


def read_journal_sides(journal: pathlib.Path) -> dict[str, str]:
    sides: dict[str, str] = {}
    if not journal.exists():
        return sides
    for line in journal.read_text(encoding="utf-8", errors="replace").splitlines():
        try:
            game = json.loads(line)["game"]
        except (ValueError, KeyError, TypeError):
            continue
        if game.get("white") in ("a", "b") and game.get("number") is not None:
            sides[str(game["number"])] = game["white"]
    return sides


def pentanomial(games: list[dict[str, str]], engine_a: str,
                sides: dict[str, str] | None = None) -> tuple[list[int], dict]:
    sides = sides or {}
    if not sides and games and all(game.get("White") == game.get("Black") for game in games):
        raise ValueError("both sides carry the same name and there is no journal to tell them apart")
    tagged = any("ColosseumSample" in game for game in games)
    pairs: dict[str, list[dict[str, str]]] = collections.defaultdict(list)
    for game in games:
        if tagged and game.get("ColosseumSample") != "official":
            continue
        pairs[game.get("PairNumber", "?")].append(game)

    counts = [0, 0, 0, 0, 0]
    wdl = [0, 0, 0]
    incomplete = 0
    for _, members in sorted(pairs.items(), key=lambda item: int(item[0])):
        if len(members) != 2:
            incomplete += 1
            continue
        points = 0.0
        for game in members:
            result = game.get("Result")
            side = sides.get(game.get("GameNumber", ""))
            if sides and side is None:
                raise ValueError(f"game {game.get('GameNumber', '?')} is in the PGN but not in the journal")
            a_is_white = side == "a" if side else game.get("White") == engine_a
            if result == "1/2-1/2":
                points += 0.5
                wdl[1] += 1
            elif result == "1-0":
                points += 1.0 if a_is_white else 0.0
                wdl[0 if a_is_white else 2] += 1
            elif result == "0-1":
                points += 0.0 if a_is_white else 1.0
                wdl[2 if a_is_white else 0] += 1
        counts[int(round(points * 2))] += 1
    return counts, {"wdl": wdl, "incomplete_pairs": incomplete, "pairs_seen": len(pairs),
                    "tagged": tagged, "journal_sides": bool(sides)}


def estimates(counts: list[int]) -> dict:
    total = sum(counts)
    if not total:
        return {"pairs": 0}
    mu = sum(count * score for count, score in zip(counts, PAIR_SCORES)) / total
    variance = sum(count * (score - mu) ** 2 for count, score in zip(counts, PAIR_SCORES)) / total
    sigma = math.sqrt(variance)
    standard_error = sigma / math.sqrt(total)
    result = {"pairs": total, "mu": mu, "sigma": sigma}
    if 0 < mu < 1:
        result["elo"] = -400 * math.log10(1 / mu - 1)
        result["elo_error"] = 1.96 * standard_error * 400 / (math.log(10) * mu * (1 - mu))
    if sigma > 0:
        scale = 800 / (math.log(10) * math.sqrt(2))
        result["nelo"] = (mu - 0.5) / sigma * scale
        result["nelo_error"] = 1.96 * standard_error / sigma * scale
    return result


def recount(directory: pathlib.Path) -> dict:
    record = json.loads((directory / "run-record.json").read_text(encoding="utf-8"))
    games = read_games(directory / "games.pgn")
    fields = {field["label"]: field["value"] for field in record.get("progress", {}).get("fields", [])}
    players = fields.get("players", "")
    engine_a = players.split(" vs. ")[0].strip() if " vs. " in players else ""
    if not engine_a:
        names = [game.get("White", "") for game in games if game.get("PairGame") == "1"]
        engine_a = collections.Counter(names).most_common(1)[0][0] if names else ""

    counts, detail = pentanomial(games, engine_a, read_journal_sides(directory / "games.jsonl"))
    terminations = collections.Counter(game.get("Termination", "?") for game in games)
    official = (sum(1 for game in games if game.get("ColosseumSample") == "official")
                if detail["tagged"] else len(games))
    sample = record.get("official_sample", {})
    recorded = list(sample.get("pentanomial", []))
    recorded_source = "run-record.json"
    if not any(recorded):
        checkpoint_path = directory / "checkpoint.json"
        if checkpoint_path.exists():
            payload = json.loads(checkpoint_path.read_text(encoding="utf-8")).get("payload", {})
            if any(payload.get("pentanomial") or []):
                recorded = list(payload["pentanomial"])
                recorded_source = "checkpoint.json"
    comparable = any(recorded) and sample.get("scored_games") is not None
    return {
        "run": directory.name,
        "command": record.get("command"),
        "status": record.get("status"),
        "engine_a": engine_a,
        "players": players,
        "games_in_pgn": len(games),
        "official_games": official,
        "recorded_scored_games": sample.get("scored_games"),
        "pentanomial": counts,
        "recorded_pentanomial": recorded,
        "recorded_source": recorded_source,
        "oriented_by": "journal" if detail["journal_sides"] else "engine names",
        "comparable": comparable,
        "agrees": (not comparable) or ((counts == recorded) and (official == sample.get("scored_games"))),
        "wdl": detail["wdl"],
        "incomplete_pairs": detail["incomplete_pairs"],
        "terminations": dict(terminations),
        "faults": fields.get("faults"),
        "seconds": round((record.get("updated_unix_ms", 0) - record.get("started_unix_ms", 0)) / 1000),
        **estimates(counts),
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("runs", nargs="+", type=pathlib.Path)
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args()
    results = [recount(run) for run in args.runs]
    if args.json:
        print(json.dumps(results, indent=2))
    else:
        for result in results:
            print(f"{result['run']}: {result['official_games']} games, pentanomial {result['pentanomial']}, "
                  f"recorded {result['recorded_pentanomial']}, agrees={result['agrees']}")
    return 0 if all(result["agrees"] for result in results) else 1


if __name__ == "__main__":
    raise SystemExit(main())
