// Universal binary (08/10/2026): ONE translation unit per ISA variant. The whole engine (the project's .cpp files
// except fathom, which is C and ISA-independent) lives in the variant's namespace, chosen by -DTRIUMV_VID=<n>:
//   1 avx2-nopext   2 avx2   3 avx512   4 vnni512   5 avx512icl
// The engine's main becomes Triumv_v<n>::main, called by entry.cpp after cpuid.
// The global constructors of the variant go into the section .tv<n>$m and not into .CRT$XCU: the CRT does not run them,
// entry.cpp runs them only for the chosen variant. Compiled with the variant's instructions (AVX-512 ...), on a CPU
// that does not have them they must not start.
#include "prelude.h"

#if TRIUMV_VID == 1
#pragma init_seg(".tv1$m")
#define TRIUMV_VNS Triumv_v1
#elif TRIUMV_VID == 2
#pragma init_seg(".tv2$m")
#define TRIUMV_VNS Triumv_v2
#elif TRIUMV_VID == 3
#pragma init_seg(".tv3$m")
#define TRIUMV_VNS Triumv_v3
#elif TRIUMV_VID == 4
#pragma init_seg(".tv4$m")
#define TRIUMV_VNS Triumv_v4
#elif TRIUMV_VID == 6
#pragma init_seg(".tv6$m")
#define TRIUMV_VNS Triumv_v6
#elif TRIUMV_VID == 7
#pragma init_seg(".tv7$m")
#define TRIUMV_VNS Triumv_v7
#elif TRIUMV_VID == 5
#pragma init_seg(".tv5$m")
#define TRIUMV_VNS Triumv_v5
#else
#error "TRIUMV_VID mancante (1..7)"
#endif

namespace TRIUMV_VNS {
#include "../attacks.cpp"
#include "../init.cpp"
#include "../io.cpp"
#include "../magic.cpp"
#include "../main.cpp"
#include "../misc.cpp"
#include "../movegen.cpp"
#include "../perft.cpp"
#include "../presentation.cpp"
#include "../random.cpp"
#include "../see.cpp"
#include "../threads.cpp"
#include "../uci_mt.cpp"
#include "../zobrist.cpp"
#include "../chess960.cpp"
#include "../consilium_bridge.cpp"
#include "../sliders.cpp"
#include "../large_pages.cpp"
#include "../consilium/cn.cpp"
#include "../syzygy.cpp"
}  // namespace TRIUMV_VNS

#ifdef TRIUMV_STANDALONE
// Build of the single variant (PGO training and trials): the real main calls the variant's one. Here the global
// constructors are in .tv<n>$m and the CRT does not run them: this main runs them, as entry.cpp does.
typedef void(__cdecl *TvInit)(void);
#if TRIUMV_VID == 1
#pragma section(".tv1$a", read)
#pragma section(".tv1$z", read)
__declspec(allocate(".tv1$a")) const TvInit tv_a = nullptr;
__declspec(allocate(".tv1$z")) const TvInit tv_z = nullptr;
#elif TRIUMV_VID == 2
#pragma section(".tv2$a", read)
#pragma section(".tv2$z", read)
__declspec(allocate(".tv2$a")) const TvInit tv_a = nullptr;
__declspec(allocate(".tv2$z")) const TvInit tv_z = nullptr;
#elif TRIUMV_VID == 3
#pragma section(".tv3$a", read)
#pragma section(".tv3$z", read)
__declspec(allocate(".tv3$a")) const TvInit tv_a = nullptr;
__declspec(allocate(".tv3$z")) const TvInit tv_z = nullptr;
#elif TRIUMV_VID == 4
#pragma section(".tv4$a", read)
#pragma section(".tv4$z", read)
__declspec(allocate(".tv4$a")) const TvInit tv_a = nullptr;
__declspec(allocate(".tv4$z")) const TvInit tv_z = nullptr;
#elif TRIUMV_VID == 6
#pragma section(".tv6$a", read)
#pragma section(".tv6$z", read)
__declspec(allocate(".tv6$a")) const TvInit tv_a = nullptr;
__declspec(allocate(".tv6$z")) const TvInit tv_z = nullptr;
#elif TRIUMV_VID == 7
#pragma section(".tv7$a", read)
#pragma section(".tv7$z", read)
__declspec(allocate(".tv7$a")) const TvInit tv_a = nullptr;
__declspec(allocate(".tv7$z")) const TvInit tv_z = nullptr;
#else
#pragma section(".tv5$a", read)
#pragma section(".tv5$z", read)
__declspec(allocate(".tv5$a")) const TvInit tv_a = nullptr;
__declspec(allocate(".tv5$z")) const TvInit tv_z = nullptr;
#endif
extern "C" const char *g_triumv_isa = "standalone";
int main() {
  for (const TvInit *p = &tv_a + 1; p < &tv_z; ++p)
    if (*p)
      (*p)();
  return TRIUMV_VNS::main();
}
#endif
