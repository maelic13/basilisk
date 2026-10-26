#pragma once

#include <cstdlib>
#include <exception>
#include <iostream>
#include <mutex>
#include <string>
#include <string_view>

inline std::mutex& uci_output_mutex() {
    static std::mutex mutex;
    return mutex;
}

inline void uci_write(std::string_view text) {
    std::scoped_lock lock(uci_output_mutex());
    std::cout << text;
    std::cout.flush();
}

inline void uci_write_line(std::string_view line) {
    std::scoped_lock lock(uci_output_mutex());
    std::cout << line << '\n';
    std::cout.flush();
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
        std::unique_lock lock(uci_output_mutex(), std::try_to_lock);
        std::cout << "info string CRITICAL ERROR: " << what << "; exiting." << std::endl;
    }
    std::cerr << "CRITICAL ERROR: " << what << '\n';
    // _Exit, not exit: other threads are still running.
    std::_Exit(1);
}

inline void install_fatal_error_report() {
    std::set_terminate(report_terminate);
}
