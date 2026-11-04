# Time management in 8.1

Part of the 8.1 plan in [`DEVELOPMENT_8.1.md`](../../../DEVELOPMENT_8.1.md). Time management runs in parallel with the
four search blocks: it does not change the search tree, so its tests can run whenever the test machine is free.

## Open measurements carried over from 8.0

- Time per expert of the network: more or less time in the phases where the network is less sure.
- The graded surprise rule: the time bonus after a change of best move, scaled by how large the change is.
- The two defence rules, for positions where the engine is worse.

## Planned

- **An instant move with a single legal reply**, and less time when the legal moves are few (a known idea; the root
  move count is already exact).
- The use of time at the CCRL controls (40/15 with increment), from the study of lost games in step 2 of the plan.

Status: planned; each rule is measured at the time control where it acts.
