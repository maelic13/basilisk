# BAS-E18

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Differential-harness observations -->

**BAS-E18 — 5.9.11 results: the labels were the defect, and the scalar surface
is saturated** (2026-08-27, arm B still running). Conditions per BAS-E17: same
starts, same extraction, same 348-parameter `scalars` fit, `3+0.03`, 1T, Hash 64
both arms, UHO. Baseline `5911-base` = current head, bench 11,941,440.

| arm | labels | Elo | games | outcome |
|---|---|---:|---:|---|
| **A** | Basilisk self-play @ 8k | **−2.85 ±3.11** | 20,096 | H0 accepted — **rejected** |
| **C** | Basilisk self-play @ 25k | **+1.00 ±2.11** | 44,800 | **no verdict** — stopped by decision |
| **B** | Stockfish self-play @ 8k | **−7.30 ±4.76** | 9,852 | H0 accepted — **rejected, worst arm** |
| *5.9.6* | *Stockfish **evaluations*** | *−77.92 ±15.32* | *1,292* | *H0 — rejected* |

**The diagnosis is confirmed.** Arm A differs from 5.9.6 in exactly one thing —
the labels are game results instead of Stockfish evaluations — and the same fit
of the same 348 parameters moves from **−77.92 to −2.85**. Changing only the
label source recovered roughly **75 Elo**. The BAS-E11 correction stands: the
distillation was the primary cause, not the quiet filter.

**And the scalar surface is saturated.** Neither Basilisk-labelled arm gains
anything. That is now four independent results agreeing: cycle 6 washed at
+1.37 ±5.21; BAS-E08's ablation found the added structure inert; arm A −2.85;
arm C +1.00. Clean labels stop the fit doing harm; they do not extract anything
further from those 348 coefficients. Arm C beating arm A by ~3.9 Elo is the
predicted direction for stronger labels, but far too small to matter.

**A stronger oracle's outcomes made the evaluation WORSE.** Arm B is the worst
of the three and the fastest to reject — 9,852 games in under two hours, against
four to eight hours for the Basilisk-labelled arms. Same starts, same
extraction, same fit, same row count; only the engine that played the games
differs.

*This settles the oracle question, and it settles it against the oracle.* The
argument for Stockfish labels was that stronger play makes an outcome a
less-noisy estimate of a position's true value. Arm B's wider W/L skew (34.9% /
24.2% against arm A's 33.4% / 27.7%) confirms the labels really were cleaner in
that sense — and it still lost 4.5 Elo to arm A. The counter-argument is what
held: **a label should reflect what OUR search can realize.** A position
Stockfish converts and Basilisk cannot is, for our purposes, not that won, and
fitting to it teaches the evaluation to value what the search cannot cash.
Evaluation belongs to the search that consumes it.

*Convenient corollary:* PLAN's independence constraint and the strongest
measured option agree here, so no trade-off had to be made.

*Registered prediction, scored honestly.* BAS-E17 predicted **all three beat the
head, ordering C > A > B**. The **ordering was exactly right** — C (+1.00) > A
(−2.85) > B (−7.30). The **"all three beat the head" half was wrong**: none did.
Recorded as a half-hit; the ordering intuition was sound, the level was not.

**Arm C carries a protocol deviation and must be reported as an estimate.** Its
SPRT never reached a boundary — LLR sat at **+0.03**, dead centre of
(−2.94, 2.94) — and it was halted at 44,800 games by maintainer decision so that
arm B could run. Never quote it as a pass or a fail. The ±2.11 interval is
tighter than most accepted verdicts in this project; what is missing is a
stopping-rule decision, not precision.

*Why it could not simply be left to finish.* `sprt.ps1` honours `-Games` only in
`fixed`/`calibrate` modes — line 476, `$rounds = if ($fixedSize) {...} else
{ 50000 }`. A **gainer**-mode run silently discards the cap and stops only on an
LLR boundary or the 50,000-round (100,000-game) hard stop. The run was launched
with `-Games 16000` believing each arm was capped near 3 hours; arm A resolved
in 3h45m only because it was decisive, and arm C was ~10 hours from stopping
with no prospect of deciding.

**Decision-rule outcome, applied as registered.** BAS-E17 rule 3: *"If NO arm
passes, the label-source hypothesis is not supported, and the next suspect is
the extraction contract or the fit itself — not another corpus."* No arm passed,
so **no further corpus work is warranted**, and the winning label source for all
downstream work is **our own self-play**, at the highest node count affordable
(arm C's 25k beat arm A's 8k by ~3.9 Elo).

The hypothesis needs splitting to be stated accurately, though. Labels were
decisively the cause of the *−77.92 damage* — arm A proves that at 75 Elo of
recovery. Labels are **not** a source of *gain*, because the surface they feed
is saturated. Both halves are true and they are not in conflict.

*Lessons.*

1. **A parameter that is accepted and ignored is a defect.** `sprt.ps1` now
   throws when `-Games` is passed in a mode that cannot honour it, and the
   runner no longer passes it.
2. **A neutral SPRT does not stop.** Budget one long arm per night, not three.
   Decisiveness, not the cap, is what makes an arm cheap.
3. **A stronger labelling engine is not a better labelling engine.** Measured,
   not argued: −7.30 against −2.85 for our own engine at the same node count.
   Do not revisit an external oracle for evaluation labels without new evidence.
4. **Stopping a run early is legitimate when it cannot decide, and must be
   recorded as such.** The reason to stop was that the informative arm was
   waiting behind an uninformative one — not that the numbers looked settled.
