/// An exception that escapes a thread other than the UCI thread is reported
/// on stdout before the process exits. CTest runs this with an expected exit
/// status of 1 and the CRITICAL ERROR line (CMakeLists.txt); without the
/// handler the process aborts and prints nothing.

#include "uci_output.h"

#include <stdexcept>
#include <thread>

int main() {
    install_fatal_error_report();
    std::thread worker([] { throw std::runtime_error("probe from a helper thread"); });
    worker.join();
    return 0;
}
