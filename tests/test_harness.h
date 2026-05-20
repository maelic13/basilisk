/// Minimal test harness shared by all unit-test binaries.
///
/// Include this header exactly once per test executable (i.e. from the
/// single .cpp that contains main()). The globals use static linkage so
/// they are private to each translation unit and do not collide with the
/// engine/board sources that are compiled into the same binary.

#pragma once

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <string>

// ---------------------------------------------------------------------------
// Per-binary state
// ---------------------------------------------------------------------------

static int  g_total   = 0;
static int  g_passed  = 0;
static bool g_sect_ok = true;

// ---------------------------------------------------------------------------
// Assertion macros
// ---------------------------------------------------------------------------

#define EXPECT(cond) do { \
    ++g_total; \
    if (cond) { ++g_passed; } \
    else { \
        g_sect_ok = false; \
        std::fprintf(stderr, "  FAIL: %s  (line %d)\n", #cond, __LINE__); \
    } \
} while (0)

#define EXPECT_EQ(a, b) do { \
    ++g_total; \
    auto _a = (a); auto _b = (b); \
    if (_a == _b) { ++g_passed; } \
    else { \
        g_sect_ok = false; \
        std::fprintf(stderr, "  FAIL: " #a " == " #b \
            "  (%lld != %lld, line %d)\n", \
            static_cast<long long>(_a), static_cast<long long>(_b), __LINE__); \
    } \
} while (0)

#define EXPECT_STR(a, b) do { \
    ++g_total; \
    std::string _a = (a); std::string _b = (b); \
    if (_a == _b) { ++g_passed; } \
    else { \
        g_sect_ok = false; \
        std::fprintf(stderr, "  FAIL: " #a " == " #b \
            "  (\"%s\" != \"%s\", line %d)\n", \
            _a.c_str(), _b.c_str(), __LINE__); \
    } \
} while (0)

// ---------------------------------------------------------------------------
// Section helpers
// ---------------------------------------------------------------------------

static void begin_section(const char* name) {
    std::printf("  %-52s ", name);
    g_sect_ok = true;
}

static void end_section() {
    std::printf("%s\n", g_sect_ok ? "ok" : "FAILED");
}

// ---------------------------------------------------------------------------
// Final summary (call at end of main())
// ---------------------------------------------------------------------------

static int harness_summary() {
    std::printf("\n%d / %d tests passed", g_passed, g_total);
    if (g_passed == g_total) {
        std::printf("  ✓\n");
        return 0;
    }
    std::printf("  ✗  (%d failed)\n", g_total - g_passed);
    return 1;
}
