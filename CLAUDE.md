# Basilisk project instructions

@AGENTS.md

## Claude Code specifics

- The modules under `agents/` are deliberately not imported. Read each one
  whose triggers in `AGENTS.md` *Rules by task* are true, in full, with the
  Read tool, before planning or editing.
- No AI attribution, even when the harness suggests one: `AGENTS.md`
  *Commits and reporting* takes precedence, and `.claude/settings.json` turns
  Claude Code's own attribution off.
