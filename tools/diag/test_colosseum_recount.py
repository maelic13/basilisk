import json
import pathlib
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import colosseum_recount

NAME = "basilisk-null"
GAMES = [(1, 1, "a", "1-0"), (1, 2, "b", "0-1"),
         (2, 1, "a", "1/2-1/2"), (2, 2, "b", "1-0")]
EXPECTED = [0, 1, 0, 0, 1]


def write_run(root, record_pentanomial=EXPECTED, journal=True, names=(NAME, NAME), drop=()):
    root = pathlib.Path(root)
    pgn, lines = [], []
    for number, (pair, pair_game, white_side, result) in enumerate(GAMES, start=1):
        white, black = (names[0], names[1]) if white_side == "a" else (names[1], names[0])
        pgn.append("\n".join([f'[White "{white}"]', f'[Black "{black}"]', f'[Result "{result}"]',
                              '[Termination "normal"]', f'[GameNumber "{number}"]',
                              f'[PairNumber "{pair}"]', f'[PairGame "{pair_game}"]', "", f"1. e4 e5 {result}", ""]))
        if number not in drop:
            lines.append(json.dumps({"game": {"number": number, "white": white_side}}))
    (root / "games.pgn").write_text("\n".join(pgn), encoding="utf-8")
    if journal:
        (root / "games.jsonl").write_text("\n".join(lines) + "\n", encoding="utf-8")
    (root / "run-record.json").write_text(json.dumps({
        "command": "match", "status": "completed",
        "official_sample": {"scored_games": len(GAMES), "pentanomial": record_pentanomial},
        "progress": {"fields": [{"label": "players", "value": f"{names[0]} vs. {names[1]}"}]},
    }), encoding="utf-8")
    return root


class RecountTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = pathlib.Path(self.temp.name)

    def tearDown(self):
        self.temp.cleanup()

    def test_null_pair_uses_journal_sides(self):
        result = colosseum_recount.recount(write_run(self.root))
        self.assertEqual(result["pentanomial"], EXPECTED)
        self.assertTrue(result["agrees"])

    def test_same_names_without_journal_are_refused(self):
        with self.assertRaisesRegex(ValueError, "same name"):
            colosseum_recount.recount(write_run(self.root, journal=False))

    def test_missing_journal_game_is_refused(self):
        with self.assertRaisesRegex(ValueError, "not in the journal"):
            colosseum_recount.recount(write_run(self.root, drop=(3,)))

    def test_distinct_names_can_fall_back_to_names(self):
        result = colosseum_recount.recount(write_run(self.root, journal=False, names=("new", "base")))
        self.assertEqual(result["pentanomial"], EXPECTED)

    def test_disagreement_is_reported(self):
        result = colosseum_recount.recount(write_run(self.root, record_pentanomial=[0, 0, 2, 0, 0]))
        self.assertFalse(result["agrees"])


if __name__ == "__main__":
    unittest.main()
