#pragma once

// The process's one stdout sink. The protocol, the engine, bench and the WAC
// runner write through it, so lines from different threads never interleave.
// It knows nothing of UCI: the engine layer may use it without depending on
// the protocol above it.

#include <cstdlib>
#include <exception>
#include <iostream>
#include <mutex>
#include <string>
#include <string_view>

inline std::mutex& output_mutex() {
    static std::mutex mutex;
    return mutex;
}

// A write that fails means the reader is gone (a GUI that closed our stdout,
// a closed pipe): nothing the engine does can be seen any more, so it ends
// here, on every platform, rather than search on unread. The caller holds the
// output mutex; other threads are still running, hence _Exit.
inline void end_if_output_lost() {
    if (std::cout)
        return;
    std::cerr << "stdout is closed; exiting.\n";
    std::_Exit(0);
}

inline void write_output(std::string_view text) {
    std::scoped_lock lock(output_mutex());
    std::cout << text;
    std::cout.flush();
    end_if_output_lost();
}

inline void write_output_line(std::string_view line) {
    std::scoped_lock lock(output_mutex());
    std::cout << line << '\n';
    std::cout.flush();
    end_if_output_lost();
}

// An exception that leaves any thread but the UCI thread (the engine thread,
// a search helper) ends in std::terminate, which by default aborts with
// nothing on stdout, where GUIs and harnesses look. This handler reports it
// there and exits. The dying thread may hold the output mutex, so the report
// does not wait for it.
[[noreturn]] inline void report_terminate() {
    std::string what = "std::terminate without an exception";
    if (const std::exception_ptr error = std::current_exception()) {
        try {
            std::rethrow_exception(error);
        } catch (const std::exception& e) {
            what = std::string("unhandled exception: ") + e.what();
        } catch (...) {
            what = "unhandled non-standard exception";
        }
    }
    {
        std::unique_lock lock(output_mutex(), std::try_to_lock);
        std::cout << "info string CRITICAL ERROR: " << what << "; exiting." << std::endl;
    }
    std::cerr << "CRITICAL ERROR: " << what << '\n';
    // _Exit, not exit: other threads are still running.
    std::_Exit(1);
}

inline void install_fatal_error_report() {
    std::set_terminate(report_terminate);
}
