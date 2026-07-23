#pragma once

#include <iostream>
#include <mutex>
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
