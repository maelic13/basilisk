# Licence and originality audit (2026-11-23)

- State / class: research, no engine or rule change, `R2`
- Owner / date: agent, 2026-11-23, at the maintainer's request after
  Rarog's licence decision of the same day. The live Rarog files read,
  at its head of 2026-11-23 (9bf4f2b), are
  `analysis/coda_read_2026-11-23.md`, `docs/PROCESS.md` *The independence
  boundary* and `docs/PLAN.md` D.11.
- Not legal advice. The maintainer decides.

**Recommendation.** Open one new step and do its first part this week:
take the Reckless patch out of the tree and adopt Rarog's rule for AGPL
and unlicensed engines. It is Basilisk's only incompatible material, and
the step is an hour of `M` work. The rest of the step is notices for past
releases, and reaching originality. That part can ride on leaves that
already refit the same constants (C.2.8, D.1, Phase C's families), so it
costs no extra games if it is decided before C.2.8 registers its start.

## 1. Method and what was checked

- **Licences** were read on each donor's own repository, not from memory:
  - GitHub's API gave each repository's licence and its LICENSE or
    COPYING commit history: Reckless, Triumviratus, Triumviratus-Networks,
    Fathom (`jdart1/Fathom`, and `basil00/Fathom` before it),
    `official-stockfish/books` and Disservin's `chess-library`.
  - Stockfish's `Copying.txt` and source headers were read at each tag
    Basilisk uses, in the local checkout.
  - The Chess Programming Wiki's footer was read, with its PeSTO page.
- **The tracked tree**: every file in `git ls-files`, by directory. Every
  `.patch`, `.diff`, `.epd`, `.pgn` and network file. Every mention of
  Reckless outside `docs/reference/`. Fenced code blocks near a donor's
  name in all tracked Markdown (none quote donor code: §2.5).
- **Published history**: every file ever added on any ref (1,506 paths),
  the four non-release tags on `origin`, and the releases on GitHub with
  their assets and notes.
- **The engine**: `tools/diag/originality_scan.py` ran on `src/` at
  `HEAD` against three engines:
  - Reckless `8d7d1dd` (2026-11-13);
  - Rarog 9bf4f2b (2026-11-23);
  - Triumviratus `0c18907`'s source, from the snapshot.
  The tool reads C++ only, so the Rust trees were scanned as renamed
  copies, which works for numbers and tokens. B.11.3 had already scanned
  against both Stockfishes (`analysis/b113_originality_2026-11-18.md`);
  that result is reused, not repeated. The outputs are in the session
  scratchpad, untracked.
- **Authorship**: author and committer of every commit on every ref.

## 2. Inventory

Licence is at the version Basilisk read or took. "Ships" means it is
compiled into, or packaged with, a release binary.

### 2.1 Code and transcriptions in the engine (ships)

| Item | Where | Source and version | Licence then |
|---|---|---|---|
| Windows large-page privilege set-up, close to the donor | `src/large_pages.cpp:59–80` | Stockfish `9587eeeb` `misc.cpp` | GPL-3.0-or-later |
| Cuckoo table of reversible moves | `src/zobrist.cpp:68–79` | Stockfish `9587eeeb` `position.cpp` | GPL-3.0-or-later |
| Compile date from `__DATE__` | `src/version.cpp:26–35` | Stockfish `sf_19` `misc.cpp` | GPL-3.0-or-later |
| Search, TT, histories, ordering, threads: the design | `src/search_*` | Stockfish `sf_19` | ideas; GPL-3.0-or-later |
| Endgame scaling rules around KRP-KR, as conditions | `src/eval.cpp:635–701` | Stockfish `9587eeeb` `endgame.cpp` | GPL-3.0-or-later |
| Syzygy prober, vendored | `external/fathom/` | Fathom (de Man, basil00, Dart) | MIT |

The scans found no code from Reckless, Triumviratus or Rarog in `src/`:

- **Reckless:** 2 raw token runs and 4 normalised. They are the square
  enumeration and three bench FENs: the standard perft positions from
  the Chess Programming Wiki, which both engines open with.
- **Triumviratus:** the longest raw run (97 tokens) is the castling-rights
  mask, the classic 13/15/14/7/3/11 table. Basilisk has had it since
  2023, and it predates both engines. The rest are `#include` lists and
  enumerations.
- **Rarog:** the matches are fixtures the two engines share by design
  (§5).

### 2.2 Constants, including seeds a fit later moved (ships)

| Item | Where | Source | Licence then | State |
|---|---|---|---|---|
| The time manager's formula and its constants | `src/search_worker.cpp:97` | `sf_19` `timeman.cpp` | GPL-3.0-or-later | unchanged; D.1 rebuilds the time manager in Stockfish's shape |
| LMR history divisors (16) | `src/search_kernel_core.cpp:140` | `sf_19` `search.cpp` | GPL-3.0-or-later | entries unchanged; one fitted scale |
| Continuation-consistency multipliers (7) | `src/search_history_core.cpp:30` | `sf_19` `search.cpp` | GPL-3.0-or-later | unchanged |
| Piece values inside the histories | `src/move_picker_core.h:33` | `sf_19` `types.h` | GPL-3.0-or-later | unchanged, by design |
| Search literals left at their seed: not fitted, or rounded back by the fit | listed in `analysis/b67_kernel_literals.md` | `sf_19` | GPL-3.0-or-later | B.6.7 moved 93 of 118 |
| Endgame scaling rules' constants | `src/eval.cpp:635–701` | `9587eeeb` | GPL-3.0-or-later | unchanged |
| Search constants seeded from Stockfish and since fitted | across `src/search_*` | `sf_19`, converted at 0.282 | GPL-3.0-or-later | moved |
| Bench, magics, Zobrist keys | `src/bench.cpp`, `magics.h`, `zobrist.cpp` | Basilisk's own; splitmix64's three constants | public domain (splitmix64) | own |

### 2.3 Tables, layouts and index constructions (ships)

| Item | Where | Source | Licence then | State |
|---|---|---|---|---|
| Piece-square tables | `src/eval_params.h:28` (mg), `:85` (eg) | PeSTO, Ronald Friederich; first posted on TalkChess 2018-08-28 and published on the Chess Programming Wiki | CC BY-SA 3.0, the wiki's licence; the page grants nothing else | 604 of 736 entries still PeSTO's (`analysis/pst_orientation_2026-10-23.md`); C.2.8 refits them from `sf_16`'s tables |
| Six values of one pawn row | `src/eval_params.h:88` | `9587eeeb` `psqt.cpp` | GPL-3.0-or-later | goes with the refit |
| Castling mask, square and direction enumerations | `src/board.h:273`, `types.h` | convention | none: functional | own form |
| WAC test positions, compiled in | `src/wac_epd.h` (from `src/wac.epd`) | Reinfeld's *Win at Chess* (1958), the widely circulated EPD; byte-identical to Rarog's | positions are facts; the compilation names no licence | functional |
| Bench FENs, three of them the standard perft positions | `src/bench.cpp:29` | Chess Programming Wiki; the rest Basilisk's, shared with Rarog | facts | functional |

Basilisk has no network, no NNUE feature layout and no index
construction from any engine. The `archive/nnue-local` tag's
`tests/data/test_h16.mnn` came from the maintainer's own trainer.

### 2.4 Vendored libraries

- **`external/fathom/`**: MIT, with its notice (`external/fathom/LICENSE`,
  copyright de Man 2013–2018, basil00 2015, Dart 2016–2025). Ships in
  every binary. Fathom's MIT licence is unbroken from 2015 to its 2025
  LICENSE update.
- **`hybrid/`** is untracked here (build outputs only). The oracle's
  source is published under the tags `oracle/hybrid` (`01df815`) and
  `oracle/hybrid-diag` (`324ace4`): 53 files of Stockfish `9587eeeb`
  with its `Copying.txt`, plus Basilisk's bridge. It is GPL, kept with
  its notice, and never ships.
- **Disservin's `chess-library`** (MIT) was vendored with its notice and
  a `THIRD_PARTY_LICENSES.txt` from 2025-06-18 (`d2683d4`). It was
  removed when the custom board was written, before `v1.0.0`, which
  contains none of it. No release carried it. It remains in published
  history, with its notice.

### 2.5 Patches and quoted donor code in `docs/`, `analysis/`, `tools/`

| Item | Against | Licence of the context lines | Note |
|---|---|---|---|
| `tools/board_compare/reckless/reckless-board-adapter.patch` | Reckless `8d7d1dd` (2026-11-13) | **AGPL-3.0** | 33 context lines of Reckless source (its piece values among them) and 449 added lines that modify Reckless. On `master` since 2026-11-14 (`55e3010`), and in `v1.11.0`'s source archive. Unlabelled |
| `tools/board_compare/stockfish19/…`, `stockfish-classical/…` | Stockfish `edb0d9db`, `9587eeeb` | GPL-3.0-or-later | unlabelled, compatible |
| `tools/donor/sf16-*.patch`, `sf9587-*.patch` | Stockfish `sf_16`, `9587eeeb` | GPL-3.0-or-later | two patches swap one classical evaluation into the other: 592 and 521 context lines. Unlabelled, compatible |
| `tools/oracle/bridge-*.patch` | the oracle's Stockfish | GPL-3.0-or-later | unlabelled, compatible |
| `tools/board_compare/rarog/board_xv2.diff` | Rarog `611307e` | the maintainer's own, GPL-3.0-or-later | — |
| `docs/reference/rarog/refs/oracle-hybrid-*.patch` | Rarog's Stockfish oracle | GPL-3.0-or-later | labelled by the snapshot README |
| `docs/reference/triumviratus-networks/trainer/*.patch` | Stockfish's trainer, as Triumviratus patched it | GPL-3.0 (its `COPYING`) | inside a labelled snapshot |
| `tools/texel/seed_sf16.py` | reads `sf_16` `psqt.cpp` from a local checkout with `git show` | GPL-3.0-or-later | embeds no table |

- No tracked Markdown file outside `docs/reference/` has a fenced block
  of donor code, by a search for code-shaped blocks near any donor's
  name. Donor citations are prose with `file:line` references.
  About thirty tracked files outside the snapshots cite Reckless this
  way: fourteen analyses, PLAN, PROCESS, archived plans, five ledger
  records and three tools. They cite it as a timed reference (B.7.2's
  board, BAS-P32, BAS-X41) or as Rarog's donor in imported priors (§5).
  A citation is not a copy.
- The Rarog snapshot names Reckless in 20 files, all prose. It has no
  fenced Reckless code and no Reckless patch. Its three Rarog tools that
  name Reckless only take a Reckless binary's path as an argument.

### 2.6 Snapshots of other repositories, tracked verbatim

| Snapshot | Source | Licence | Content |
|---|---|---|---|
| `docs/reference/triumviratus/` | `Tors3/Triumviratus` `0c18907` (2026-11-03) | GPL-3.0; COPYING added 2026-05-29, so this commit is GPL | **the whole engine's source**: 93 files under `source/`, its build scripts and documents. Fathom and incbin are excluded; `COPYING` is kept |
| `docs/reference/triumviratus-networks/` | `5f3dc60` (2026-10-28) | GPL-3.0; COPYING from 2026-09-28 | recipes, documents and trainer patches; no networks |
| `docs/reference/rarog/`, `rarog-2026-10-06/` | Rarog `015bccae`, `dcf15c51` | the maintainer's own, GPL-3.0-or-later | documents, tools, tests; no `src/` |

Triumviratus states "GPLv3" with no "or later", so it is read as
GPL-3.0-only. Conveying it verbatim with its `COPYING` meets the
licence. It is not in the engine.

### 2.7 Networks, training data and opening books

| Item | Source | Licence | Status |
|---|---|---|---|
| `UHO_Lichess_4852_v1.epd`, the SPRT and SPSA book | `official-stockfish/books` | CC0-1.0, its LICENSE added 2025-05-10 | untracked (`tools/books/` has only `.gitkeep`); never ships |
| `SuperGM_4mvs.pgn`, used in BAS-E14 and BAS-E15 | the community book of that name | no licence found | untracked, used only to play; never ships |
| Datagen starts, `beast_seed*.epd` | the Beast pool, the maintainer's own engine `maelic13/beast` (GPL-3.0) | own | untracked |
| Texel labels, corpora A and B | Basilisk's own self-play, relabelled with Syzygy | own | untracked |
| Networks | none yet; Phase F trains on own data only; trainer `maelic13/net_trainer`, GPL-3.0, the maintainer's own | own | — |

### 2.8 Published history and authorship

- The repository is public at `github.com/maelic13/basilisk`. History
  holds, beyond §2.1 to §2.7:
  - the oracle tags (§2.4);
  - `chess-library` (§2.4);
  - the Reckless patch since 2026-11-14.
  It holds no network or book from anyone else.
- Every commit on every ref is the maintainer's: 1,511, under three
  addresses. Eleven early commits, from 2023-08-21 to 2023-09-07 (the
  first C++ port), carry a second, non-personal address. Only the
  maintainer can confirm that nothing claims those commits. Rarog's
  1,593 commits are the maintainer's too.

### 2.9 What the releases shipped

| Releases | Binary contents from others | Licence texts in the assets | Attribution |
|---|---|---|---|
| 1.0.0 to 1.9.1 (23) | Fathom; PeSTO tables; Stockfish-derived items as the code had them | none: bare executables | README named PeSTO, and Fathom from 1.3.0 |
| 1.9.2 to 1.10.2 (5) | the same | none | README named Fathom and Stockfish; **PeSTO named nowhere**, nor in the release notes |
| 1.11.0 | the same; §2.1 to §2.3 | `LICENSE.txt` (GPL) and `LICENSE-Fathom.txt` in every archive | README *Acknowledgements* by category; the source archive carries the Reckless patch |

Each release's source is GitHub's tag archive beside the binaries, which
meets GPL §6(d). The binaries' own duty, the licence texts, was unmet
until 1.11.0.

## 3. Rules by licence class

What may cross into Basilisk, which is GPL-3.0-or-later. "Expression" is
code, transcription, comments, names, structure, tables and data layouts.
Ideas, algorithms and facts are not protected under any of these
licences. Whether a tuned constant is expression or a functional fact is
unsettled, so the conservative column treats a donor's constant as
material.

| Class | Here | May take | May not take | Obliges |
|---|---|---|---|---|
| GPL-3.0-or-later | Stockfish, Rarog, net_trainer, Beast | everything, under GPL-3.0-or-later | — | keep the donor's copyright notices on what is copied, and mark modified files (§4, §5a). Ship the GPL text and offer source with every binary (§6). Basilisk may not be relicensed away from GPL while that material stays |
| GPL-3.0-only | Triumviratus | everything, but the combined work is then GPL-3.0 only: the "or later" option is lost for it | — | as above. Today only the verbatim snapshot, which is not in the engine |
| GPL-2.0-or-later | none here | everything, taken under 3 | — | as GPL-3.0 |
| GPL-2.0-only | none here | ideas only | code, tables, constants: GPL-2.0-only and GPL-3.0 cannot be combined | — |
| AGPL-3.0 | Reckless from 2026-01-26; any AGPL engine | ideas only, by Rarog's rule (§6). The law would allow more: GPL-3.0 §13 lets the two be combined, the AGPL parts stay AGPL, and AGPL §13's network clause then covers the whole engine | code, transcription, formulas, tables, layouts, index constructions, constants (even as seeds), patches, quoted code | as AGPL if any part is taken: label each part with Reckless's notice, and the whole must offer its source to network users. Running its binary obliges nothing |
| No licence | Reckless from 2025-02-22 to 2026-01-26; any unlicensed engine | ideas only | anything else: all rights are reserved | — |
| MIT, BSD | Fathom, chess-library, Reckless before 2025-02-22 | everything | — | the copyright and permission notice in every copy, binaries included. BSD-3 adds no endorsement |
| CC BY-SA 3.0 | PeSTO, through the Chess Programming Wiki | the values, with attribution, if they are protected at all | combining them into GPL code as an adaptation: BY-SA 3.0 is not GPL-compatible (only BY-SA 4.0 is, one way, to GPLv3) | attribution: author, source, licence link, and a note of changes. Share-alike if the tables are an adaptation |
| CC BY-SA 4.0 | none here | as BY-SA 3.0, and it may become GPLv3 | — | attribution; the result may be GPLv3 |
| CC0, public domain | the Stockfish books, the Lichess database, splitmix64 | everything | — | nothing |
| ODbL | Leela data, which Triumviratus trains on | not used: Phase F trains on own data only | — | a network trained on it is a produced work and needs a notice; the database share-alike reaches derived databases, not the engine's code |
| Facts | WAC, perft and test positions; chess rules and endgame theory | everything | — | nothing; credit is courtesy |

## 4. Implications for Basilisk now

1. **The one incompatibility: the Reckless patch.**
   `tools/board_compare/reckless/reckless-board-adapter.patch` is AGPL
   material in a GPL repository, unlabelled. It is on `master` and in
   1.11.0's source archive. It is the same defect as Rarog's oracle
   patches (Rarog, `analysis/coda_read_2026-11-23.md`). It is small and
   tooling-only: nothing of Reckless is in the engine, and the binaries
   are clean. The fix is forward: move the patch out of the tree, and
   have `build.py` read it from untracked storage. History keeps the old
   copy (§7).
2. **Missing notices in 28 releases.**
   - 1.0.0 to 1.10.2 shipped Fathom's code without its MIT notice and
     without the GPL text. A strict reading puts every one of them out of
     compliance with Fathom's licence.
   - 1.9.2 to 1.10.2 also shipped PeSTO's tables with no attribution
     anywhere.
   - The fix: attach both licence files to each old release, and add one
     line to the notes of 1.9.2 to 1.10.2 naming PeSTO. These are the
     maintainer's outward-facing commands (§8, step 3).
3. **PeSTO's share-alike question** stands while the tables are
   PeSTO's. It ends when C.2.8's refit lands (PLAN C.2.5). If tuned
   values are protected at all, CC BY-SA 3.0 cannot be relicensed into
   GPL code. The README's attribution meets the BY part, not the SA part.
4. **Labelling gaps, compatible material.** The Stockfish patches in
   `tools/donor/`, `tools/oracle/` and `tools/board_compare/` carry no
   statement of their licence. The three routines in §2.1 carry no
   Stockfish copyright line. GPL §4 and §5 ask for both if the routines
   are protected expression; a one-natural-form routine probably is
   not. A `NOTICES` file fixes this in one place.
5. **Pending flows that would change the answer.** Basilisk's PLAN holds
   Reckless-form priors from Rarog (§5). If any of them is ever
   implemented by copying Reckless rather than taking the idea, it
   creates a second incompatibility. PROCESS has no rule that stops
   this today: its boundary names Reckless only as Rarog's donor.
6. **The disclosure is accurate.** README *Acknowledgements* and PROCESS
   *The independence boundary* match §2.1 to §2.3. Three points are
   missing: Fathom's notice is not in the old releases (item 2), the
   compiled-in WAC positions, and that the tracked tree carries
   Triumviratus's whole source.

## 5. Material between Basilisk and Rarog

Both engines are the maintainer's own and GPL-3.0-or-later. The
maintainer's own original code moves freely in either direction. What
carries a licence question is third-party material riding along with it.

**Rarog to Basilisk**

- **Snapshots** (§2.6). They are Rarog's own work and carry no Reckless
  code. They hold the Stockfish oracle patches (GPL).
- **Ported tools**, among them `originality_scan.py`, and **fixtures**
  (the bench FENs, the WAC EPD byte for byte, the endgame suites). They
  are own material or facts.
- **Reckless-form priors in PLAN**, imported 2026-11-21
  (`analysis/rarog_import_2026-11-21.md`):
  - D.1's prior from Rarog's adoption of Reckless's time manager;
  - D.2's shared correction tables "in Reckless's form";
  - TP-4's helper LMR bias "from Reckless", with Reckless-measured
    numbers.
  These are ideas and measured priors, and may stay as such. Their
  forms must come from Stockfish or from Basilisk's own derivation, and
  no Reckless constant may cross. Stockfish has a form for each: its
  time manager, which D.1 already takes, its thread voting and its
  correction histories. Whether Stockfish shares correction histories
  between threads at its current revision is D.2's to read.
- **What must not flow:** Rarog's compact kernel shape, its clock (the
  Reckless clock with its constants), the constants still at Reckless's
  values, and the two transcribed places. Rarog's own D.11 resolves
  these there. Until it does, any Rarog code from after 2026-11-13 is
  suspect for Basilisk. The scan against Rarog 9bf4f2b found none of it
  in Basilisk's `src/`.

