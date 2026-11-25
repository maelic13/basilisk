# Agent rules — load limits, budget, inventory and trigger test

- State / class: closed with the change it describes; not a PLAN leaf (a
  maintainer request, 2026-11-25).
- Owner / date: 2026-11-25.
- Decision needed: the eight items in §7.

Agent tools load a rule file only up to a size limit and drop the rest
without a warning, and a rulebook grows by one rule per incident. This packet
records what each tool loads here and how much of it, the budget that follows,
what moved between the core (`AGENTS.md`) and the modules (`agents/`), the
proof that no rule was lost, and a test of the module triggers on work the
project has already done. The base is `ff65589`, `master` when the change
branched.

## 1. What is loaded, and how large it was

Sizes are UTF-8 bytes. A file is counted twice: with LF line endings, as git
stores it, and with CRLF, as this host checks it out. The budget is held
against the CRLF size, the larger of the two, so the answer is the same on
every machine.

**Evidence.** Loaded by a tool with no action from the agent:

| File | Loaded by | LF bytes | CRLF bytes | Lines |
|---|---|---|---|---|
| `CLAUDE.md` | Claude Code | 612 | 627 | 15 |
| `AGENTS.md` | Claude Code through `CLAUDE.md`'s import; natively by every tool that reads `AGENTS.md` (Codex, Antigravity, Gemini CLI when configured, Cursor, Copilot) | 12,829 | 13,051 | 222 |
| `.claude/settings.json` | Claude Code, as settings, not as rules | 70 | — | 3 |
| Claude Code's memory index (`MEMORY.md`, outside the repository, on the maintainer's machine) | Claude Code | 6,190 | — | 44 |

The repository holds no `GEMINI.md`, `.agent/`, `.cursor/`,
`.github/copilot-instructions.md` or other stub. Two more files carry a rule
file's name: `docs/reference/rarog/AGENTS.md` (21,572 CRLF bytes) and
`docs/reference/rarog-2026-10-06/AGENTS.md` (23,805). They are Rarog's
rules inside verbatim snapshots (§7, item 4).

Read on a trigger, never loaded by a tool:

| File | LF bytes | CRLF bytes | Lines |
|---|---|---|---|
| `agents/implementation.md` | 2,454 | 2,501 | 47 |
| `agents/measurement.md` | 7,790 | 7,921 | 131 |
| `agents/records.md` | 3,980 | 4,045 | 65 |
| `agents/research.md` | 7,760 | 7,884 | 124 |

Documents the rules send an agent to at orientation:

| Document | What the rules say to read | CRLF bytes | Lines |
|---|---|---|---|
| `GUIDE.md` | its *Now* table and the next step; the file is one page | 7,676 | 159 |
| `docs/PLAN.md` | "the selected PLAN section" | 233,828 whole | 3,644 |
| — one leaf's entry | | median 1,469, largest 10,207 (82 entries) | |
| — §2, the operating rules the modules cite by number | | 19,715 | 292 |
| — Phase C, the current phase | | 132,611 | 2,157 |
| `docs/DESIGN.md` | first, before a mechanism (`agents/research.md`) | 12,100 | 206 |
| `docs/PROCESS.md` | the procedure a rule names, by section | 54,884 whole | 930 |

## 2. Load limits

**Evidence**, from each tool's own documentation, read on 2026-11-25:

| Tool | Limit | Past it | Source |
|---|---|---|---|
| Antigravity | 24,000 bytes for one rule file, counted after its `@[label](path)` includes are expanded; a 20,000-token budget for all always-on rules together. It reads `AGENTS.md` and `GEMINI.md` as always-on rules. | The file is truncated; past the token budget the largest files are replaced by pointers. | <https://antigravity.google/docs/rules> |
| Codex | 32 KiB (`project_doc_max_bytes`) for all instruction files together | It stops adding files. | <https://developers.openai.com/codex> (the AGENTS.md guide) |
| Claude Code | No truncation of `CLAUDE.md` below 4 MiB; a target of under 200 lines for each file. An import loads with the file that imports it, to four hops. `AGENTS.md` is read by itself only when no `CLAUDE.md` exists, and an import never loads it twice. | A file over 4 MiB is skipped. | <https://code.claude.com/docs/en/memory> |
| Claude Code, memory index | The first 200 lines or 25 KB of `MEMORY.md` | The rest is not loaded. | the same page |
| Gemini CLI | None documented for `GEMINI.md`; imports to five levels | — | the `gemini-md` and `memport` pages of <https://github.com/google-gemini/gemini-cli> |
| Windsurf | 12,000 characters for each of its own rules files. Its page does not say the limit applies to `AGENTS.md`, nor what happens past it. | not stated | <https://docs.devin.ai/windsurf/plugins/cascade/memories.md> |
| Cursor | None; a recommendation of under 500 lines for a rule | — | <https://cursor.com/docs/context/rules> |
| Copilot | None found on the English pages for the coding agent or chat | — | <https://docs.github.com/en/copilot> |
| Junie | None found | — | <https://junie.jetbrains.com/docs/> |

The Antigravity, Claude Code and Windsurf rows were read twice, the second
time against the page text. The other rows were read once. A report that
Copilot's code review reads only the first 4,000 characters of an
instruction file was found on a translated page only and is not confirmed.

**Inference.** The 24,000-byte cut this task was told of is Antigravity's,
and it is documented. It is the smallest documented limit that applies to
`AGENTS.md`. Windsurf's 12,000 characters is smaller, applies to another
kind of file, and belongs to a tool this repository has no stub for (§7,
item 6).

## 3. Budget

**Decision.** **18,000 bytes**: 75% of 24,000. It bounds every set of files
a tool loads by itself (`AGENTS.md` alone, and `CLAUDE.md` with the
`AGENTS.md` it imports) and each module, so that a module can always be read
in one go. The core declares it, in *Changing these rules*;
`tools/diag/check_rules.py` reads it from there and enforces it.

| Set | Before | After | Of the budget | Of the 24,000-byte cut |
|---|---|---|---|---|
| `AGENTS.md` alone | 13,051 | 13,588 | 75% | 57% |
| Claude Code: `CLAUDE.md` + `AGENTS.md` | 13,678 | 14,215 | 78% | 59% |
| `agents/implementation.md` | 2,501 | 3,247 | 18% | |
| `agents/measurement.md` | 7,921 | 7,917 | 43% | |
| `agents/records.md` | 4,045 | 6,395 | 35% | |
| `agents/research.md` | 7,884 | 8,675 | 48% | |

The core is 537 bytes larger than it was. About 2,700 bytes of task-scoped
rules left it; the module index with observable triggers, the closing check,
the conflict rule and the growth rules came in. Against Codex's 32 KiB the
loaded set is 43%; against Antigravity's token budget it is roughly a fifth,
at four bytes to a token.

