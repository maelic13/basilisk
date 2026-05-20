cmake_minimum_required(VERSION 3.24)

project(basilisk VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------
set(SOURCES
    src/Basilisk.cpp
    src/Board.cpp
    src/Engine.cpp
    src/Parameters.cpp
    src/UciProtocol.cpp
    src/bitboard.cpp
    src/move.cpp
    src/attacks.cpp
    src/zobrist.cpp
    src/eval.cpp
    src/search.cpp
)

add_executable(basilisk ${SOURCES})

target_include_directories(basilisk PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# ---------------------------------------------------------------------------
# Compiler flags
# ---------------------------------------------------------------------------
# ---------------------------------------------------------------------------
# Compiler flags
# ---------------------------------------------------------------------------
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(basilisk PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wconversion
        -Wno-unused-parameter
        $<$<CONFIG:Release>:-O3 -march=native -ffast-math>
        $<$<CONFIG:RelWithDebInfo>:-O2 -g -march=native>
        $<$<CONFIG:Debug>:-O0 -g3>
    )
    target_link_options(basilisk PRIVATE
        $<$<CONFIG:Debug>:-g3>
    )
    if(SANITIZE)
        target_compile_options(basilisk PRIVATE -fsanitize=address,undefined)
        target_link_options(basilisk PRIVATE -fsanitize=address,undefined)
    endif()

elseif(MSVC)
    target_compile_options(basilisk PRIVATE
        /W4
        /WX-
        /utf-8
        $<$<CONFIG:Release>:/O2 /GL>
        $<$<CONFIG:Debug>:/Od /Zi>
    )
    target_link_options(basilisk PRIVATE
        $<$<CONFIG:Release>:/LTCG>
    )
endif()

# ---------------------------------------------------------------------------
# Link-time optimisation (Release only, non-MSVC handled above via /GL+/LTCG)
# ---------------------------------------------------------------------------
if(NOT MSVC)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT _lto_ok OUTPUT _lto_msg)
    if(_lto_ok)
        set_property(TARGET basilisk PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    else()
        message(STATUS "LTO not supported: ${_lto_msg}")
    endif()
endif()

# ---------------------------------------------------------------------------
# Optional BMI2 PEXT support (faster sliding piece attacks on Haswell+)
# ---------------------------------------------------------------------------
option(USE_PEXT "Use BMI2 PEXT instruction for magic bitboards (requires Haswell+)" OFF)
if(USE_PEXT)
    target_compile_definitions(basilisk PRIVATE USE_PEXT)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(basilisk PRIVATE -mbmi2)
    elseif(MSVC)
        target_compile_options(basilisk PRIVATE /arch:AVX2)
    endif()
endif()

# ---------------------------------------------------------------------------
# Static runtime (default ON on MinGW to avoid libc++.dll / libgcc_s.dll
# dependency failures when the binary is run outside the MSYS2 environment)
# ---------------------------------------------------------------------------
option(STATIC_RUNTIME "Link the C++ runtime and standard library statically" ${MINGW})
if(STATIC_RUNTIME)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_link_options(basilisk PRIVATE -static)
        message(STATUS "Static runtime linking enabled (-static)")
    endif()
endif()

# ---------------------------------------------------------------------------
# Default build type
# ---------------------------------------------------------------------------
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Build type not set — defaulting to Release")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
endif()
