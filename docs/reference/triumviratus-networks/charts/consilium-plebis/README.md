# Consilium Plebis: charts of the training run

Consilium Plebis is the network of Triumviratus 8.1, trained from 21 to 28/10/2026: a pretraining P in epochs of one
billion positions (stopped by the author after epoch 496 of the 550 planned), a finetune F of 60 epochs and a
continuation of 3 epochs down to a zero learning rate, whose weight average is the final network. Its architecture is
described in the engine repository
([`docs/networks/NETWORKS.md`](https://github.com/Tors3/Triumviratus/blob/main/docs/networks/NETWORKS.md)); the file
is attached to the release [`consilium-plebis`](https://github.com/Tors3/Triumviratus-Networks/releases/tag/consilium-plebis)
and the recipe is [`recipes/07_consilium-plebis/`](../../recipes/07_consilium-plebis/). Final result: +10.3 +- 4.3
Elo against Consilium, same search on both sides, 30k nodes per move, 8,332 games.

## Training loss

| chart | what it shows |
|---|---|
| [`c2_pf_overall.png`](c2_pf_overall.png) | the whole run, P, F and the continuation on one epoch axis: loss on a log scale, the zoom from epoch 350 on a linear scale and the learning rate |
| [`c2_pf_zoom.png`](c2_pf_zoom.png) | the end of F and its continuation, with the final losses of P and F as references |
| [`c2_pf_loss_vs_lr.png`](c2_pf_loss_vs_lr.png) | the loss against the learning rate for P and F: at the same learning rate F is below P, and over the flat stretch at 15% the loss stays nearly flat |
| [`c2_loss_per_epoch.png`](c2_loss_per_epoch.png) | training loss per epoch of P, log scale, with the end of the warmup and the point where lambda stops at 0.75 |
| [`c2_loss_lr_lambda.png`](c2_loss_lr_lambda.png) | the same loss from epoch 25 on a linear scale, with the learning rate (% of its peak) and lambda on the same epoch axis |
| [`c2_loss_vs_lr.png`](c2_loss_vs_lr.png) | the loss against the learning rate after the warmup |
| [`c1_c2_training.png`](c1_c2_training.png) | the pretraining of Consilium and of Consilium Plebis on the same epoch axis, with lambda and learning rate of each: different lambdas make the two training losses not comparable in absolute value |

## Loss on held-out positions

`t91` is a sample of Leela Chess Zero test91 games that neither network trained on. Every network reads the same
positions, so the differences to Consilium are paired.

| chart | what it shows |
|---|---|
| [`c1_c2_holdout.png`](c1_c2_holdout.png) | loss of the exports relative to Consilium (the 8.0 release network) at lambda 1.0 (evaluation), 0.75 (training target) and 0 (game result): Consilium Plebis passed Consilium near epoch 240 and is 3% below at epoch 435 |
| [`c2_t91_by_bucket.png`](c2_t91_by_bucket.png) | epoch 435 against Consilium by number of pieces: far ahead in the endgames, less in the middlegame |
| [`c2_t91_by_expert.png`](c2_t91_by_expert.png) | epochs 376 and 435 against Consilium by expert (the opponent's non-pawn material): every expert still improves at low learning rate, the middlegame experts most |

## Evaluation scale

| chart | what it shows |
|---|---|
| [`scale_trend.png`](scale_trend.png) | spread of the search evaluation of 41 exports against Consilium's on 49,594 positions, and the side-to-move offset |

## Strength

| chart | what it shows |
|---|---|
| [`consilium2_elo_trajectory.png`](consilium2_elo_trajectory.png) | Elo of the exports against the final Consilium at fixed nodes, same search |
| [`consilium_compare_trajectory.png`](consilium_compare_trajectory.png) | Consilium and Consilium Plebis across training, each against the network it replaces |
| [`consilium_transpose_fit.png`](consilium_transpose_fit.png) | the Consilium Plebis points matched to a fit of Consilium's curve |

Every chart is also in SVG.
