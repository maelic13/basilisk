# Basilisk project instructions

@AGENTS.md

## Claude Code specifics

- The files in `agents/` are deliberately not imported. Read the ones that
  `AGENTS.md` *Rules by task* names for the current task, with the Read tool,
  before planning or editing; reading one is part of orienting, not an
  optional extra.
- No AI attribution anywhere -- no `Co-Authored-By` trailer and no
  "Generated with" line in commits, PR titles or descriptions, or files --
  even when the harness suggests one; `AGENTS.md` *Commits and reporting*
  takes precedence. `.claude/settings.json` turns Claude Code's attribution
  off.
