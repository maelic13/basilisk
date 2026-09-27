#pragma once

// The bundled KQvK Syzygy fixture (tests/fixtures/syzygy, base64), written to
// a temporary directory on first use. Shared by the tests that need real
// tablebase probes. The including target must define
// BASILISK_TEST_SYZYGY_FIXTURE_DIR.

#include "syzygy.h"

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

static int base64_value(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

static std::vector<unsigned char> decode_base64(std::string_view encoded) {
    std::vector<unsigned char> decoded;
    decoded.reserve(encoded.size() * 3 / 4);
    unsigned accumulator = 0;
    int bits = 0;

    for (char c : encoded) {
        if (c == '=')
            break;
        const int value = base64_value(c);
        if (value < 0)
            continue;
        accumulator = (accumulator << 6) | static_cast<unsigned>(value);
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            decoded.push_back(static_cast<unsigned char>(accumulator >> bits));
            accumulator &= (1u << bits) - 1u;
        }
    }
    return decoded;
}

static void materialize_syzygy_file(const std::filesystem::path& source,
                                    const std::filesystem::path& destination,
                                    size_t expected_size) {
    std::ifstream input(source, std::ios::binary);
    if (!input)
        throw std::runtime_error("missing Syzygy test fixture: " + source.string());
    const std::string encoded((std::istreambuf_iterator<char>(input)),
                              std::istreambuf_iterator<char>());
    const auto decoded = decode_base64(encoded);
    if (decoded.size() != expected_size)
        throw std::runtime_error("invalid Syzygy fixture size: " + source.string());

    std::ofstream output(destination, std::ios::binary);
    output.write(reinterpret_cast<const char*>(decoded.data()),
                 static_cast<std::streamsize>(decoded.size()));
    if (!output)
        throw std::runtime_error("could not materialize Syzygy fixture: "
                                 + destination.string());
}

struct SyzygyFixtureDirectory {
    std::filesystem::path path;

    SyzygyFixtureDirectory() {
        const auto nonce = std::chrono::high_resolution_clock::now()
                               .time_since_epoch().count();
        const auto temp = std::filesystem::temp_directory_path();
        bool created = false;
        for (int attempt = 0; attempt < 100; ++attempt) {
            path = temp / ("basilisk-syzygy-test-" + std::to_string(nonce)
                           + "-" + std::to_string(attempt));
            if (std::filesystem::create_directory(path)) {
                created = true;
                break;
            }
        }
        if (!created)
            throw std::runtime_error("could not create temporary Syzygy fixture directory");

        const std::filesystem::path source(BASILISK_TEST_SYZYGY_FIXTURE_DIR);
        materialize_syzygy_file(source / "KQvK.rtbw.b64", path / "KQvK.rtbw", 272);
        materialize_syzygy_file(source / "KQvK.rtbz.b64", path / "KQvK.rtbz", 5392);
    }

    ~SyzygyFixtureDirectory() {
        Syzygy::clear();
        std::error_code error;
        std::filesystem::remove_all(path, error);
    }
};

static const std::filesystem::path& syzygy_fixture_path() {
    static SyzygyFixtureDirectory fixture;
    return fixture.path;
}
