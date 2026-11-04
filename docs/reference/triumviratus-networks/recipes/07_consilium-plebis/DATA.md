# Consilium Plebis: training data of the P run

Every `.binpack` in the VM's `/data/bt4` (43 files, 701.3 GB = 653 GiB), listed on 26/10/2026 and matched by name to
its Hugging Face dataset. Downloaded by [`training/download_bt4.sh`](training/download_bt4.sh) (the BT4-relabelled
corpus, `.q.` variants and duplicated months left out) and [`training/download_c2.sh`](training/download_c2.sh)
(`tb5dtm.binpack`); [`training/download_missing.sh`](training/download_missing.sh) completes files missing or of the
wrong size against the sizes on Hugging Face.

- **Content.** Leela self-play positions of the test60, test77-test80 runs and of older Leela data (T60/T70
  "wrong is right", leela96), Stockfish self-play and DFRC (`master-binpacks`), all **relabelled with Leela's BT4
  network** (`relabel-BT4-tf13tune`; the two 2024 test80 months are the `xushawn` relabelling with the same network),
  plus 5-man DTM mates (`tb5dtm`). The same corpus trained Consilium.
- **Not used:** `jshriver/t91-binpacks` and `t90-binpacks` (Leela's test91/test90 debug runs of BT4 in RL mode, 800
  and 400 visits per move, labels of the run's own networks, never measured against the BT4 relabelling). A sample of
  t91 (`t91_sample.binpack`) is the holdout of [`docs/C2_HOLDOUT_T91_2026-10-26.md`](docs/C2_HOLDOUT_T91_2026-10-26.md).
- **How it is drawn.** `train_c2.sh` passes every file to the loader, which draws files in proportion to their size,
  so every position weighs the same. Filters and sampling of the run: random skipping 2, soft early skipping to ply 20
  (kept fraction 0.25 / 0.40 / 0.60 / 0.90 at ply 0 / 6 / 10 / 18), piece-count spline `pc-y` 0.0 / 0.60 / 1.0 / 0.95 /
  0.75, quiet-position filters on ([`training/train_c2.sh`](training/train_c2.sh)). One epoch = 1 G positions; P = 550
  epochs.

## By source

| source (Hugging Face dataset) | files | GB | share |
|---|---|---|---|
| [`vondele/master-binpacks_relabel`](https://huggingface.co/datasets/vondele/master-binpacks_relabel) | 5 | 129.3 | 18.4% |
| [`vondele/linrock_relabel_1`](https://huggingface.co/datasets/vondele/linrock_relabel_1) | 13 | 204.0 | 29.1% |
| [`vondele/linrock_relabel_2`](https://huggingface.co/datasets/vondele/linrock_relabel_2) | 12 | 141.8 | 20.2% |
| [`xushawn/test80-bt4-relabel`](https://huggingface.co/datasets/xushawn/test80-bt4-relabel) | 2 | 17.7 | 2.5% |
| [`vondele/from_kaggle_1_relabel`](https://huggingface.co/datasets/vondele/from_kaggle_1_relabel) | 5 | 97.7 | 13.9% |
| [`vondele/from_kaggle_2_relabel`](https://huggingface.co/datasets/vondele/from_kaggle_2_relabel) | 5 | 110.7 | 15.8% |
| [`vondele/rescored`](https://huggingface.co/datasets/vondele/rescored) | 1 | 0.1 | 0.0% |
| **total** | **43** | **701.3** | 100% |

## Every file

| file | source | bytes | share |
|---|---|---|---|
| `dfrc_n5000.relabel-BT4-tf13tune.binpack` | `vondele/master-binpacks_relabel` | 40,993,832,254 | 5.85% |
| `fishpack32.relabel-BT4-tf13tune.binpack` | `vondele/master-binpacks_relabel` | 5,859,076,763 | 0.84% |
| `multinet_pv-2_diff-100_nodes-5000.relabel-BT4-tf13tune.binpack` | `vondele/master-binpacks_relabel` | 30,487,704,890 | 4.35% |
| `nodes5000pv2_UHO.relabel-BT4-tf13tune.binpack` | `vondele/master-binpacks_relabel` | 44,150,410,791 | 6.30% |
| `wrongIsRight_nodes5000pv2.relabel-BT4-tf13tune.binpack` | `vondele/master-binpacks_relabel` | 7,800,748,146 | 1.11% |
| `test60-2021-11-nov-12tb7p.min-v2.relabel-BT4-tf13tune.binpack` | `vondele/linrock_relabel_1` | 3,424,624,984 | 0.49% |
| `test60-2021-12-dec-12tb7p.min-v2.relabel-BT4-tf13tune.binpack` | `vondele/linrock_relabel_1` | 3,214,349,890 | 0.46% |
| `test77-2021-12-dec-16tb7p.v6-dd.min.relabel-BT4-tf13tune.binpack` | `vondele/linrock_relabel_1` | 18,385,327,867 | 2.62% |
| `test78-2022-01-to-05-jantomay-16tb7p.v6-dd.min.relabel-BT4-tf13tune.binpack` | `vondele/linrock_relabel_1` | 21,492,671,398 | 3.06% |
| `test78-2022-06-to-09-juntosep-16tb7p.v6-dd.min.relabel-BT4-tf13tune.binpack` | `vondele/linrock_relabel_1` | 10,632,973,051 | 1.52% |
| `test79-2022-04-apr-16tb7p.v6-dd.min.relabel-BT4-tf13tune.binpack` | `vondele/linrock_relabel_1` | 8,475,816,547 | 1.21% |
| `test79-2022-05-may-16tb7p.v6-dd.min.relabel-BT4-tf13tune.binpack` | `vondele/linrock_relabel_1` | 6,518,121,179 | 0.93% |
| `test80-2022-06-jun-16tb7p.v6-dd.min.relabel-BT4-tf13tune.binpack` | `vondele/linrock_relabel_1` | 12,873,163,930 | 1.84% |
| `test80-2022-07-jul-16tb7p.v6-dd.min.relabel-BT4-tf13tune.binpack` | `vondele/linrock_relabel_1` | 13,728,281,953 | 1.96% |
| `test80-2022-08-aug-16tb7p.v6-dd.min.relabel-BT4-tf13tune.binpack` | `vondele/linrock_relabel_1` | 10,942,344,039 | 1.56% |
| `test80-2022-09-sep-16tb7p.v6-dd.min.relabel-BT4-tf13tune.binpack` | `vondele/linrock_relabel_1` | 11,939,821,863 | 1.70% |
| `test80-2022-10-oct-16tb7p.v6-dd.relabel-BT4-tf13tune.binpack` | `vondele/linrock_relabel_1` | 69,105,297,401 | 9.85% |
| `test80-2022-11-nov-16tb7p.v6-dd.min.relabel-BT4-tf13tune.binpack` | `vondele/linrock_relabel_1` | 13,280,433,826 | 1.89% |
| `test80-2023-01-jan-16tb7p.v6-sk20.min.relabel-BT4-tf13tune.binpack` | `vondele/linrock_relabel_2` | 12,698,871,197 | 1.81% |
| `test80-2023-02-feb-16tb7p.v6-dd.min.relabel-BT4-tf13tune.binpack` | `vondele/linrock_relabel_2` | 10,359,292,788 | 1.48% |
| `test80-2023-03-mar-2tb7p.v6-sk16.min.relabel-BT4-tf13tune.binpack` | `vondele/linrock_relabel_2` | 14,732,039,950 | 2.10% |
| `test80-2023-04-apr-2tb7p.v6-sk16.min.relabel-BT4-tf13tune.binpack` | `vondele/linrock_relabel_2` | 15,079,776,962 | 2.15% |
| `test80-2023-05-may-2tb7p.v6.min.relabel-BT4-tf13tune.binpack` | `vondele/linrock_relabel_2` | 15,274,946,491 | 2.18% |
| `test80-2023-06-jun-2tb7p.min-v2.v6.relabel-BT4-tf13tune.binpack` | `vondele/linrock_relabel_2` | 17,983,836,266 | 2.56% |
| `test80-2023-07-jul-2tb7p.min-v2.v6.relabel-BT4-tf13tune.binpack` | `vondele/linrock_relabel_2` | 16,521,032,727 | 2.36% |
| `test80-2023-08-aug-2tb7p.v6.min.relabel-BT4-tf13tune.binpack` | `vondele/linrock_relabel_2` | 7,294,509,271 | 1.04% |
| `test80-2023-09-sep-2tb7p.min-v2.v6.relabel-BT4-tf13tune.binpack` | `vondele/linrock_relabel_2` | 8,627,821,314 | 1.23% |
| `test80-2023-10-oct-2tb7p.min-v2.v6.relabel-BT4-tf13tune.binpack` | `vondele/linrock_relabel_2` | 7,981,817,319 | 1.14% |
| `test80-2023-11-nov-2tb7p.min-v2.v6.relabel-BT4-tf13tune.binpack` | `vondele/linrock_relabel_2` | 7,214,046,403 | 1.03% |
| `test80-2023-12-dec-2tb7p.min-v2.v6.relabel-BT4-tf13tune.binpack` | `vondele/linrock_relabel_2` | 7,987,875,858 | 1.14% |
| `test80-2024-01-jan-2tb7p.min-v2.v6.relabel.binpack` | `xushawn/test80-bt4-relabel` | 9,295,363,159 | 1.33% |
| `test80-2024-02-feb-2tb7p.min-v2.v6.relabel.binpack` | `xushawn/test80-bt4-relabel` | 8,362,996,228 | 1.19% |
| `leela96-filt-v2.min.split_0.relabel-BT4-tf13tune.binpack` | `vondele/from_kaggle_1_relabel` | 19,550,050,458 | 2.79% |
| `leela96-filt-v2.min.split_1.relabel-BT4-tf13tune.binpack` | `vondele/from_kaggle_1_relabel` | 19,548,579,082 | 2.79% |
| `leela96-filt-v2.min.split_2.relabel-BT4-tf13tune.binpack` | `vondele/from_kaggle_1_relabel` | 19,549,769,139 | 2.79% |
| `leela96-filt-v2.min.split_3.relabel-BT4-tf13tune.binpack` | `vondele/from_kaggle_1_relabel` | 19,549,070,571 | 2.79% |
| `leela96-filt-v2.min.split_4.relabel-BT4-tf13tune.binpack` | `vondele/from_kaggle_1_relabel` | 19,548,894,060 | 2.79% |
| `T60T70wIsRightFarseerT60T74T75T76.split_0.relabel-BT4-tf13tune.binpack` | `vondele/from_kaggle_2_relabel` | 22,120,303,232 | 3.15% |
| `T60T70wIsRightFarseerT60T74T75T76.split_1.relabel-BT4-tf13tune.binpack` | `vondele/from_kaggle_2_relabel` | 22,145,581,828 | 3.16% |
| `T60T70wIsRightFarseerT60T74T75T76.split_2.relabel-BT4-tf13tune.binpack` | `vondele/from_kaggle_2_relabel` | 22,131,380,240 | 3.16% |
| `T60T70wIsRightFarseerT60T74T75T76.split_3.relabel-BT4-tf13tune.binpack` | `vondele/from_kaggle_2_relabel` | 22,118,569,308 | 3.15% |
| `T60T70wIsRightFarseerT60T74T75T76.split_4.relabel-BT4-tf13tune.binpack` | `vondele/from_kaggle_2_relabel` | 22,164,105,768 | 3.16% |
| `tb5dtm.binpack` | `vondele/rescored` | 139,519,453 | 0.02% |
