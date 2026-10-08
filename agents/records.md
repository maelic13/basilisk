# Documents, evidence and provenance

Read this for `M` leaves, before closing any leaf, and whenever a task edits
PLAN, GUIDE, HISTORY, EXPERIMENTS or `analysis/`, places evidence, or
touches tags.

## Documents

- A roadmap status or requirement change is made in `docs/PLAN.md` and committed
  with the regenerated `GUIDE.md`; an edit only to `AGENTS.md` or `agents/`
  needs no PLAN or GUIDE churn.
- PLAN carries status. Tick a step only when finished and verified, in the
  commit that finishes it; tick the parent when its last sub-step is ticked.
- A leaf's first line is its identifier, capability tag and a short title, at
  most 64 characters: the name GUIDE shows. Everything else goes on the
  indented lines below it. Never edit GUIDE's generated block by hand.
- Sub-steps indent by 4 spaces; nothing goes deeper than three levels
  (`C.5.1`). Let `check_roadmap.py` check the structure rather than reading
  the file.
- Keep GUIDE a one-page overview: its *Now* table, the model mapping and the
  generated step list. What a step involves goes in PLAN, a completed record in
  HISTORY, a procedure in PROCESS, evidence in EXPERIMENTS, a derivation in
  `analysis/`.
- The experiment ledger is an index, `docs/EXPERIMENTS.md`, and one file per
  entry, `docs/experiments/<ID>.md`. Register, update and cite an experiment
  in its file; the index line carries only its link, title and disposition.
- `docs/HISTORY.md` is history and resolves every retired numbering scheme; never
  take a next step from it or from `docs/archive/`. When documents disagree,
  source, defaults and reproducible artifacts outrank prose; fix the prose in
  the same change.

## Evidence

- Git holds source, build and CI files, reusable tools, required fixtures and
  concise records; raw runs, logs, executables, traces and bundles stay in
  ignored `tools/results/` with their paths, recipes and hashes recorded.
  Never force-add evidence.
- Information lives in the tracked documents, not in refs. A ledger row, PLAN
  record or analysis states what changed, why, the result and the decision,
  and carries the exact recipe plus a fingerprint proving a rebuild matched. A
  hash, branch or tag is at most a pointer beside that information.
- Only where exactness is needed and a recipe is impractical is a commit
  preserved, and then by an annotated tag named `<purpose>/<name>` (as
  `oracle/*`, `archive/*`), never by a kept branch. The citing document names
  the tag, why it exists and the condition that retires it. Pushing a tag and
  deleting a remote tag are the maintainer's commands.