**Basilisk to Rarog**

- Test anchors, fixtures and the splitmix64 key generation (Rarog's
  `src/board/zobrist.rs`, `tests/behaviour_anchors.rs`), and release
  packaging (D.15.1). All own material.
- **Stockfish-derived items Rarog took from Basilisk**, such as the
  endgame rules or the time-manager constants, stay GPL-3.0-or-later:
  compatible, but they count in Rarog's own inventory.
- **PeSTO is a shared question.** Rarog seeded its tables from PeSTO
  too, and a free refit moved them by 11 to 57 cp an entry
  (`analysis/rarog_audit_2026-10-24.md`, item C-66). Whether any entry
  is still PeSTO's is Rarog's D.11.1, and Basilisk's C.2.8 settles it
  here.
- **net_trainer** is shared and GPL-3.0. If Rarog's Phase F ever brings
  Reckless-derived runtime or trainer code into it, Basilisk's networks
  inherit the question. Rarog's 2026-11-23 rule (Reckless ideas only)
  closes this for now.

## 6. The rule Basilisk lacks

Rarog's PROCESS now has a rule for AGPL and unlicensed engines (Rarog
`docs/PROCESS.md`, *The independence boundary*, the Reckless paragraph).
Basilisk's has nothing equivalent, because Reckless was never its donor.
The rule to adopt, in Basilisk's PROCESS, for any AGPL or unlicensed
engine:

