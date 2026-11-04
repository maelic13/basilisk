// Universal binary with separate files (09/10/2026, build_universal.ps1 -MultiTU): namespace and section of the global
// constructors of the variant -DTRIUMV_VID=<n>, shared by all the wrappers of the variant (one wrapper for every .cpp
// of the engine, like the units of the separate build). Same rules as variant.cpp: the constructors go into .tv<n>$m and
// entry.cpp (or standalone.cpp) runs them only for the chosen variant.
#pragma once

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
#elif TRIUMV_VID == 5
#pragma init_seg(".tv5$m")
#define TRIUMV_VNS Triumv_v5
#elif TRIUMV_VID == 6
#pragma init_seg(".tv6$m")
#define TRIUMV_VNS Triumv_v6
#elif TRIUMV_VID == 7
#pragma init_seg(".tv7$m")
#define TRIUMV_VNS Triumv_v7
#else
#error "TRIUMV_VID mancante (1..7)"
#endif
