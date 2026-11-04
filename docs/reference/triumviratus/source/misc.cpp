// Origin: this file descends from BBC (Bit Board Chess) by Maksim Korzh, https://github.com/maksimKorzh/bbc,
// released under the GNU General Public License v3; the project started from BBC in 2024 and has since rewritten
// and extended much of it. Triumviratus is distributed under the GNU GPL v3 (see COPYING).
// The input polling (input_waiting, read_input) comes to BBC from VICE by Richard Allbert.
#include "defs.h"
#include <cstdio>
#include <cstring>
#include <chrono>

#ifdef _WIN32
  #include "Windows.h"
  #include <io.h>
  #include <fcntl.h>
  #include <intrin.h>
  #include <conio.h>
#else
  #include <unistd.h>
  #include <sys/select.h>
  #include <sys/time.h>
  #include <chrono>
#endif

// get time in milliseconds
// 12/10/2026: on Windows it was GetTickCount(), which advances in steps of ~15.6 ms: the time check overshot by ~10 ms
// per move (go movetime 100 -> 109 ms) and every clock decision had that grain. Now steady_clock (QPC) on
// all platforms, counted from the process start (no int wrap-around after 24.8 days).
int get_time_ms()
{
    using namespace std::chrono;
    static const steady_clock::time_point t0 = steady_clock::now();
    return (int)duration_cast<milliseconds>(steady_clock::now() - t0).count();
}

int input_waiting()
{
#ifdef _WIN32
    return _kbhit();
#else
    // POSIX: does stdin have data ready? (non-blocking poll via select)
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);
    struct timeval tv = {0, 0};
    return select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv) > 0;
#endif
}

// read GUI/user input
void read_input()
{
    if (input_waiting())
    {
        char input[256] = "";
        int  bytes;
#ifdef _WIN32
        bytes = _read(_fileno(stdin), input, 255);
#else
        bytes = read(fileno(stdin), input, 255);
#endif
        // EOF (closed pipe) or error: do NOT stop the search. On Linux select()
        // signals "readable" at EOF too: treating it as stop aborted the
        // fixed-depth search at once -> no move -> rescue path.
        if (bytes <= 0) return;
        input[bytes] = '\0';
        char* endc = strchr(input, '\n');
        if (endc) *endc = '\0';
        // Only the UCI interruption commands stop the search.
        if (!strncmp(input, "quit", 4)) { quit = 1; stopped = 1; }
        else if (!strncmp(input, "stop", 4)) { stopped = 1; }
    }
}

// a bridge function to interact between search and GUI input
void communicate() {
    if (timeset == 1 && get_time_ms() > stoptime) {
        stopped = 1;
    }
    read_input();
}

// count_bits / get_ls1b_index: ora `static inline` in defs.h (09/09/2026).

