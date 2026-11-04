# Tools

| script | what it does |
|---|---|
| `fetch_net.ps1` | downloads the `.nnue` exports of a running training from the training machine; with `-Last` it first serializes the live checkpoint there, on CPU, without touching the GPUs. `.\fetch_net.ps1 -VmHost <host> -Port <port> [-Key <ssh key>] [-Last]` |
| `match_moe.ps1` | plays a checkpoint against `legio-septima` with fastchess: two builds of the same engine source, differing only in the input layer, so the match isolates the network. Checks both benches first |

Measurement conventions used throughout: 1 thread, 64 MB hash, UHO 2024 openings (+0.85/+0.94), draw adjudication at
move 40 after 8 moves under 10 cp, two-sided resign after 3 moves over 600 cp, and **20+0.2 or longer** for any
conclusion about a network (see recipe 02 for why).
