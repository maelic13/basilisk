# v5 CandidatePassers — procedura (toolchain PRONTA e VERIFICATA in locale, 2026-07-18)

## Cosa è il blocco
96 feature (48 case orientate × {candidato proprio, candidato nemico}), folded in coda a
threatWeights dopo PassedPawns — **stesso slot v4 che occupava Outposts** (testato
negativo e rimosso, vedi `TrainingV4_Outposts/README.md`). Il pedone di colore C su
`sq` (file f, rank r) è **candidato passato** sse:
1. NON è già passato (i passati li copre il blocco PassedPawns), E
2. colonna semi-aperta davanti: nessun pedone (di entrambi i colori) su file f con
   rank strettamente davanti (niente doppiati davanti, niente bloccanti sul file), E
3. **conteggio maggioranza**: #(pedoni di C su file f±1 con rank dietro-o-pari) >=
   #(pedoni di ~C su file f±1 con rank strettamente davanti). Aiutanti dietro-o-pari
   vs sentinelle davanti.
Data la (2), "non passato" si riduce a: almeno una sentinella esiste.
NIENTE flag: square-only, pawn-event-only come PassedPawns — `DirtyPawns` riusato
identico (stesso XOR before/after). Hash blocco **0x43414E44** ("CAND"). Feature
string v4-slot: `Full_Threats+HalfKAv2_hm^+PawnPair+PassedPawns+CandidatePassers`
(hash composto FT 0xa9cc63ce).

**Index math IDENTICA a PassedPawns (`o-8`)**: i candidati sono pedoni, vivono sui
rank orientati 2-7 (di fatto 2-6: un pedone sulla 7a orientata è sempre passato →
mai candidato; la banda 48 regge). NESSUNA divergenza di banda come per Outposts.
MaxActive: 16 (ogni pedone candidato, bound teorico) — `full_threats.h` resta a 320.

## Stato verifica (TUTTO PASSATO in locale, 2026-07-18)
- Riferimento puro-Python [A]: self-check ok (`graft/candidate_passers.py`, FEN a mano:
  startpos=0, maggioranza 1v1 limite / 2v1 sì / 1v2 no, doppiato davanti no,
  già-passato escluso (c3 passato per il riferimento PassedPawns), specchio nero,
  bande own 0..47 / enemy 48..95, mirror del re a1-vs-h1).
- Loader C++ == riferimento su 8 FEN, e PawnPair/PassedPawns NON spostati [C]
  (`graft/check_loader_candidates.py`, dll ricompilata in `build_check/` con
  `cmake --build build_check --config Release --target training_data_loader` e
  copiata in `build/`).
- Zero-init identity [B]: FT forward identico con/senza feature candidate attive.
- Engine **tri-format** (un solo binario carica v2/v3/v4-slot):
  - net v4-slot zerograft (graft su v3 plain → serialize → reader nativo): bench
    **592074** byte-identico al v3;
  - net v3 (`nn-rubicon-alea-v3.nnue`, zero-fill CandidatePassers): bench **592074** = release;
  - net v2 (`nn-rubicon-alea-v2.nnue`, zero-fill PassedPawns+CandidatePassers): bench **440335**.
- `nnueverify on` + `incremental on` + bench con net a pesi candidate RANDOM:
  **0 MISMATCH** (incrementale == full-refresh ad ogni foglia; bench 547136 ≠ 592074
  → le feature sparano). Zerograft con incremental+verify: 592074, 0 mismatch.
- **Mapping indici engine == trainer** (test single-row): net con SOLO la riga 21
  non-zero (own candidate c4, prospettiva bianca, Re e1 → o=26^7=29, 29-8=21;
  FEN `4k3/8/1p6/8/2P5/1P6/8/4K3`) sposta l'eval (491→583), la riga 22 NON la
  sposta (491=491).

## File toccati
- **Engine** (`Triumviratus_6/`): `nnue/nnue/features/candidate_passers.{h,cpp}` (NUOVI,
  `outposts.{h,cpp}` ELIMINATI), `nnue_architecture.h` (include + `CandidateFeatureSet`),
  `nnue_feature_transformer.h` (5ª stanza read/write invariata nella struttura, rinominata
  `CandidateInputDimensions`; enum `LoadCompat` tri-format RESTA), `nnue_accumulator.cpp`
  (3 call-site), `network.{h,cpp}` (commenti hash compat), `Triumviratus_5.0.vcxproj`, `Makefile`.
- **Trainer** (`Training_NNUE/TrainingAleaV2_Grafting/nnue-pytorch/`, da portare sulla VM):
  `model/modules/features/candidate_passers.py` (NUOVO, `outposts.py` ELIMINATO),
  `features/__init__.py`, `model/optimizers/config.py` (`candidatepassers_lr` al posto di
  `outposts_lr`), `model/lightning_module.py` (multi-group),
  `data_loader/cpp/training_data_loader.cpp` (struct `CandidatePassers` + extractor +
  registrazione; riusa `PassedPawns::pawn_id`).
