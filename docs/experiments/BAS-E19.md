# BAS-E19

<!-- part 1 of 1: from docs/EXPERIMENTS.md, Differential-harness observations -->

**BAS-E19 — 5.9.15 LTC probe: no depth story** (2026-08-27). `5911-armC`
against `5911-base` at **`10+0.1`**, `-Mode fixed` 6,000 games, 1T, Hash 64
both arms, UHO. A fixed-N estimate by design — the harness makes no accept or
reject decision.

| | TC | Elo | 95% CI |
|---|---|---:|---|
| STC | `3+0.03` | +1.00 ±2.11 | 44,800 games |
| **LTC** | **`10+0.1`** | **+0.29 ±5.46** | 6,000 games |

Both time controls agree: **neutral.** The hypothesis that arm C is genuinely
better but hidden by shallow search is **not supported**. The probe was sized to
detect a +10–15 Elo depth effect and found nothing remotely that large; the LTC
point estimate is if anything *lower* than the STC one.

*Do not extend this run, despite the harness suggesting it.* Doubling to 12,000
games narrows the interval to roughly ±3.9, which still cannot resolve +1 — and
we already hold a **tighter** measurement of the same candidate at STC (±2.11).
More LTC games would buy precision on a null result we have already established
more cheaply elsewhere.

*The harness's canned classification does not apply here and was not followed.*
The `fixed` mode prints an 8.6.8A accept-audit reading — "A = removal side; a
NEGATIVE estimate means removing the feature lost", straddle ⇒ "default a single
to KEEP". That instrument answers a **removal** question, where the candidate is
the *simplification* and keeping the feature is the conservative default. Arm C
is an **addition**. For an addition, a straddle around zero is not a reason to
keep; it is the absence of a reason to ship. Read the number, ignore the verdict
line.

*Conditional lesson.* BAS-M07 warns that fast-TC gains compress at longer TC.
The converse — that a neutral STC result hides an LTC gain — is a real
possibility and was worth 3h50m to check, but it did **not** hold here. An
evaluation change that is neutral at STC should not be assumed to be waiting for
depth. Retry trigger: a candidate whose *mechanism* is specifically depth-gated
(deep endgame recognisers, tablebase-like knowledge), not a general refit.
