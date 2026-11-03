# BAS-X38

| Field | Value |
|---|---|
| ID | BAS-X38 |
| Triumviratus evidence | **Measurement and tuning failure modes it found in itself** (`analysis/triumviratus_review_2026-11-03.md` §4.7, TM-1 to TM-8). Two engines on one physical core cost about 1.6 plies at 5 s, and SPSA vectors tuned with hyperthreading gained only on the slower socket; a NUMA node without memory and 75 concurrent games hid a network's gain (+0.8 ± 10.1 against +23.4 ± 12.0 at 38). Its SPSA tool decayed the perturbation from the value meant as the final one, so every 8.0 run explored with a half to two fifths of the intended step. Of thirteen tests, seven peaked at +5 to +17 and ended between −2 and +2; five "leans" together read −0.1 ± 6.7; a bundle of four single +1 to +2 estimates read +0.51 ± 6.49. A lever enters a bake only at a net move of 2.5 times the random-walk expectation, since random-walk segments look monotone 30% of the time. Removing quiet checks from quiescence flipped sign once a one-move cap was lifted. Speed was read by hardware counters at an identical tree with the engine pinned and only search threads counted, after the start-up had inflated every figure |
| Basilisk implication | Most are already Basilisk rules: physical-core pinning (PLAN rule 7), registered stop rules, the journal z-read (B-101). Two are checks: that Basilisk's SPSA schedule reaches its final perturbation at the last iteration (the review's §6, open check 1), and a counter-based speed instrument beside A.9.4's pooled read (B.7.2). The regime lesson is cited in B.4 and C.0 |
| PLAN coverage | B.4, B.7.2, C.0, F.6 |
| Disposition | import |
