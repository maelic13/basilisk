#include "nnue.h"

#include <algorithm>
#include <atomic>
#include <cstring>
#include <fstream>
#include <vector>

#include "Board.h"
#include "bitboard.h"

// Compile-time embedded network (generated into the build tree by CMake;
// a stub with size 0 when -DBASILISK_NNUE_FILE was not given).
extern const unsigned char g_nnue_embedded_data[];
extern const std::size_t g_nnue_embedded_size;

namespace nnue {
namespace {

struct Net {
    int h = 0;
    int qa = 0, qb = 0, scale = 0;
    std::vector<int16_t> ft_w;   // [768 * h], feature-major
    std::vector<int16_t> ft_b;   // [h]
    std::vector<int16_t> out_w;  // [2h], us-half then them-half
    int32_t out_b = 0;
    bool loaded = false;
};

Net g_net;
std::atomic<bool> g_enabled{false};
std::string g_source = "<none>";

constexpr int NUM_FEATURES = 768;

bool parse(const unsigned char* data, std::size_t size, Net& net) {
    if (size < 14 || std::memcmp(data, "MNN1", 4) != 0)
        return false;
    const unsigned char* p = data + 4;
    const int arch = p[0], act = p[1];
    auto rd16 = [&](int off) { return int(p[off]) | int(p[off + 1]) << 8; };
    const int h = rd16(2), qa = rd16(4), qb = rd16(6), scale = rd16(8);
    if (arch != 1 || act != 1 || h <= 0 || h > 4096 || qa <= 0 || qb <= 0)
        return false;

    const std::size_t need = 14
        + std::size_t(NUM_FEATURES) * h * 2  // ft_w
        + std::size_t(h) * 2                 // ft_b
        + std::size_t(2) * h * 2             // out_w
        + 4;                                 // out_b
    if (size != need)
        return false;

    net.h = h; net.qa = qa; net.qb = qb; net.scale = scale;
    const unsigned char* w = data + 14;
    auto read_i16 = [&](std::vector<int16_t>& dst, std::size_t count) {
        dst.resize(count);
        std::memcpy(dst.data(), w, count * 2);
        w += count * 2;
    };
    read_i16(net.ft_w, std::size_t(NUM_FEATURES) * h);
    read_i16(net.ft_b, h);
    read_i16(net.out_w, std::size_t(2) * h);
    std::memcpy(&net.out_b, w, 4);
    net.loaded = true;
    return true;
}

// Feature index for one perspective (docs/mnn_format.md):
//   (piece color != perspective) * 384 + ptype0 * 64 + (sq, flipped for BLACK)
inline int feature(Color perspective, Color pc, int ptype0, int sq) {
    const int rel_color = (pc != perspective) ? 1 : 0;
    const int rel_sq = perspective == WHITE ? sq : sq ^ 56;
    return rel_color * 384 + ptype0 * 64 + rel_sq;
}

}  // namespace

bool load_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f)
        return false;
    std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(f)),
                                     std::istreambuf_iterator<char>());
    Net fresh;
    if (!parse(bytes.data(), bytes.size(), fresh))
        return false;
    g_net = std::move(fresh);
    g_source = path;
    return true;
}

bool load_embedded() {
    if (g_nnue_embedded_size == 0)
        return false;
    Net fresh;
    if (!parse(g_nnue_embedded_data, g_nnue_embedded_size, fresh))
        return false;
    g_net = std::move(fresh);
    g_source = "<embedded>";
    return true;
}

bool available() { return g_net.loaded; }

bool set_enabled(bool on) {
    if (on && !g_net.loaded)
        return false;
    g_enabled.store(on, std::memory_order_relaxed);
    return true;
}

bool enabled() {
    return g_enabled.load(std::memory_order_relaxed) && g_net.loaded;
}

int evaluate(const Board& b) {
    const Net& net = g_net;
    const int h = net.h;
    // Bring-up path: full recompute of both perspective accumulators.
    std::vector<int32_t> acc_us(net.ft_b.begin(), net.ft_b.end());
    std::vector<int32_t> acc_th(net.ft_b.begin(), net.ft_b.end());
    const Color stm = b.side_to_move;

    for (int c = WHITE; c < NCOLORS; ++c) {
        for (int pt = PAWN; pt < PIECE_TYPE_NB; ++pt) {
            Bitboard bb = b.pieces[c][pt];
            while (bb) {
                const int sq = pop_lsb(bb);
                const int ptype0 = pt - 1;
                const int16_t* row_us =
                    &net.ft_w[std::size_t(feature(stm, Color(c), ptype0, sq)) * h];
                const int16_t* row_th =
                    &net.ft_w[std::size_t(feature(Color(stm ^ 1), Color(c), ptype0, sq)) * h];
                for (int i = 0; i < h; ++i) acc_us[i] += row_us[i];
                for (int i = 0; i < h; ++i) acc_th[i] += row_th[i];
            }
        }
    }

    int64_t s = 0;
    for (int i = 0; i < h; ++i) {
        const int64_t a = std::clamp(acc_us[i], 0, net.qa);
        s += a * a * net.out_w[i];
    }
    for (int i = 0; i < h; ++i) {
        const int64_t a = std::clamp(acc_th[i], 0, net.qa);
        s += a * a * net.out_w[h + i];
    }
    // The contract specifies FLOORED division (round toward -inf, matching
    // the Python reference); C++ '/' truncates toward zero, so negatives
    // need the adjustment.
    const auto floordiv = [](int64_t a, int64_t b) {  // b > 0
        return a >= 0 ? a / b : -((-a + b - 1) / b);
    };
    const int64_t pre = floordiv(s, net.qa) + net.out_b;
    return int(floordiv(pre * net.scale, int64_t(net.qa) * net.qb));
}

std::string status() {
    if (!g_net.loaded)
        return "NNUE: no network loaded";
    return "NNUE: " + std::string(enabled() ? "ON" : "off") + ", net "
        + g_source + " (h=" + std::to_string(g_net.h) + ")";
}

}  // namespace nnue
