# Documents, evidence and provenance

Scope: Roadmap and ledger documents, evidence, citations, tags, capability
classes and the pull-request message.

Read in full when: Task -- Any `M` leaf; closing any leaf. Paths --
`docs/PLAN.md`, `GUIDE.md`, `CHANGELOG.md`, `docs/HISTORY.md`,
`docs/EXPERIMENTS.md`, `docs/experiments/`, `docs/archive/`, `analysis/`,
`tools/diag/citation_exceptions.tsv`. Actions -- Before citing a commit in a
document; before adding a result, log or fixture to git; before creating,
pushing or deleting a tag, or keeping a branch; before opening a pull request
or writing its message; before changing a capability tag; when a change is
user-visible (strength, UCI options or output, defaults, the bench
fingerprint, platforms).

## Documents

- A roadmap status or requirement change is made in `docs/PLAN.md` and committed
  with the regenerated `GUIDE.md`; an edit only to `AGENTS.md` or `agents/`
  needs no PLAN or GUIDE churn.
- After any PLAN change run `python tools/diag/check_roadmap.py --write-guide`;
  run `python tools/diag/check_roadmap.py` whenever PLAN or GUIDE changes. It
  fails when GUIDE's generated block is stale.
- PLAN carries status. Tick a step only when finished and verified, in the
  commit that finishes it; tick the parent when its last sub-step is ticked.
- `CHANGELOG.md`'s `[Unreleased]` is kept current. A user-visible change
  gets its entry in the commit that records it, in plain language with its
  deciding result. User-visible means playing strength, UCI options or
  output, defaults, the bench fingerprint, or supported platforms. Tooling
  and documentation changes get no entry. A release's section opens with a
  short summary and a `### Highlights` list of the basics, which are all
  the release page shows; the page links the full section.
- A leaf's first line is its identifier, capability tag and a short title, at
  most 64 characters: the name GUIDE shows. Everything else goes on the
  indented lines below it. Never edit GUIDE's generated block by hand.
- Sub-steps indent by 4 spaces; nothing goes deeper than three levels
  (`C.7.1`). Let `check_roadmap.py` check the structure rather than reading
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
- A document cites only commits that stay reachable: on `master`, under a
  pull request (`refs/pull/<n>/head`, which GitHub keeps after the squash) or
  on a kept tag. A throwaway branch -- a test arm -- is cited by its recipe or
  patch file, never by its hash. Cite a commit as a backticked hash; another
  repository's commit or a content hash cited the same way is declared in
  `tools/diag/citation_exceptions.tsv`. `python tools/diag/check_citations.py`
  checks every citation, and it needs the PR refs. Every clone maps them
  once, so that every fetch keeps them, a pruning one included:
  `git config --add remote.origin.fetch "+refs/pull/*/head:refs/remotes/origin/pr/*"`.
  A one-off `git fetch` of that refspec is lost at the next pruning fetch,
  which GitHub Desktop runs on its own.
- No preservation tags and no kept branches without the maintainer's
  agreement. The tags kept so far are listed with their reason and retirement
  condition in HISTORY *Preserved commits*; a new one, if agreed, is an
  annotated tag named `<purpose>/<name>`. Pushing a tag and deleting a remote
  tag are the maintainer's commands. A tag that holds another engine's code
  is kept only locally, and its cited commits are declared `local`.

## Capability classes

Open PLAN leaves carry a capability tag, which GUIDE shows. PLAN §2,
*Workflow states and capability classes*, defines the six classes.

Classes are routing hints, not permission, state or evidence. The editable
mapping from classes to currently available models belongs only in GUIDE.md.
Do not silently downgrade a class; if scope or uncertainty calls for
escalation, say why, change the PLAN tag and regenerate GUIDE. Completed historical
model tags may remain unchanged.

## Pull requests

- Before opening the PR, merge `master` into the branch if `master` moved, so
  the PR shows only the branch's own change. After the merge the branch is
  deleted (PROCESS *Delivery*).
- The squash message -- the PR title and description, which is all that
  reaches `master` -- has a fixed shape: the title is short and imperative
  and says what changed ("Rebuild king safety"), with no results and no
  phase, plan or ledger references; the body is a few plain lines on what and
  why, wrapped at 72 columns; then the test results, one per line ("STC
  3+0.03: +32.7 ± 8.9 Elo (2,450 games)"); then, after an empty line,
  `Bench: <nodes>`. Empty lines separate the title, the body, the results
  and the bench line.
