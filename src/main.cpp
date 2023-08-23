#include <chrono>
#include <iostream>
#include <thread>

#include "Constants.h"
#include "Engine.h"
#include "UciProtocol.h"

int main() {
    std::cout << engineName << " " << engineVersion << " by " << engineAuthor << "\n";

    std::mutex mutex;
    std::condition_variable conditionVariable;

    std::atomic_bool go = false;
    std::atomic_bool quit = false;
    Parameters parameters = Parameters();

    Engine engine = Engine(go, quit, parameters, mutex, conditionVariable);
    UciProtocol uciProtocol = UciProtocol(go, quit, parameters, conditionVariable);

    std::thread engineThread = std::thread(&Engine::start, &engine);
    uciProtocol.UciLoop();
    engineThread.join();

    return 0;
}