- ideas may be read;
- no code, transcription, formula, table, data layout, index
  construction or constant crosses, not even as a seed;
- no patch of its source and no quotation of its code in the tree;
- its released binaries may be played, timed and profiled.

Rarog's other edges stay as they are here: Stockfish material may cross
as seeds, and transcription is allowed for one-natural-form routines.
The target in §7 would tighten those too; that is the maintainer's
choice.

## 7. Reaching an original engine that owes nothing

The target: no third-party material in the engine, and so no
disclosure obligation and no licence obligation to anyone. As sole
author, the maintainer could then license Basilisk on any terms, and
move code between their engines with no licence question. Item by item:

| Item | Route | Reason or derivation | Cost |
|---|---|---|---|
| Reckless patch in `tools/` | **remove** from the tree; keep it in untracked storage | AGPL | minutes |
| PeSTO tables (604 of 736 entries) | **reimplement**: C.2.8 fits from a start that is not a donor's | BAS-O12: the identified tables reach the same shapes from different starts, and the start's effect is below resolution (2.1e-5 model, 1e-7 realized). The mg king (36.5 cp) and eg pawn (8.2 cp) keep some seed, so those two need a start from Basilisk's own quantities: its own data's means, or a king-safety shape from its own eval | none extra if C.2.8's registered start changes before it runs; otherwise one more fit |
| `sf_16` PST seed (C.2.5's choice) | **replace** with the neutral start above, or **keep** as GPL material that is fitted and disclosed | same evidence | as above |
| Pawn row from `9587eeeb` | goes with the refit | — | none |
| Time manager formula and constants | **reimplement**: the formula's shape is an idea; fit the constants on Basilisk's clock from a start that is not Stockfish's | Coda refitted Viridithas's: 40% came back bit-identical, which reads as functional optima. D.1 already builds and gates the clock | D.1's SPSA, plus its SPRTs, from a different start |
| LMR history divisors, consistency multipliers | **reimplement**: derive them from Basilisk's own history statistics, or fit them from flat starts | own quantities | one overnight SPSA and one SPRT for the cluster |
| Piece values in the histories | **reimplement**: Basilisk's own material values, scaled to the history limits | own quantities | in the same cluster |
| Search literals left at their seed | **refit** from a non-donor start. A literal that the fit rounds back to the same value may then **stay as a functional fact**, with that result written beside it | B.6.7's method | in the same cluster, or B.6.7's next joint fit |
| Endgame scaling rules | **keep** the rules: they are chess facts (Philidor, the rook-pawn draws). **Refit** their constants | the rules are endgame theory, not expression | Phase C, already refitting the evaluation |
| Phase C's `sf_16` families and seeds | families are ideas; **seed** them from neutral starts | BAS-O12's evidence | possibly slower convergence; under-identified terms keep their seed, so they need own priors |
| Three routines | **rewrite** from primary sources: Microsoft's documented privilege API sequence, the cuckoo method as published (van Kervinck), `__DATE__`'s format from the C++ standard | one natural form each; rewriting removes the question cheaply | an hour each, bench-exact (`4,777,412` unchanged) |
| Fathom | **keep** as a library, with its notice | a Syzygy prober of one's own is weeks of `I2` work, against a format whose reference code is itself de Man's | the notice stays forever |
| WAC, perft and test positions; castling mask; splitmix64 | **keep** as functional facts | facts and public domain | none |
| Search design (Stockfish 19's) | **keep** as ideas | not protected expression; no licence obligation | — |
| Stockfish patches, Triumviratus snapshot, oracle tags | **keep**, labelled in `NOTICES`; or move the study material to a private repository, as Coda did | GPL-compatible, and not in the engine | labelling: an hour. Moving them breaks the snapshot manifests and citations |

**What cannot be reached, and why:**

- **The published history.** It keeps the Reckless patch from
  2026-11-14, `chess-library` from 2025-06 to 2026-05, and the oracle's
  Stockfish source, each under its own licence. Rewriting a public
  history breaks every citation and every clone. Coda did not rewrite
  its history, and said so; Basilisk should do the same.
- **Earlier releases stay as published.** 1.0.0 to 1.11.0 carry PeSTO's
  tables and Stockfish's constants. Their notices can be added (§8), but
  their contents cannot change.
- **Fathom's notice**, unless Basilisk writes its own prober. That is not
  worth it, and a linked library's notice is not an originality question.
- **The design debt is not a licence debt.** Basilisk's search is
  Stockfish 19's design by PLAN rule 1, and its histories keep the
  donor's limits by design. That obliges nothing legally. But a
  registration form or a rating list asks about origins, not licences,
  so README's credit stays a courtesy for as long as the design is
  Stockfish's. "Needs no disclosure" is reachable in law; in the
  community's sense it is reachable only by a design of Basilisk's own,
  which no roadmap leaf proposes.

## 8. Decision

**A new step is needed.** The maintainer places it in PLAN. Suggested
title: *Licence boundary, notices and the donor-constant exit*.

1. **Now, `M`, about an hour:**
   - move the Reckless patch out of the tree, and point `build.py` at
     untracked storage;
   - add the AGPL and no-licence rule of §6 to PROCESS;
   - mark PLAN's Reckless-form priors (D.1, D.2, TP-4) as ideas only,
     with forms from Stockfish or own derivation;
   - declare the foreign commits this note names in
     `tools/diag/citation_exceptions.tsv`. This note writes Reckless's
     licence commits by date and pull request, because editing that
     file was out of scope.
2. **`M` with a small `I1` part, an hour or two:** a `NOTICES` file
   listing §2's third-party material by class:
   - the Stockfish-derived items in the engine;
   - Fathom;
   - PeSTO, until C.2.8 lands;
   - the compiled-in WAC positions;
   - the tracked Stockfish patches and the Triumviratus snapshot, with
     their licences.
   `release.yml` then copies `NOTICES` into each archive beside the two
   licence files, and README *License* points to it.
3. **The maintainer's commands, minutes:** attach the two licence files
   to releases 1.0.0 to 1.10.2, and add the PeSTO line to the notes of
   1.9.2 to 1.10.2. They are outward-facing, so the step prepares the
   files and the command list and the maintainer runs it.
4. **Before C.2.8 registers, a decision, `R2`:** does C.2.8 start from
   `sf_16` (GPL, disclosed) or from a neutral start, with own-quantity
   starts for the mg king and eg pawn tables? BAS-O12 says the choice
   costs nothing measurable for the identified tables. This is the
   cheapest point at which to take the target, because the fit runs
   anyway.
5. **Folded into leaves that already fit the same constants, `I1` plus
   `V`:**
   - the time-manager constants in D.1;
   - the history tables and the leftover search seeds in one overnight
     SPSA cluster and its SPRT;
   - the three routines, rewritten bench-exact;
   - Phase C's families seeded from neutral starts.
   No games beyond those leaves' own, except the history-table cluster:
   one overnight SPSA and one STC SPRT.

The order is 1, 2 and 3 now (they fix the incompatibility and the past
notices), then 4 at C.2.8, then 5 as each owning leaf comes up. Class:
`M` for steps 1 to 3, `R2` for step 4, `I1` and `V` for step 5. Steps 1
to 3 do not depend on the target; steps 4 and 5 are needed only if the
maintainer adopts it.