## 4. The split

The rules were already split: `AGENTS.md` as the core and four modules under
`agents/`, named by tests, tools, ledger entries and PLAN, so the module
paths and section names stay. What changed:

- **The index.** *Rules by task* was a table of task kinds. Each module now
  has one entry giving its scope and three kinds of trigger: the task's
  kind, the paths changed, the actions taken. Each module opens with the
  same text, and the check fails when the two differ by a word.
- **A closing check**, before any report: hold the triggers against what was
  changed, run and committed; read what was missed; name the modules read.
  `check_rules.py --triggered` answers the path part from the branch's diff.
- **Out of the core, unchanged**, each to the module whose trigger reaches
  it: the capability classes, the squash message's shape, the two sentences
  on merging `master` before a pull request and deleting the branch after,
  and the `check_roadmap.py` commands (`agents/records.md`); what research
  owns, and the independent review (`agents/research.md`); what
  implementation owns and does not (`agents/implementation.md`).
- **Kept in the core though task-scoped:** the fingerprint a
  behaviour-neutral change reproduces. Implementation, review and
  measurement all need the number, and `check_roadmap.py` checks it there.
- **Incidents out of the rules.** Six sentences that tell what once went
  wrong left the modules; each rule keeps its citation, and the sentences
  are in §5, word for word.
- **`CLAUDE.md` is unchanged.** It imports the core and no module.

New rules, which the base did not have. Each is short, and each is the
maintainer's to strike:

| | Rule | Where |
|---|---|---|
| N1 | A module is as binding as the core, and is skipped only because no trigger is true, never to save time. | core, *Rules by task* |
| N2 | The closing check, and the modules read are named in the report. | core, *Rules by task* |
| N3 | If a rule conflicts with the task as given, or with another rule, stop and ask. | core, *Rules by task* |
| N4 | The budget, and who may raise it. | core, *Changing these rules* |
| N5 | A new rule goes into the module whose triggers reach it; only a rule whose violation does damage on any task enters the core; the incident behind a rule goes to the ledger or a packet. | core, *Changing these rules* |
| N6 | A stub may import the core and never a module. | core, *Changing these rules* |

Triggers the old table did not have, all of which make a module read more
often, none less: for research, the paths `docs/DESIGN.md`,
`docs/experiments/` and `analysis/`, a change that moves the bench
fingerprint, reading another engine's source or `docs/reference/`, and
closing a leaf that ran an experiment; for implementation, `tests/`, the
build files, the CI workflows and a review of code; for measurement, the
ledger paths, `tools/colosseum/`, the launchers (`tools/run_*`), the
compiler pin, a command or launcher handed to the maintainer, and any run
that loads the machine for minutes; for records, `CHANGELOG.md`,
`docs/archive/`, the citation exceptions, a cited commit, a pull request, a
capability tag and a user-visible change.

## 5. Inventory

A block is a paragraph, a list item or a table row; headings are not blocks.
`python tools/diag/rules_inventory.py` recomputes the blocks from the base
and fails unless every block's text, or the rewording recorded for it, is
found word for word in the file its row names. Kinds: an *invariant* applies
to every task; a *task-scoped* rule to certain work; a *duplicate* repeats
the row it names; *rationale* tells why and is not a rule. IDs are the
file's letter and the block's number at the base: A `AGENTS.md`, C
`CLAUDE.md`, I, M, D and R the implementation, measurement, records
(documents) and research modules.

No two blocks were exact duplicates, so none were merged. The near
duplicates are noted on their rows and listed in §7, item 2.

<!-- BEGIN GENERATED by `python tools/diag/rules_inventory.py --write` -->

### Blocks at the base

| File | Blocks |
|---|---|
| `AGENTS.md` | 55 |
| `CLAUDE.md` | 3 |
| `agents/implementation.md` | 7 |
| `agents/measurement.md` | 41 |
| `agents/records.md` | 13 |
| `agents/research.md` | 26 |
| **All** | **145** |

### Totals

| Count | Blocks |
|---|---|
| Core blocks kept in the core | 40 |
| Core blocks moved whole to a module | 14 |
| Core blocks split between the core and a module | 1 |
| Stub blocks, unchanged | 3 |
| Module blocks kept in their module | 85 |
| Reworded (old and new text below) | 15 |
| Exact duplicates merged | 0 |
| Incident text moved out of the rules | 6 |
| Kind: invariant | 38 |
| Kind: task-scoped | 104 |
| Kind: duplicate | 1 |
| Kind: rationale or history | 2 |

### Normative words, before and after

| Word | At the base | Now |
|---|---|---|
| must | 8 | 7 |
| never | 28 | 29 |
| always | 3 | 3 |
| only | 27 | 29 |
| do not | 20 | 21 |
| may not | 2 | 2 |

### Every block

