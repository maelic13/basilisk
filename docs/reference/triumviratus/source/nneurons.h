// nneurons.h -- study of the neurons of the network's first layer (09/10/2026), ONLY with -DTRIUMV_NEURONS.
// For every real evaluation of the search it records the outputs of the transformation (clipped ReLU and pairwise product:
// output j = clamp(acc[j]) * clamp(acc[j + L1/2]) / 512, for the two perspectives) separated per material band,
// i.e. per expert of HalfKAv2_hm_P4 (pieces on the board, kings included: <=9, 10-15, 16-23, >=24):
//  - statistics per output: evaluations with a non-zero output, sum, sum of squares, outputs at the maximum;
//  - a sample of the whole vectors (one every NEURONS_EVERY evaluations, at most NEURONS_MAX per band) with
//    number of pieces, material difference from the side to move and king squares, for correlations and duplicates.
// Output in the folder of the environment variable NEURONS_DIR (nothing if it is not set): stats_b<k>.txt and
// sample_b<k>.bin (record: 8-byte header + the outputs). Analysis: tools/neurons_analyze.py.
// Meant for one thread (Threads 1): the counters are not atomic. Without TRIUMV_NEURONS: identical code.
#pragma once

#ifdef TRIUMV_NEURONS
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace NNeurons {
constexpr int Bands = 4, MaxOut = 4096;
inline std::uint64_t evals[Bands];
inline std::uint64_t nz[Bands][MaxOut], sum[Bands][MaxOut], sumsq[Bands][MaxOut], sat[Bands][MaxOut];
inline int           width = 0;
inline std::uint64_t kept[Bands];
inline std::FILE*    sample[Bands];
inline std::FILE*    meta[Bands];   // positions of the sample records (record further below)
inline long          every = -1, maxkeep = 0;
inline std::string   dir;

inline int band_of_count(int pieces) { return pieces <= 9 ? 0 : pieces <= 15 ? 1 : pieces <= 23 ? 2 : 3; }

inline void dump() {
    if (dir.empty() || !width) return;
    for (int b = 0; b < Bands; b++) {
        if (sample[b]) std::fclose(sample[b]);
        if (meta[b]) std::fclose(meta[b]);
        std::FILE* o = std::fopen((dir + "/stats_b" + std::to_string(b) + ".txt").c_str(), "w");
        if (!o) continue;
        std::fprintf(o, "evals %llu width %d\n", (unsigned long long) evals[b], width);
        for (int j = 0; j < width; j++)
            std::fprintf(o, "%d %llu %llu %llu %llu\n", j, (unsigned long long) nz[b][j], (unsigned long long) sum[b][j],
                         (unsigned long long) sumsq[b][j], (unsigned long long) sat[b][j]);
        std::fclose(o);
    }
}

inline void init() {
    const char* d = std::getenv("NEURONS_DIR");
    every   = 0;
    if (!d) return;
    dir     = d;
    const char* e = std::getenv("NEURONS_EVERY");
    const char* m = std::getenv("NEURONS_MAX");
    every   = e ? std::atol(e) : 16;
    maxkeep = m ? std::atol(m) : 100000;
    for (int b = 0; b < Bands; b++)
        sample[b] = std::fopen((dir + "/sample_b" + std::to_string(b) + ".bin").c_str(), "wb");
    std::atexit(dump);
}

// out: the outputs of the transformation (n bytes, n = 2 * L1/2); pieces, matdiff (centipawns from the side to move),
// stm, ksq_us, ksq_them: from the evaluated position.
// bbs (evening of 09/10/2026, for the clustering of the outputs and the search for concepts): the twelve bitboards per piece
// of the position (P N B R Q K p n b r q k, squares a8 = 0 .. h1 = 63), written in meta_b<k>.bin next to every record
// of the sample (104 bytes: 12 x 8 bytes, side to move, 7 zero bytes), so the analysis rebuilds the position.
inline void record(const std::uint8_t* out, int n, int pieces, int matdiff, int stm, int ksq_us, int ksq_them,
                   const unsigned long long* bbs = nullptr) {
    if (every < 0) init();
    if (dir.empty()) return;
    width        = n;
    const int b  = band_of_count(pieces);
    const auto k = evals[b]++;
    for (int j = 0; j < n; j++) {
        const unsigned v = out[j];
        nz[b][j] += v != 0;
        sum[b][j] += v;
        sumsq[b][j] += v * v;
        sat[b][j] += v >= 127;
    }
    if (sample[b] && every > 0 && k % every == 0 && kept[b] < std::uint64_t(maxkeep)) {
        const std::int16_t md = std::int16_t(matdiff);
        const std::uint8_t h[8] = {std::uint8_t(pieces), std::uint8_t(stm), std::uint8_t(ksq_us), std::uint8_t(ksq_them),
                                   std::uint8_t(md & 0xff), std::uint8_t((md >> 8) & 0xff), 0, 0};
        std::fwrite(h, 1, 8, sample[b]);
        std::fwrite(out, 1, n, sample[b]);
        if (bbs) {
            if (!meta[b]) meta[b] = std::fopen((dir + "/meta_b" + std::to_string(b) + ".bin").c_str(), "wb");
            if (meta[b]) {
                const std::uint8_t tail[8] = {std::uint8_t(stm), 0, 0, 0, 0, 0, 0, 0};
                std::fwrite(bbs, 8, 12, meta[b]);
                std::fwrite(tail, 1, 8, meta[b]);
            }
        }
        kept[b]++;
    }
}
}  // namespace NNeurons
#endif