- **Graft/verify** (`Training_NNUE/TrainingV5_CandidatePassers/graft/`, NUOVI):
  `candidate_passers.py` (riferimento+self-check), `graft_candidates.py`,
  `verify_candidates.py`, `check_loader_candidates.py`; net di test usa-e-getta
  `alea_v5_zerograft.pt`, `alea_v5_randcand.pt`, `alea_v5_row21.pt`, `alea_v5_row22.pt`,
  `nn-v5-zerograft.nnue`, `nn-v5-randcand.nnue`, `nn-v5-row21.nnue`, `nn-v5-row22.nnue`
  (cancellabili, ricreabili dagli script).
- **Run**: `train_v5.sh` (questa cartella).

## Net di partenza per il graft: il PLAIN, NON il perm
Il graft va fatto su **`Networks_Triumviratus_6/v3_ep4_screening_CONFIRMED.nnue`**
(= v3 finale PLAIN). `nn-rubicon-alea-v3-final-perm.nnue` è il net di SHIP con la
permutazione FT applicata (ottimizzazione inference ~+2%): eval identica ma ordine
colonne L1 diverso — grafta il plain, allena, e riapplica `ftperm` solo alla
serializzazione FINALE.

## Procedura sulla VM (`trium-training`, layout `~/alea/{venv,nnue-pytorch,graft}`)
1. Porta i file trainer+graft (scp via `/tmp` poi `cp` — sticky-bit; path ASSOLUTI).
   ⚠️ Gotcha noti della VM:
   - `PYTHONPATH=~/alea/nnue-pytorch:~/alea/graft`
   - `__init__.py` va in `model/modules/features/` (non dimenticare di sovrascriverlo)
   - il loader PGO vuole il symlink `.pgo/small.binpack`
   - RICOMPILA il loader dalla dir nnue-pytorch della VM:
     `sed -i 's/\r$//' compile_data_loader.sh && bash compile_data_loader.sh`
     (il sed toglie i CRLF Windows, lezione v3).
2. Verifica loader: `python <graft>/check_loader_candidates.py` → [C] OK atteso.
   (Lo script aggiunge ai path `TrainingAleaV2_Grafting/graft` per riusare
   `verify_graft.py`/`verify_passedpawns.py`: sulla VM tieni le cartelle graft
   sorelle come in locale, o aggiusta i `sys.path.insert` in testa.)
3. Graft sul **v3 finale PLAIN**:
   `python <graft>/graft_candidates.py v3_ep4_screening_CONFIRMED.nnue <graft>/alea_v5_grafted.pt`
   `python <graft>/verify_candidates.py v3_ep4_screening_CONFIRMED.nnue <graft>/alea_v5_grafted.pt` → [A]+[B] OK.
4. **Screening FROZEN-BASE** (SOLO frozen — lezione Outposts: il co-adapt su questa
   rete danneggia-e-pareggia): `bash train_v5.sh blackwell /home/USER/data`
   = base **--lr 0** (se il trainer protesta: 1e-9), `--candidatepassers-lr 1e-3`,
   gamma **0.99**, 12 epoche × 100M, batch 65536, `--network-save-period 2 --save-top-k -1`,
   **λ 0.75 costante e WDL default invariato: FISSI BY DESIGN**.
   All'avvio DEVE stampare `[configure_optimizers] CandidatePassers block lr=0.001`.
5. **Go/no-go SENZA aspettare la fine**: serializza ep3 e ep5 plain (NO ft_optimize):
   `python serialize.py <ckpt> vX.nnue --features "Full_Threats+HalfKAv2_hm^+PawnPair+PassedPawns+CandidatePassers"`
   e gate net-isolated vs zerograft (STESSO binario tri-format su entrambi i lati,
   ponder OFF), **~2000 partite 12+0.12**. Su v3 il segnale c'era già a ep3-4;
   il blocco satura in ~4 epoche (lezione PassedPawns: lo screening ERA il training).
   ⚠️ fastchess: opzioni spin con `=1/=0`, MAI `true/false`.
6. Verdetto: **≥ +5 → GO** (ftperm sul finale, bench, NETWORKS.md, ship come per v3);
   piatto/negativo a ep5 → feature scartata (il verdetto frozen-base È isolato sul blocco).

## Note
- Il bench di riferimento del source attuale: **592074** (net v3, default embedded).
- Il motore carica v2/v3/v4-slot → nessun doppio binario per i gate.
- Trainer: NUM_REAL_FEATURES 87904→88000; spazio training 89952→90048
  (offset CandidatePassers nel training space: 89952 — identico allo slot Outposts).
