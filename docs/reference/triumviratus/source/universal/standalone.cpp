// Universal binary with separate files (09/10/2026, build_universal.ps1 -MultiTU): main of the single variant, for the
// instrumented builds of the PGO training (instead of entry.cpp). Same code as the TRIUMV_STANDALONE block of variant.cpp: the
// global constructors of the variant are in .tv<n>$m and the CRT does not run them, this main runs them.
#include "vns.h"

namespace TRIUMV_VNS { int main(); }

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