| ID | Was at | Kind | Is now in | Note | Opens |
|---|---|---|---|---|---|
| A01 | `AGENTS.md:3` | invariant | `AGENTS.md` | unchanged | These instructions apply to every agent work… |
| A02 | `AGENTS.md:14` | invariant | `AGENTS.md` | reworded | This file is always in force. Before startin… |
| A03 | `AGENTS.md:20` | task-scoped | `AGENTS.md` | reworded: triggers made observable | \| Research, diagnosis, experiment design, r… |
| A04 | `AGENTS.md:21` | task-scoped | `AGENTS.md` | reworded: triggers made observable | \| Engine or tool code; I1/I2 leaves \| agen… |
| A05 | `AGENTS.md:22` | task-scoped | `AGENTS.md` | reworded: triggers made observable | \| Builds to measure, bench/NPS/counters, ga… |
| A06 | `AGENTS.md:23` | task-scoped | `AGENTS.md` | reworded: triggers made observable | \| PLAN, GUIDE, HISTORY, EXPERIMENTS or anal… |
| A07 | `AGENTS.md:27` | invariant | `AGENTS.md` | unchanged | - Treat docs/PLAN.md as the roadmap: every s… |
| A08 | `AGENTS.md:30` | invariant | `AGENTS.md` | unchanged | - First classify the requested work as resea… |
| A09 | `AGENTS.md:35` | invariant | `AGENTS.md` | unchanged | - "Do the next step" means handle only the e… |
| A10 | `AGENTS.md:40` | invariant | `AGENTS.md` | unchanged | - Do not start later steps, combine adjacent… |
| A11 | `AGENTS.md:42` | invariant | `AGENTS.md` | unchanged | - Finish the requested leaf, verify it propo… |
| A12 | `AGENTS.md:45` | task-scoped | `agents/records.md` | moved; overlaps D02 | - After any PLAN change run python tools/dia… |
| A13 | `AGENTS.md:51` | invariant | `AGENTS.md` | unchanged | The normal playing-change path is: |
| A14 | `AGENTS.md:53` | invariant | `AGENTS.md` | unchanged | RESEARCH -> READY_FOR_IMPLEMENTATION -> IMPL… |
| A15 | `AGENTS.md:55` | invariant | `AGENTS.md` | unchanged | Not every task uses every state. Documentati… |
| A16 | `AGENTS.md:61` | invariant | `AGENTS.md` | unchanged | READY_FOR_IMPLEMENTATION is the hard boundar… |
| A17 | `AGENTS.md:66` | task-scoped | `agents/research.md` | moved; its last sentence is R13's but for one word | - **Research owns** the causal question, com… |
| A18 | `AGENTS.md:70` | task-scoped | `agents/implementation.md` | moved | - **Implementation owns** ordinary engineeri… |
| A19 | `AGENTS.md:74` | task-scoped | `agents/implementation.md` | moved; overlaps I04 | - **Implementation does not** replace the hy… |
| A20 | `AGENTS.md:78` | invariant | `AGENTS.md` | unchanged | - **A false premise returns the leaf.** Keep… |
| A21 | `AGENTS.md:84` | task-scoped | `agents/research.md` | moved | - **Review is independent.** A cluster imple… |
| A22 | `AGENTS.md:91` | task-scoped | `agents/records.md` | moved | Open PLAN leaves carry a capability tag, whi… |
| A23 | `AGENTS.md:95` | task-scoped | `agents/records.md` | moved; PLAN §2 and GUIDE word the classes differently | \| R3 \| frontier research; unresolved causa… |
| A24 | `AGENTS.md:96` | task-scoped | `agents/records.md` | moved | \| R2 \| bounded but correctness-sensitive a… |
| A25 | `AGENTS.md:97` | task-scoped | `agents/records.md` | moved | \| I2 \| difficult implementation requiring… |
| A26 | `AGENTS.md:98` | task-scoped | `agents/records.md` | moved | \| I1 \| well-specified implementation \| |
| A27 | `AGENTS.md:99` | task-scoped | `agents/records.md` | moved | \| M \| mechanical documentation, manifests… |
| A28 | `AGENTS.md:100` | task-scoped | `agents/records.md` | moved | \| V \| verification or measurement work \| |
| A29 | `AGENTS.md:102` | task-scoped | `agents/records.md` | moved | Classes are routing hints, not permission, s… |
| A30 | `AGENTS.md:110` | invariant | `AGENTS.md` | unchanged | - Do the work specified by the current leaf.… |
| A31 | `AGENTS.md:112` | invariant | `AGENTS.md` | unchanged | - If a newly found bug blocks the leaf, stop… |
| A32 | `AGENTS.md:114` | invariant | `AGENTS.md` | unchanged | - If a bug, chess error or promising improve… |
| A33 | `AGENTS.md:118` | invariant | `AGENTS.md` | unchanged | - Analyze code in chess terms as well as sof… |
| A34 | `AGENTS.md:122` | invariant | `AGENTS.md` | unchanged | - Call out suspicious or incorrect chess beh… |
| A35 | `AGENTS.md:127` | invariant | `AGENTS.md` | unchanged | Almost every agent mistake in these projects… |
| A36 | `AGENTS.md:134` | invariant | `AGENTS.md` | unchanged | - Match verification cost to risk. Do not ru… |
| A37 | `AGENTS.md:139` | task-scoped | `AGENTS.md` | kept in the core: implementation, review and measurement all need the number, and `check_roadmap.py` checks it here | - A behavior-neutral engine change reproduce… |
| A38 | `AGENTS.md:145` | invariant | `AGENTS.md` | unchanged | - Check exit status directly, never through… |
| A39 | `AGENTS.md:147` | invariant | `AGENTS.md` | unchanged | - Test constructs and behaviour, not words i… |
| A40 | `AGENTS.md:151` | invariant | `AGENTS.md` | unchanged | - If the maintainer interrupts work with a c… |
| A41 | `AGENTS.md:154` | invariant | `AGENTS.md` | unchanged | - Do not spawn broad parallel agents merely… |
| A42 | `AGENTS.md:158` | invariant | `AGENTS.md` | unchanged | - Before nontrivial edits, briefly state the… |
| A43 | `AGENTS.md:161` | invariant | `AGENTS.md` | unchanged | - A nontrivial final report normally names f… |
| A44 | `AGENTS.md:167` | invariant | `AGENTS.md` | unchanged; "the selected PLAN section" is §7, item 1 | - Orient once per session: these rules, the… |
| A45 | `AGENTS.md:171` | invariant | `AGENTS.md` | unchanged | - Batch independent reads and checks; send v… |
| A46 | `AGENTS.md:174` | invariant | `AGENTS.md` | unchanged | - Keep a compact working record: leaf, sourc… |
| A47 | `AGENTS.md:177` | invariant | `AGENTS.md` | unchanged; its last sentence overlaps M20 | - Before repeating any read, check or run, n… |
| A48 | `AGENTS.md:183` | invariant | `AGENTS.md`, `agents/records.md` | split: its two pull-request sentences moved, unchanged | - master is the trunk. Every coherent piece… |
| A49 | `AGENTS.md:192` | task-scoped | `agents/records.md` | moved | - The squash message -- the PR title and des… |
| A50 | `AGENTS.md:200` | invariant | `AGENTS.md` | unchanged | - Commits on a branch are working records, k… |
| A51 | `AGENTS.md:203` | invariant | `AGENTS.md` | unchanged | - No AI attribution anywhere: no Co-Authored… |
| A52 | `AGENTS.md:206` | invariant | `AGENTS.md` | unchanged | - Preserve unrelated maintainer changes and… |
| A53 | `AGENTS.md:208` | invariant | `AGENTS.md` | unchanged | - **Every report opens with a one-line recom… |
| A54 | `AGENTS.md:215` | invariant | `AGENTS.md` | unchanged | - When maintainer action is needed, give run… |
| A55 | `AGENTS.md:217` | invariant | `AGENTS.md` | unchanged | - End-step reports are short: outcome, essen… |
| C01 | `CLAUDE.md:3` | invariant | `CLAUDE.md` | unchanged | @AGENTS.md |
| C02 | `CLAUDE.md:7` | invariant | `CLAUDE.md` | unchanged | - The files in agents/ are deliberately not… |
| C03 | `CLAUDE.md:11` | duplicate | `CLAUDE.md` | of A51, kept: it adds what is particular to Claude Code | - No AI attribution anywhere -- no Co-Author… |
| I01 | `agents/implementation.md:3` | task-scoped | `agents/implementation.md` | reworded: the header, and the pointer to the ownership bullets now below it | Read this for I1 and I2 leaves and any chang… |
| I02 | `agents/implementation.md:11` | task-scoped | `agents/implementation.md` | unchanged | Per-node code (negamax, qsearch, move pickin… |
| I03 | `agents/implementation.md:25` | task-scoped | `agents/implementation.md` | unchanged | - Exact bench identity is a necessary determ… |
| I04 | `agents/implementation.md:32` | task-scoped | `agents/implementation.md` | unchanged | - Never relax a correctness test in the comm… |
| I05 | `agents/implementation.md:34` | task-scoped | `agents/implementation.md` | unchanged | - Comments explain the problem or the invari… |
| I06 | `agents/implementation.md:39` | task-scoped | `agents/implementation.md` | unchanged | - New tools are written in Python and portab… |
| I07 | `agents/implementation.md:45` | task-scoped | `agents/implementation.md` | unchanged | - No new PowerShell tools. The existing ones… |
| M01 | `agents/measurement.md:3` | task-scoped | `agents/measurement.md` | reworded: the header; its second sentence is unchanged | Read this for V leaves and whenever a task b… |
| M02 | `agents/measurement.md:10` | task-scoped | `agents/measurement.md` | unchanged | - After PLAN A.3, Colosseum CLI is the main… |
| M03 | `agents/measurement.md:17` | task-scoped | `agents/measurement.md` | unchanged | - Never measure strength or NPS on a TEXEL o… |
| M04 | `agents/measurement.md:19` | task-scoped | `agents/measurement.md` | unchanged | - Measure only on an idle host: check CPU us… |
| M05 | `agents/measurement.md:22` | task-scoped | `agents/measurement.md` | unchanged | - Rebuild before measuring, with the exact p… |
| M06 | `agents/measurement.md:26` | task-scoped | `agents/measurement.md` | unchanged | - bench and Diag counters are per position;… |
| M07 | `agents/measurement.md:30` | task-scoped | `agents/measurement.md` | reworded: M11's citation joins it | - An LLVM major step is a speed change even… |
| M08 | `agents/measurement.md:31` | task-scoped | `agents/measurement.md` | unchanged | - It owes a pooled NPS read, registered with… |
| M09 | `agents/measurement.md:33` | task-scoped | `agents/measurement.md` | unchanged | - Patch releases owe none. CI prints them, a… |
| M10 | `agents/measurement.md:35` | task-scoped | `agents/measurement.md` | unchanged | - No compiler step, a patch included, falls… |
| M11 | `agents/measurement.md:37` | rationale | this packet | moved out; its citation stays on M07 | - Rarog's LLVM 22 to 23 step kept its finger… |
| M12 | `agents/measurement.md:39` | task-scoped | `agents/measurement.md`, this packet | reworded: the incident moved out, its citation stays | - A SQLite database is copied together with… |
| M13 | `agents/measurement.md:42` | task-scoped | `agents/measurement.md` | unchanged | - A binary entered in a rated pool is a tagg… |
| M14 | `agents/measurement.md:45` | task-scoped | `agents/measurement.md` | unchanged | - The current fingerprint is declared once,… |
| M15 | `agents/measurement.md:51` | task-scoped | `agents/measurement.md` | unchanged | - A playing change needs deterministic regre… |
| M16 | `agents/measurement.md:54` | task-scoped | `agents/measurement.md` | unchanged | - Build the actual candidate configuration t… |
| M17 | `agents/measurement.md:57` | task-scoped | `agents/measurement.md` | unchanged | - Score-based game adjudication is off by de… |
| M18 | `agents/measurement.md:59` | task-scoped | `agents/measurement.md` | unchanged | - Never accept a candidate that fails a hard… |
| M19 | `agents/measurement.md:61` | task-scoped | `agents/measurement.md` | unchanged | - Prove a guard or harness wire live before… |
| M20 | `agents/measurement.md:67` | task-scoped | `agents/measurement.md` | unchanged | - The strength unit is one dependency-comple… |
| M21 | `agents/measurement.md:71` | task-scoped | `agents/measurement.md` | unchanged | - [0,3] nElo is the default bracket; sprt.ps… |
| M22 | `agents/measurement.md:76` | task-scoped | `agents/measurement.md` | reworded: M26's citation joins it | - **Fit the gate to the change; never copy t… |
| M23 | `agents/measurement.md:79` | task-scoped | `agents/measurement.md` | unchanged | - a strength claim: the SPRT bracket above,… |
| M24 | `agents/measurement.md:81` | task-scoped | `agents/measurement.md` | unchanged | - a repair kept unless it harms (H1 and the… |
| M25 | `agents/measurement.md:88` | task-scoped | `agents/measurement.md` | unchanged | - a change that cannot alter a move (output,… |
| M26 | `agents/measurement.md:89` | rationale | this packet | moved out; its citation stays on M22 | BAS-S32 and BAS-S33 ran 4,000-pair caps, cop… |
| M27 | `agents/measurement.md:91` | task-scoped | `agents/measurement.md` | unchanged | - High bounds reject small gains. That is ov… |
| M28 | `agents/measurement.md:93` | task-scoped | `agents/measurement.md` | unchanged | - Bench and counter screens choose candidate… |
| M29 | `agents/measurement.md:95` | task-scoped | `agents/measurement.md` | unchanged | - An unresolved stop is not "probably fine":… |
| M30 | `agents/measurement.md:97` | task-scoped | `agents/measurement.md` | unchanged | - One gate, one read: an SPRT's verdict and… |
| M31 | `agents/measurement.md:99` | task-scoped | `agents/measurement.md` | unchanged | - **First-crossing estimates.** The estimate… |
| M32 | `agents/measurement.md:102` | task-scoped | `agents/measurement.md`, this packet | reworded: the incident moved out, its citation stays | - The bias is not one-sided: a fixed 600-gam… |
| M33 | `agents/measurement.md:104` | task-scoped | `agents/measurement.md` | unchanged | - A stopped SPRT is never extended, and neve… |
| M34 | `agents/measurement.md:106` | task-scoped | `agents/measurement.md` | unchanged | - A stopped SPRT is the wrong instrument for… |
| M35 | `agents/measurement.md:107` | task-scoped | `agents/measurement.md` | unchanged | - SPSA is conditional: first show activation… |
| M36 | `agents/measurement.md:115` | task-scoped | `agents/measurement.md` | unchanged | - Agents may run short builds, benches, focu… |
| M37 | `agents/measurement.md:117` | task-scoped | `agents/measurement.md` | unchanged | - Do not start long SPRTs, SPSAs, tournament… |
| M38 | `agents/measurement.md:120` | task-scoped | `agents/measurement.md` | unchanged | - For a required long run, prepare and valid… |
| M39 | `agents/measurement.md:124` | task-scoped | `agents/measurement.md` | unchanged | - After the maintainer returns the artifacts… |
| M40 | `agents/measurement.md:127` | task-scoped | `agents/measurement.md` | unchanged | - Respect temporary resource reservations st… |
| M41 | `agents/measurement.md:129` | task-scoped | `agents/measurement.md` | unchanged | - The host is shared with Rarog. Never start… |
| D01 | `agents/records.md:3` | task-scoped | `agents/records.md` | reworded: the header | Read this for M leaves, before closing any l… |
| D02 | `agents/records.md:9` | task-scoped | `agents/records.md` | unchanged | - A roadmap status or requirement change is… |
| D03 | `agents/records.md:12` | task-scoped | `agents/records.md` | unchanged | - PLAN carries status. Tick a step only when… |
| D04 | `agents/records.md:14` | task-scoped | `agents/records.md` | unchanged | - CHANGELOG.md's [Unreleased] is kept curren… |
| D05 | `agents/records.md:21` | task-scoped | `agents/records.md` | unchanged | - A leaf's first line is its identifier, cap… |
| D06 | `agents/records.md:24` | task-scoped | `agents/records.md` | unchanged | - Sub-steps indent by 4 spaces; nothing goes… |
| D07 | `agents/records.md:27` | task-scoped | `agents/records.md` | unchanged | - Keep GUIDE a one-page overview: its *Now*… |
| D08 | `agents/records.md:31` | task-scoped | `agents/records.md` | unchanged | - The experiment ledger is an index, docs/EX… |
| D09 | `agents/records.md:34` | task-scoped | `agents/records.md` | unchanged | - docs/HISTORY.md is history and resolves ev… |
| D10 | `agents/records.md:41` | task-scoped | `agents/records.md` | unchanged | - Git holds source, build and CI files, reus… |
| D11 | `agents/records.md:45` | task-scoped | `agents/records.md` | unchanged | - Information lives in the tracked documents… |
| D12 | `agents/records.md:49` | task-scoped | `agents/records.md` | unchanged | - A document cites only commits that stay re… |
| D13 | `agents/records.md:60` | task-scoped | `agents/records.md` | unchanged | - No preservation tags and no kept branches… |
| R01 | `agents/research.md:3` | task-scoped | `agents/research.md` | reworded: the header; its last sentence is unchanged | Read this for research and diagnosis, experi… |
| R02 | `agents/research.md:9` | task-scoped | `agents/research.md` | unchanged | - Read docs/DESIGN.md first. It holds the en… |
| R03 | `agents/research.md:11` | task-scoped | `agents/research.md` | unchanged | - Answer these four questions **in the repor… |
| R04 | `agents/research.md:12` | task-scoped | `agents/research.md` | unchanged | 1. What mechanism should produce strength? N… |
| R05 | `agents/research.md:14` | task-scoped | `agents/research.md` | unchanged | 2. What existing features interact with it?… |
| R06 | `agents/research.md:16` | task-scoped | `agents/research.md` | unchanged | 3. What engine invariants must remain true?… |
| R07 | `agents/research.md:18` | task-scoped | `agents/research.md` | unchanged | 4. What experiment would falsify the idea? R… |
| R08 | `agents/research.md:20` | task-scoped | `agents/research.md` | unchanged | - Chess-engine techniques are not independen… |
| R09 | `agents/research.md:25` | task-scoped | `agents/research.md` | unchanged | - Consult docs/EXPERIMENTS.md before answeri… |
| R10 | `agents/research.md:27` | task-scoped | `agents/research.md` | unchanged | - For a substantial playing change, also sta… |
| R11 | `agents/research.md:31` | task-scoped | `agents/research.md` | unchanged | - Prefer interaction-first analysis. Check f… |
| R12 | `agents/research.md:37` | task-scoped | `agents/research.md` | unchanged | - **Recommend before you document.** When re… |
| R13 | `agents/research.md:44` | task-scoped | `agents/research.md` | unchanged; its last sentence is A17's but for one word | - Price the experiment before substantial wo… |
| R14 | `agents/research.md:51` | task-scoped | `agents/research.md`, this packet | reworded: the incident moved out, its citation stays | - Donor engines teach mechanisms, contracts,… |
| R15 | `agents/research.md:62` | task-scoped | `agents/research.md` | unchanged | - Rarog is read only through the pinned snap… |
| R16 | `agents/research.md:71` | task-scoped | `agents/research.md` | unchanged | - What may cross, and how the code is writte… |
| R17 | `agents/research.md:78` | task-scoped | `agents/research.md` | unchanged | Refusing to build something, and refuting a… |
| R18 | `agents/research.md:82` | task-scoped | `agents/research.md` | unchanged | - **Say "this does not fit; do not implement… |
| R19 | `agents/research.md:87` | task-scoped | `agents/research.md`, this packet | reworded: the incident moved out, its citation stays | - **Challenge the plan when the evidence doe… |
| R20 | `agents/research.md:93` | task-scoped | `agents/research.md` | unchanged | - **Report the cost even when the change wor… |
| R21 | `agents/research.md:96` | task-scoped | `agents/research.md` | unchanged | - **A refusal must be falsifiable too.** Sta… |
| R22 | `agents/research.md:100` | task-scoped | `agents/research.md` | unchanged | - **Do not soften a negative result to match… |
| R23 | `agents/research.md:106` | task-scoped | `agents/research.md` | unchanged | - Before closing an experimental leaf, audit… |
| R24 | `agents/research.md:112` | task-scoped | `agents/research.md` | unchanged | - Report an experimental-design confound or… |
| R25 | `agents/research.md:116` | task-scoped | `agents/research.md` | unchanged | - Freeze the prospective prediction, confide… |
| R26 | `agents/research.md:121` | task-scoped | `agents/research.md` | unchanged | - Keep evidence layers separate. Better fit… |

### Rewordings: old text and new

**A02** (`AGENTS.md:14`), old:

> This file is always in force. Before starting, read the `agents/` files the task needs; most leaves need more than one, and closing any leaf needs `agents/records.md`.

new:

> This file is always in force. The files under `agents/` are modules, as binding as this file: read one in full before starting when any of its triggers is true, and skip it only because none is, never to save time. Triggers are facts -- the task's kind, the paths changed, the commands run -- and most leaves fire several. When unsure, read the module.
>
> -- `AGENTS.md`

**A03** (`AGENTS.md:20`), old:

> | Research, diagnosis, experiment design, reviews; `R2`/`R3` leaves; proposing or changing a mechanism | `agents/research.md` |

new:

> Scope: mechanisms, research, reviews, donors, refusals and experiment design. Read in full when: Task -- research, diagnosis, experiment design or a review; any `R2` or `R3` leaf. Paths -- `docs/DESIGN.md`, `docs/experiments/`, `analysis/`. Actions -- before proposing, adding, removing or altering a mechanism, a code change meant to alter a move included; when a change moves the bench fingerprint; before reading another engine's source or `docs/reference/`; before registering an experiment or closing a leaf that ran one.
>
> -- `AGENTS.md`

**A04** (`AGENTS.md:21`), old:

> | Engine or tool code; `I1`/`I2` leaves | `agents/implementation.md` (plus `agents/research.md` *Before implementing a mechanism* for any playing change) |

new:

> Scope: engine, test, build and tool code. Read in full when: Task -- any `I1` or `I2` leaf; a review of code. Paths -- `src/`, `tests/`, `tools/`, `cmake/`, `CMakeLists.txt`, `CMakePresets.json`, `.github/workflows/`. Actions -- before writing or changing code anywhere else.
>
> -- `AGENTS.md`

> Scope: mechanisms, research, reviews, donors, refusals and experiment design. Read in full when: Task -- research, diagnosis, experiment design or a review; any `R2` or `R3` leaf. Paths -- `docs/DESIGN.md`, `docs/experiments/`, `analysis/`. Actions -- before proposing, adding, removing or altering a mechanism, a code change meant to alter a move included; when a change moves the bench fingerprint; before reading another engine's source or `docs/reference/`; before registering an experiment or closing a leaf that ran one.
>
> -- `AGENTS.md`

**A05** (`AGENTS.md:22`), old:

> | Builds to measure, bench/NPS/counters, games, SPRT, SPSA, gauntlets, datagen, fits, long runs, returned artifacts; `V` leaves | `agents/measurement.md` |

new:

> Scope: measurement, acceptance, gates and long runs. Read in full when: Task -- any `V` leaf. Paths -- `docs/experiments/`, `docs/EXPERIMENTS.md`, `tools/colosseum/`, `tools/run_*`, `.github/windows-clang-version`. Actions -- before building a binary to measure; before running or reading bench, NPS, `Diag` counters or a profile; before running, registering or preparing games, an SPRT, SPSA, gauntlet, datagen or fit; before handing the maintainer a command or launcher; before any run that loads the machine for minutes; before reading returned artifacts.
>
> -- `AGENTS.md`

**A06** (`AGENTS.md:23`), old:

> | PLAN, GUIDE, HISTORY, EXPERIMENTS or `analysis/` edits, evidence placement, tags; `M` leaves; closing any leaf | `agents/records.md` |

new:

> Scope: roadmap and ledger documents, evidence, citations, tags, capability classes and the pull-request message. Read in full when: Task -- any `M` leaf; closing any leaf. Paths -- `docs/PLAN.md`, `GUIDE.md`, `CHANGELOG.md`, `docs/HISTORY.md`, `docs/EXPERIMENTS.md`, `docs/experiments/`, `docs/archive/`, `analysis/`, `tools/diag/citation_exceptions.tsv`. Actions -- before citing a commit in a document; before adding a result, log or fixture to git; before creating, pushing or deleting a tag, or keeping a branch; before opening a pull request or writing its message; before changing a capability tag; when a change is user-visible (strength, UCI options or output, defaults, the bench fingerprint, platforms).
>
> -- `AGENTS.md`

**A48** (`AGENTS.md:183`), old:

> - `master` is the trunk. Every coherent piece of work -- one feature or experiment, or one tooling or documentation change -- lives on its own short-lived branch from `master` and lands as ONE squash commit through a pull request: no merge commits, no rebase merges. On the branch, commit in small verified steps after every completed step or substep; GitHub keeps those commits under the PR (`refs/pull/<n>/head`) after the squash. Before opening the PR, merge `master` into the branch if `master` moved, so the PR shows only the branch's own change. After the merge the branch is deleted (PROCESS *Delivery*).

new:

> - `master` is the trunk. Every coherent piece of work -- one feature or experiment, or one tooling or documentation change -- lives on its own short-lived branch from `master` and lands as ONE squash commit through a pull request: no merge commits, no rebase merges. On the branch, commit in small verified steps after every completed step or substep; GitHub keeps those commits under the PR (`refs/pull/<n>/head`) after the squash.
>
> -- `AGENTS.md`

> - Before opening the PR, merge `master` into the branch if `master` moved, so the PR shows only the branch's own change. After the merge the branch is deleted (PROCESS *Delivery*).
>
> -- `agents/records.md`

**I01** (`agents/implementation.md:3`), old:

> Read this for `I1` and `I2` leaves and any change to engine source or tools. Workflow ownership (what implementation owns and must not change) is in `AGENTS.md`. A change that adds, removes or alters a mechanism also needs `agents/research.md`, *Before implementing a mechanism*; one that is measured or gated also needs `agents/measurement.md`.

new:

> Scope: engine, test, build and tool code. Read in full when: Task -- any `I1` or `I2` leaf; a review of code. Paths -- `src/`, `tests/`, `tools/`, `cmake/`, `CMakeLists.txt`, `CMakePresets.json`, `.github/workflows/`. Actions -- before writing or changing code anywhere else.
>
> -- `agents/implementation.md`

> The workflow states and the return of a false premise are in `AGENTS.md`. A change that adds, removes or alters a mechanism also needs `agents/research.md`, *Before implementing a mechanism*; one that is measured or gated also needs `agents/measurement.md`.
>
> -- `agents/implementation.md`

**M01** (`agents/measurement.md:3`), old:

> Read this for `V` leaves and whenever a task builds a binary to measure, reads bench, NPS or `Diag` counters, plays games, runs or prepares an SPRT, SPSA, gauntlet, datagen or fit, or reads returned artifacts. Register an experiment by `agents/research.md`, *Experiment design*, before it runs.

new:

> Scope: measurement, acceptance, gates and long runs. Read in full when: Task -- any `V` leaf. Paths -- `docs/experiments/`, `docs/EXPERIMENTS.md`, `tools/colosseum/`, `tools/run_*`, `.github/windows-clang-version`. Actions -- before building a binary to measure; before running or reading bench, NPS, `Diag` counters or a profile; before running, registering or preparing games, an SPRT, SPSA, gauntlet, datagen or fit; before handing the maintainer a command or launcher; before any run that loads the machine for minutes; before reading returned artifacts.
>
> -- `agents/measurement.md`

> Register an experiment by `agents/research.md`, *Experiment design*, before it runs.
>
> -- `agents/measurement.md`

**M07** (`agents/measurement.md:30`), old:

> - An LLVM major step is a speed change even at an identical fingerprint.

new:

> - An LLVM major step is a speed change even at an identical fingerprint (RAR-P36 to RAR-P38).
>
> -- `agents/measurement.md`

**M12** (`agents/measurement.md:39`), old:

> - A SQLite database is copied together with its `-wal` file, or read in place. Rarog's copy without it returned 39,437 of 39,600 games and a plausible but wrong termination table (RAR-M45).

new:

> - A SQLite database is copied together with its `-wal` file, or read in place (RAR-M45).
>
> -- `agents/measurement.md`

> Rarog's copy without it returned 39,437 of 39,600 games and a plausible but wrong termination table (RAR-M45).
>
> -- this packet, below

**M22** (`agents/measurement.md:76`), old:

> - **Fit the gate to the change; never copy the last launcher's method or cap.** The registration names the question the games must answer, the cheapest method that answers it, and why:

new:

> - **Fit the gate to the change; never copy the last launcher's method or cap.** The registration names the question the games must answer, the cheapest method that answers it, and why (BAS-S32, BAS-S33):
>
> -- `agents/measurement.md`

**M32** (`agents/measurement.md:102`), old:

> - The bias is not one-sided: a fixed 600-game run read +70 where Rarog's SPRT had stopped at +54.8 (Rarog PROCESS *Harness*, 2026-10-11).

new:

> - The bias is not one-sided (Rarog PROCESS *Harness*, 2026-10-11).
>
> -- `agents/measurement.md`

> a fixed 600-game run read +70 where Rarog's SPRT had stopped at +54.8 (Rarog PROCESS *Harness*, 2026-10-11).
>
> -- this packet, below

**D01** (`agents/records.md:3`), old:

> Read this for `M` leaves, before closing any leaf, and whenever a task edits PLAN, GUIDE, HISTORY, EXPERIMENTS or `analysis/`, places evidence, or touches tags.

new:

> Scope: roadmap and ledger documents, evidence, citations, tags, capability classes and the pull-request message. Read in full when: Task -- any `M` leaf; closing any leaf. Paths -- `docs/PLAN.md`, `GUIDE.md`, `CHANGELOG.md`, `docs/HISTORY.md`, `docs/EXPERIMENTS.md`, `docs/experiments/`, `docs/archive/`, `analysis/`, `tools/diag/citation_exceptions.tsv`. Actions -- before citing a commit in a document; before adding a result, log or fixture to git; before creating, pushing or deleting a tag, or keeping a branch; before opening a pull request or writing its message; before changing a capability tag; when a change is user-visible (strength, UCI options or output, defaults, the bench fingerprint, platforms).
>
> -- `agents/records.md`

**R01** (`agents/research.md:3`), old:

> Read this for research and diagnosis, experiment design, `R2` and `R3` leaves, reviews, and before any change that adds, removes or alters a mechanism. `AGENTS.md` holds the rules every task shares.

new:

> Scope: mechanisms, research, reviews, donors, refusals and experiment design. Read in full when: Task -- research, diagnosis, experiment design or a review; any `R2` or `R3` leaf. Paths -- `docs/DESIGN.md`, `docs/experiments/`, `analysis/`. Actions -- before proposing, adding, removing or altering a mechanism, a code change meant to alter a move included; when a change moves the bench fingerprint; before reading another engine's source or `docs/reference/`; before registering an experiment or closing a leaf that ran one.
>
> -- `agents/research.md`

> `AGENTS.md` holds the rules every task shares.
>
> -- `agents/research.md`

**R14** (`agents/research.md:51`), old:

> - Donor engines teach mechanisms, contracts, dependencies, failure modes and methods. The donors are modern Stockfish (search, TT, histories, time, threads, later NNUE runtime), classical Stockfish (`sf_16`'s evaluation families and seed values, `9587eeeb`'s as the second column and as the oracle) and Triumviratus (measured priors on a Stockfish-19-shaped search, board, time, NNUE and measurement), read only through `docs/reference/triumviratus*/` and mapped to leaves by `analysis/triumviratus_review_2026-11-03.md`. Its verdicts are priors. Rarog is a sibling whose *method* this roadmap follows and whose results are imported priors; it is not a donor, and its verdicts do not transfer (BAS-X01: check-extension removal was +30.75 there and −10.17 here).

new:

> - Donor engines teach mechanisms, contracts, dependencies, failure modes and methods. The donors are modern Stockfish (search, TT, histories, time, threads, later NNUE runtime), classical Stockfish (`sf_16`'s evaluation families and seed values, `9587eeeb`'s as the second column and as the oracle) and Triumviratus (measured priors on a Stockfish-19-shaped search, board, time, NNUE and measurement), read only through `docs/reference/triumviratus*/` and mapped to leaves by `analysis/triumviratus_review_2026-11-03.md`. Its verdicts are priors. Rarog is a sibling whose *method* this roadmap follows and whose results are imported priors; it is not a donor, and its verdicts do not transfer (BAS-X01).
>
> -- `agents/research.md`

> BAS-X01: check-extension removal was +30.75 there and −10.17 here
>
> -- this packet, below

**R19** (`agents/research.md:87`), old:

> - **Challenge the plan when the evidence does not support it.** PLAN and EXPERIMENTS are the maintainer's working beliefs, not settled fact. An unmeasured claim carried in the roadmap is a target, not an authority. When a step's stated premise is wrong, say so, measure it, and correct the file -- BAS-E53 exists because a load-bearing claim had never been measured and turned out to be false.

new:

> - **Challenge the plan when the evidence does not support it.** PLAN and EXPERIMENTS are the maintainer's working beliefs, not settled fact. An unmeasured claim carried in the roadmap is a target, not an authority. When a step's stated premise is wrong, say so, measure it, and correct the file (BAS-E53).
>
> -- `agents/research.md`

> BAS-E53 exists because a load-bearing claim had never been measured and turned out to be false.
>
> -- this packet, below

### Incident text moved out of the rules

| ID | Text, word for word |
|---|---|
| M11 | - Rarog's LLVM 22 to 23 step kept its fingerprint and cost 1.5–1.7% pooled with PGO (RAR-P36 to RAR-P38). |
| M12 | Rarog's copy without it returned 39,437 of 39,600 games and a plausible but wrong termination table (RAR-M45). |
| M26 | BAS-S32 and BAS-S33 ran 4,000-pair caps, copied from a template, on changes touching about 1% of games. |
| M32 | a fixed 600-game run read +70 where Rarog's SPRT had stopped at +54.8 (Rarog PROCESS *Harness*, 2026-10-11). |
| R14 | BAS-X01: check-extension removal was +30.75 there and −10.17 here |
| R19 | BAS-E53 exists because a load-bearing claim had never been measured and turned out to be false. |

<!-- END GENERATED -->

The count of normative words is a second look, not the proof. It rises
where the new rules N1 to N6 and the index use these words. It falls twice,
neither time with a rule: one "must" was in the pointer I01 carried to the
ownership bullets ("what implementation owns and must not change"), and one
"never" is in R19's incident sentence, kept above.

## 6. The triggers, tried on work already done

Each task's changed files were run through the path triggers as they now
stand (`check_rules.fired` on the commit's file list); the task and action
triggers were judged by hand. "Needed" names the rules the task turned on,
by what went right or wrong in it.

| Task | Kind | Fired by path (files) | Fired by task or action | Rules it needed | Reached |
|---|---|---|---|---|---|
| `4b14969` Rebuild quiescence | code that changes behaviour | implementation (51), records (19), measurement (15), research (14) | research: altering a mechanism | the four questions (R03–R07); what implementation owns and does not (A18, A19, now in implementation); the return of a false premise (A20, core); per-node code (I02); the cluster's gate (M20–M22) | all |
| `43b4c35` Stop computing the zero-weight evaluation terms | behaviour-neutral repair | implementation (8), records (7), measurement (5), research (2) | measurement: a binary built to measure | the fingerprint (A37, core); bench identity is not behavioural identity (I03); an idle host, a rebuilt binary, no long read unasked (M03–M05, M37); the changelog entry (D04) | all |
| `0b5911e` Archive Phase B and trim the documents | documents only | implementation (18: comments in `src/`), records (6), research (2), measurement (2) | records: closing leaves | regenerate GUIDE and run the roadmap check (A12, now in records; D02); HISTORY and the archive are never the next step (D09); cited commits stay reachable (D12); comments carry no step numbers (I05) | all |
| `050ad18` Record the code audit and plan its repairs | research | records (3), research (1) | research: an `R2` audit | refutation as a deliverable (R17–R22); chess terms (A33, core); recommend before documenting (R12); PLAN's form (D05, D06) | all |
| Prepare C.3.4: the launcher, then the registered reads with their one command (two commits on the working branch) | a measurement handed to the maintainer | launcher: implementation (3), measurement (1); registration: research (2), measurement (2), records (2) | measurement: a command for the maintainer | prepare, commit, one command, stop, the leaf stays open (M37, M38); the candidate actually tested (M16); new tools in Python (I06); the prediction frozen before exposure (R25) | all |
| `f1e31f4` Version 1.11.0, and `1e68ec6` the release page | release | records (2; 1), implementation (1; 3) | records: a tag, a pull request | the changelog's summary and Highlights (D04, which `1e68ec6` added after the page showed everything); tags are the maintainer's commands (D13); the squash message (A49, now in records); no push unless asked (A51, core) | all |
| Close C.3.3: the rebuild's independent review (on the working branch) | review | records (3), research (1) | research: a review; implementation: a review of code | the review is independent and reproduces the fingerprints (A21, now in research); the rules the code is checked against (I02, I04, I05); tick only what is finished and verified (D03) | all |
| `00809cc`, which registered BAS-S32 and BAS-S33 | a gate's registration | implementation (18), measurement (7), records (7), research (4) | measurement: registering an SPRT | fit the gate to the change (M22, written after these two caps were copied from a template) | all |
| `5f61c94`, B.5.7's reads: a new PowerShell launcher and Windows-only calls | a tool | implementation (81), records (28), measurement (25), research (23) | — | new tools are Python and portable (I06, I07, written after it) | all |

Four defects showed, and each was fixed in the trigger, not in the record:

1. The audit and the review reached `agents/research.md` only by the kind of
   task. `analysis/` is now one of its paths, so the diff shows it too.
2. A launcher handed to the maintainer reached `agents/measurement.md` only
   by an action. `tools/run_*` is now one of its paths.
3. "A donor's source" did not cover the live Rarog repository, which is not a
   donor and is the one the rule forbids. The trigger now says "another
   engine's source".
4. A reviewer was sent to `agents/research.md` and not to the rules the code
   is checked against. "A review of code" is now a trigger of
   `agents/implementation.md`.

## 7. For the maintainer's decision

Nothing below was changed. Each item gives the evidence and a
recommendation.

1. **"The selected PLAN section" (A44) does not say how much.** One leaf's
   entry is 1.5 KB at the median and 10 KB at most; its phase is 133 KB for
   Phase C; PROCESS's research prompt says "Read PLAN", which is 234 KB.
   Recommend: "the selected leaf's entry in PLAN and its parent step's lead
   text; PLAN §2 only for a rule a module cites by number", and the same in
   the research prompt.
2. **Near duplicates, left in place because they are not word for word.**
   A17's last sentence and R13's differ by one word and now sit in the same
   module: recommend deleting it from *Ownership*. A12 and D02 both say to
   regenerate GUIDE; A47's last sentence and M20 both forbid changing a
   registered run after its results; A19 and I04 both forbid relaxing a
   correctness test; C03 restates A51 for Claude Code. The capability
   classes are worded three ways (A23–A28, PLAN §2, GUIDE): recommend
   keeping PLAN's table and cutting the one now in `agents/records.md`.
3. **The six new rules, N1 to N6 in §4,** and the wider triggers listed
   under them. Recommend keeping all; N2's "name the modules read" is what
   makes a skipped module visible in a report.
4. **Two snapshots carry a file named `AGENTS.md`.** A tool that loads the
   nearest `AGENTS.md` to the files it works on would load Rarog's rules,
   which never apply here (`agents/research.md`, *Donors*). The snapshots
   are byte-exact against their manifests, so the files cannot be renamed.
   Whether any tool in use does this was not tested. The check exempts
   `docs/reference/` and reports the count. Recommend leaving it, and
   deciding again if a second tool joins.
5. **`tools/colosseum/sprt-removal.toml` cites `AGENTS.md`** for the
   `[-5,0]` bracket, which is in `agents/measurement.md`, *Gating*, and was
   before this change. A one-line fix, not made here.
6. **Windsurf.** If it ever reads these rules and its 12,000 characters turn
   out to apply to `AGENTS.md`, the core is over (13,360). The budget would
   then be 9,000, and the next rules to leave the core are in item 8.
7. **Claude Code's target of 200 lines** for a file is a recommendation, not
   a cut. `AGENTS.md` is 228 lines, and was 222.
8. **More headroom, if wanted.** The fingerprint bullet (A37, 590 bytes)
   could go to `agents/implementation.md`; `check_roadmap.py`'s anchor, a
   test fixture, M14 and PLAN's rule table would follow it. Recommend not
   yet: three of four modules' work needs the number.

## 8. Checks

Run on the final state, each exit status read directly:

| Command | Exit |
|---|---|
| `python tools/diag/check_rules.py` | 0 |
| `python tools/diag/test_check_rules.py` (17 tests, every failure the check names) | 0 |
| `python tools/diag/rules_inventory.py` | 0 |
| `python tools/diag/check_roadmap.py` | 0 |
| `python tools/diag/check_roadmap.py --self-test` | 0 |
| `python tools/diag/check_citations.py` | 0 |
| `python tools/diag/test_release_check.py` | 0 |

The negative check: a copy of the rule files outside the repository, its
core grown past the budget, must fail `check_rules.py --root <copy>`, and
the unchanged copy must pass. Both held (exit 1 with "over the budget of
18,000", and exit 0).

Not verified: Codex's, Gemini CLI's, Cursor's, Copilot's and Junie's rows in
§2 were read once and not against the page text; no tool other than Claude
Code was run against these files, so every limit here is the documented one
and none was observed.
