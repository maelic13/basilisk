cmake_minimum_required(VERSION 3.24)

set(COMP "auto" CACHE STRING "Compiler selection: auto, clang, gcc, or llvm")
set_property(CACHE COMP PROPERTY STRINGS auto clang gcc llvm)

string(TOLOWER "${COMP}" _basilisk_compiler)
string(REPLACE "_" "-" _basilisk_compiler "${_basilisk_compiler}")

function(_basilisk_detect_darwin_processor out_var system_name processor)
    set(_processor "${processor}")
    if("${system_name}" STREQUAL "Darwin" AND _processor STREQUAL "")
        execute_process(
            COMMAND uname -m
            RESULT_VARIABLE _uname_result
            OUTPUT_VARIABLE _uname_processor
            ERROR_QUIET
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        if(_uname_result STREQUAL "0")
            set(_processor "${_uname_processor}")
        endif()
    endif()
    string(TOLOWER "${_processor}" _processor)
    set("${out_var}" "${_processor}" PARENT_SCOPE)
endfunction()

if(_basilisk_compiler STREQUAL "auto")
    set(_basilisk_compiler "clang")
endif()

if(_basilisk_compiler STREQUAL "llvm")
    _basilisk_detect_darwin_processor(_basilisk_host_processor "${CMAKE_HOST_SYSTEM_NAME}" "${CMAKE_HOST_SYSTEM_PROCESSOR}")
    if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin" OR NOT _basilisk_host_processor MATCHES "^(arm64|aarch64)$")
        message(FATAL_ERROR
            "COMP=llvm is only supported on Apple Silicon macOS. "
            "Detected host: ${CMAKE_HOST_SYSTEM_NAME}/${_basilisk_host_processor}"
        )
    endif()
    set(_basilisk_llvm_prefix "/opt/homebrew/opt/llvm")
    if(NOT EXISTS "${_basilisk_llvm_prefix}/bin/clang++")
        message(FATAL_ERROR "Homebrew LLVM was not found. Install it with: brew install llvm")
    endif()
    set(CMAKE_C_COMPILER "${_basilisk_llvm_prefix}/bin/clang" CACHE FILEPATH "C compiler" FORCE)
    set(CMAKE_CXX_COMPILER "${_basilisk_llvm_prefix}/bin/clang++" CACHE FILEPATH "C++ compiler" FORCE)
    set(CMAKE_AR "${_basilisk_llvm_prefix}/bin/llvm-ar" CACHE FILEPATH "Archive tool" FORCE)
    set(CMAKE_RANLIB "${_basilisk_llvm_prefix}/bin/llvm-ranlib" CACHE FILEPATH "Archive index tool" FORCE)
elseif(_basilisk_compiler STREQUAL "clang")
    set(CMAKE_C_COMPILER "clang" CACHE FILEPATH "C compiler" FORCE)
    set(CMAKE_CXX_COMPILER "clang++" CACHE FILEPATH "C++ compiler" FORCE)
elseif(_basilisk_compiler STREQUAL "gcc")
    set(CMAKE_C_COMPILER "gcc" CACHE FILEPATH "C compiler" FORCE)
    set(CMAKE_CXX_COMPILER "g++" CACHE FILEPATH "C++ compiler" FORCE)
else()
    message(FATAL_ERROR "Unsupported COMP='${COMP}'. Use auto, clang, gcc, or llvm.")
endif()

set(COMP "${_basilisk_compiler}" CACHE STRING "Compiler selection: auto, clang, gcc, or llvm" FORCE)

project(basilisk VERSION 1.7.0 LANGUAGES CXX)

_basilisk_detect_darwin_processor(_basilisk_system_processor "${CMAKE_SYSTEM_NAME}" "${CMAKE_SYSTEM_PROCESSOR}")
if(APPLE AND NOT _basilisk_system_processor MATCHES "^(arm64|aarch64)$")
    message(FATAL_ERROR
        "Basilisk macOS builds support Apple Silicon only; Intel macOS is not supported. "
        "Detected target: ${CMAKE_SYSTEM_NAME}/${_basilisk_system_processor}"
    )
endif()

message(STATUS "Basilisk compiler selection: ${COMP}")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(BASILISK_DIST_TAG "v${PROJECT_VERSION}" CACHE STRING "Release asset tag used in dist binary names" FORCE)

function(_basilisk_detect_dist_os out_var)
    if(WIN32)
        set(_os "windows")
    elseif(APPLE)
        set(_os "macos")
    else()
        set(_os "linux")
    endif()
    set("${out_var}" "${_os}" PARENT_SCOPE)
endfunction()

function(_basilisk_detect_dist_arch out_var)
    string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" _processor)
    if(_processor MATCHES "^(x86_64|amd64)$")
        set(_arch "x86_64")
    elseif(_processor MATCHES "^(aarch64|arm64)$")
        set(_arch "aarch64")
    else()
        set(_arch "${_processor}")
    endif()
    set("${out_var}" "${_arch}" PARENT_SCOPE)
endfunction()

function(_basilisk_dist_asset_name out_var pgo_suffix)
    _basilisk_detect_dist_os(_dist_os)
    _basilisk_detect_dist_arch(_dist_arch)
    set(_dist_cpu_suffix "")
    if(USE_AVX2)
        set(_dist_cpu_suffix "-avx2")
    elseif(USE_PEXT)
        set(_dist_cpu_suffix "-pext")
    endif()
    set("${out_var}"
        "basilisk-${BASILISK_DIST_TAG}-${_dist_os}-${_dist_arch}${_dist_cpu_suffix}${pgo_suffix}${CMAKE_EXECUTABLE_SUFFIX}"
        PARENT_SCOPE
    )
endfunction()

# ---------------------------------------------------------------------------
# Internal libraries
# ---------------------------------------------------------------------------

# Fathom Syzygy tablebase probing library (MIT). tbprobe.c includes tbchess.c.
set_source_files_properties(external/fathom/src/tbprobe.c PROPERTIES LANGUAGE CXX)
add_library(fathom STATIC
    external/fathom/src/tbprobe.c
)
target_include_directories(fathom PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/external/fathom/src
)
target_compile_definitions(fathom PRIVATE TB_NO_HELPER_API)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(fathom PRIVATE -w)
elseif(MSVC)
    target_compile_options(fathom PRIVATE /w)
endif()

# Core board library — shared by the engine, tests, and benchmarks
set(BOARD_SOURCES
    src/Board.cpp
    src/bitboard.cpp
    src/move.cpp
    src/attacks.cpp
    src/zobrist.cpp
)
add_library(basilisk_board STATIC ${BOARD_SOURCES})
target_include_directories(basilisk_board PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Engine library — evaluation and search (depends on board)
add_library(basilisk_engine STATIC
    src/eval.cpp
    src/search.cpp
    src/syzygy.cpp
)
target_link_libraries(basilisk_engine PUBLIC basilisk_board PRIVATE fathom)

# ---------------------------------------------------------------------------
# Executables
# ---------------------------------------------------------------------------
add_executable(basilisk
    src/Basilisk.cpp
    src/bench.cpp
    src/Engine.cpp
    src/Parameters.cpp
    src/UciProtocol.cpp
)
target_link_libraries(basilisk PRIVATE basilisk_engine)

# ---------------------------------------------------------------------------
# Board performance benchmark
# ---------------------------------------------------------------------------
add_executable(board_performance_test tests/board_performance.cpp)
target_link_libraries(board_performance_test PRIVATE basilisk_board)

# ---------------------------------------------------------------------------
# Board correctness tests
# ---------------------------------------------------------------------------
add_executable(test_board tests/test_board.cpp)
target_link_libraries(test_board PRIVATE basilisk_board)

# ---------------------------------------------------------------------------
# Move encoding / decoding tests
# ---------------------------------------------------------------------------
add_executable(test_move tests/test_move.cpp)
target_link_libraries(test_move PRIVATE basilisk_board)

# ---------------------------------------------------------------------------
# Transposition table tests (TT is header-only)
# ---------------------------------------------------------------------------
add_executable(test_tt tests/test_tt.cpp)
target_include_directories(test_tt PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# ---------------------------------------------------------------------------
# Evaluator tests
# ---------------------------------------------------------------------------
add_executable(test_eval tests/test_eval.cpp)
target_link_libraries(test_eval PRIVATE basilisk_engine)

# ---------------------------------------------------------------------------
# Search tests
# ---------------------------------------------------------------------------
add_executable(test_search tests/test_search.cpp src/Parameters.cpp)
target_link_libraries(test_search PRIVATE basilisk_engine)

# ---------------------------------------------------------------------------
# Engine-level ponder protocol tests
# ---------------------------------------------------------------------------
add_executable(test_engine_ponder
    tests/test_engine_ponder.cpp
    src/Engine.cpp
    src/Parameters.cpp
    src/bench.cpp
)
target_link_libraries(test_engine_ponder PRIVATE basilisk_engine)

# ---------------------------------------------------------------------------
# Engine-level threading protocol tests
# ---------------------------------------------------------------------------
add_executable(test_engine_threading
    tests/test_engine_threading.cpp
    src/Engine.cpp
    src/Parameters.cpp
    src/bench.cpp
)
target_link_libraries(test_engine_threading PRIVATE basilisk_engine)

# ---------------------------------------------------------------------------
# UCI protocol tests
# ---------------------------------------------------------------------------
add_executable(test_uci_protocol
    tests/test_uci_protocol.cpp
    src/UciProtocol.cpp
    src/Parameters.cpp
)
target_link_libraries(test_uci_protocol PRIVATE basilisk_engine)

# ---------------------------------------------------------------------------
# Endgame regression suite (Step 3.5: scale factors + known-endgame knowledge)
# ---------------------------------------------------------------------------
add_executable(test_endgames tests/test_endgames.cpp src/Parameters.cpp)
target_link_libraries(test_endgames PRIVATE basilisk_engine)

include(CTest)
add_test(NAME test_board        COMMAND test_board)
add_test(NAME test_move         COMMAND test_move)
add_test(NAME test_tt           COMMAND test_tt)
add_test(NAME test_eval         COMMAND test_eval)
add_test(NAME test_search       COMMAND test_search)
add_test(NAME test_engine_ponder COMMAND test_engine_ponder)
add_test(NAME test_engine_threading COMMAND test_engine_threading)
add_test(NAME test_uci_protocol COMMAND test_uci_protocol)
add_test(NAME test_endgames     COMMAND test_endgames
         ${CMAKE_CURRENT_SOURCE_DIR}/tests/endgames.epd)

# board_performance_test is a standalone benchmark — build it but do not
# register it with CTest so that a plain `ctest` run stays fast and clean.
# Run it manually:  ./build/release/board_performance_test

# ---------------------------------------------------------------------------
# Compiler flags (applied to all targets)
# ---------------------------------------------------------------------------
option(PORTABLE_BUILD "Build without host-specific CPU flags for distributable binaries" OFF)
option(TUNE "Expose search constants as UCI spin options for SPSA tuning (sets BASILISK_TUNE)" OFF)
option(TEXEL "Build Texel trace+tuner target basilisk-texel (sets TEXEL_TRACE; implies BASILISK_TUNE)" OFF)
set(BASILISK_PGO "OFF" CACHE STRING "Profile-guided optimization phase: OFF, GENERATE, or USE")
set_property(CACHE BASILISK_PGO PROPERTY STRINGS OFF GENERATE USE)
set(BASILISK_PGO_PROFILE_FILE "" CACHE FILEPATH "LLVM/Clang .profdata file for BASILISK_PGO=USE")
set(BASILISK_PGO_PROFILE_DIR "${CMAKE_BINARY_DIR}/pgo" CACHE PATH "GCC profile directory for BASILISK_PGO=GENERATE/USE")
string(TOUPPER "${BASILISK_PGO}" _basilisk_pgo)
if(NOT _basilisk_pgo MATCHES "^(OFF|GENERATE|USE)$")
    message(FATAL_ERROR "BASILISK_PGO must be OFF, GENERATE, or USE")
endif()

# ---------------------------------------------------------------------------
# Texel trace-based eval tuner (separate target; never in _ALL_TARGETS)
# TEXEL_TRACE is defined only here — eval.cpp is recompiled with a fresh TU.
# ---------------------------------------------------------------------------
if(TEXEL)
    add_executable(basilisk-texel
        tools/texel/tuner.cpp
        src/eval.cpp
    )
    target_link_libraries(basilisk-texel PRIVATE basilisk_board)
    target_compile_definitions(basilisk-texel PRIVATE TEXEL_TRACE BASILISK_TUNE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(basilisk-texel PRIVATE
            -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wno-unused-parameter
            $<$<CONFIG:Release>:-O3>
            $<$<AND:$<CONFIG:Release>,$<NOT:$<BOOL:${PORTABLE_BUILD}>>>:-march=native>
            $<$<CONFIG:Release>:-ffast-math>
            $<$<CONFIG:Debug>:-O0 -g3>
        )
    elseif(MSVC)
        target_compile_options(basilisk-texel PRIVATE
            /W4 /WX- /utf-8
            $<$<CONFIG:Release>:/O2 /GL>
            $<$<CONFIG:Debug>:/Od /Zi>
        )
        target_link_options(basilisk-texel PRIVATE $<$<CONFIG:Release>:/LTCG>)
    endif()
    if(USE_PEXT)
        target_compile_definitions(basilisk-texel PRIVATE USE_PEXT)
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
            target_compile_options(basilisk-texel PRIVATE -mbmi2)
        elseif(MSVC)
            target_compile_options(basilisk-texel PRIVATE /arch:AVX2)
        endif()
    endif()
    message(STATUS "TEXEL build: basilisk-texel enabled (TEXEL_TRACE + BASILISK_TUNE defined)")
endif()

set(_ALL_TARGETS
    basilisk_board basilisk_engine
    basilisk board_performance_test
    test_board test_move test_tt test_eval test_search test_engine_ponder test_engine_threading test_uci_protocol
    test_endgames
)

if(TUNE)
    foreach(_tgt IN LISTS _ALL_TARGETS)
        target_compile_definitions(${_tgt} PRIVATE BASILISK_TUNE)
    endforeach()
    message(STATUS "TUNE build: search constants exposed as UCI options (BASILISK_TUNE defined)")
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    foreach(_tgt IN LISTS _ALL_TARGETS)
        target_compile_options(${_tgt} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wshadow
            -Wconversion
            -Wno-unused-parameter
            $<$<CONFIG:Release>:-O3>
            $<$<AND:$<CONFIG:Release>,$<NOT:$<BOOL:${PORTABLE_BUILD}>>>:-march=native>
            $<$<CONFIG:Release>:-ffast-math>
            $<$<CONFIG:RelWithDebInfo>:-O2 -g>
            $<$<AND:$<CONFIG:RelWithDebInfo>,$<NOT:$<BOOL:${PORTABLE_BUILD}>>>:-march=native>
            $<$<CONFIG:Debug>:-O0 -g3>
        )
        target_link_options(${_tgt} PRIVATE
            $<$<CONFIG:Debug>:-g3>
        )
    endforeach()
    if(PORTABLE_BUILD)
        message(STATUS "Portable build enabled: omitting -march=native")
    endif()
    if(SANITIZE)
        foreach(_tgt IN LISTS _ALL_TARGETS)
            target_compile_options(${_tgt} PRIVATE -fsanitize=address,undefined)
            target_link_options(${_tgt} PRIVATE -fsanitize=address,undefined)
        endforeach()
    endif()

elseif(MSVC)
    foreach(_tgt IN LISTS _ALL_TARGETS)
        target_compile_options(${_tgt} PRIVATE
            /W4
            /WX-
            /utf-8
            $<$<CONFIG:Release>:/O2 /GL>
            $<$<CONFIG:Debug>:/Od /Zi>
        )
        target_link_options(${_tgt} PRIVATE
            $<$<CONFIG:Release>:/LTCG>
        )
    endforeach()
endif()

# ---------------------------------------------------------------------------
# Link-time optimisation (Release only, non-MSVC handled above via /GL+/LTCG)
# ---------------------------------------------------------------------------
if(NOT MSVC)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT _lto_ok OUTPUT _lto_msg)
    if(_lto_ok)
        foreach(_tgt IN LISTS _ALL_TARGETS)
            set_property(TARGET ${_tgt} PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
        endforeach()
    else()
        message(STATUS "LTO not supported: ${_lto_msg}")
    endif()
endif()

# ---------------------------------------------------------------------------
# Profile-guided optimisation
# ---------------------------------------------------------------------------
function(_basilisk_apply_pgo _target)
    if(_basilisk_pgo STREQUAL "GENERATE")
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            target_compile_options(${_target} PRIVATE $<$<CONFIG:Release>:-fprofile-instr-generate>)
            target_link_options(${_target} PRIVATE $<$<CONFIG:Release>:-fprofile-instr-generate>)
        elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            target_compile_options(${_target} PRIVATE $<$<CONFIG:Release>:-fprofile-generate=${BASILISK_PGO_PROFILE_DIR}>)
            target_link_options(${_target} PRIVATE $<$<CONFIG:Release>:-fprofile-generate=${BASILISK_PGO_PROFILE_DIR}>)
        else()
            message(FATAL_ERROR "BASILISK_PGO=GENERATE is supported for Clang and GCC builds only")
        endif()

        # Instrumented LTO builds are slower and less portable across toolchains;
        # keep LTO for the final PGO-use build instead.
        set_property(TARGET ${_target} PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE FALSE)
    elseif(_basilisk_pgo STREQUAL "USE")
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            if(NOT BASILISK_PGO_PROFILE_FILE)
                message(FATAL_ERROR "BASILISK_PGO=USE with Clang requires BASILISK_PGO_PROFILE_FILE")
            endif()
            if(NOT EXISTS "${BASILISK_PGO_PROFILE_FILE}")
                message(FATAL_ERROR "PGO profile file does not exist: ${BASILISK_PGO_PROFILE_FILE}")
            endif()
            target_compile_options(${_target} PRIVATE
                $<$<CONFIG:Release>:-fprofile-instr-use=${BASILISK_PGO_PROFILE_FILE}>
                $<$<CONFIG:Release>:-Wno-profile-instr-unprofiled>
            )
            target_link_options(${_target} PRIVATE $<$<CONFIG:Release>:-fprofile-instr-use=${BASILISK_PGO_PROFILE_FILE}>)
        elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            target_compile_options(${_target} PRIVATE
                $<$<CONFIG:Release>:-fprofile-use=${BASILISK_PGO_PROFILE_DIR}>
                $<$<CONFIG:Release>:-Wno-missing-profile>
            )
            target_link_options(${_target} PRIVATE $<$<CONFIG:Release>:-fprofile-use=${BASILISK_PGO_PROFILE_DIR}>)
        else()
            message(FATAL_ERROR "BASILISK_PGO=USE is supported for Clang and GCC builds only")
        endif()
    endif()
endfunction()

if(NOT _basilisk_pgo STREQUAL "OFF")
    foreach(_tgt IN LISTS _ALL_TARGETS)
        _basilisk_apply_pgo(${_tgt})
    endforeach()
    message(STATUS "PGO phase: ${_basilisk_pgo}")
endif()

set(_basilisk_dist_dir "${CMAKE_SOURCE_DIR}/build/dist")
_basilisk_dist_asset_name(_basilisk_dist_asset_name "")
_basilisk_dist_asset_name(_basilisk_dist_pgo_asset_name "-pgo")

if(_basilisk_pgo STREQUAL "OFF")
    add_custom_command(TARGET basilisk POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${_basilisk_dist_dir}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "$<TARGET_FILE:basilisk>"
            "${_basilisk_dist_dir}/${_basilisk_dist_asset_name}"
        COMMENT "Copying Basilisk release binary to ${_basilisk_dist_dir}/${_basilisk_dist_asset_name}"
        VERBATIM
    )
endif()

if(PORTABLE_BUILD)
    set(_basilisk_pgo_portable ON)
else()
    set(_basilisk_pgo_portable OFF)
endif()
get_filename_component(_basilisk_pgo_preset "${CMAKE_BINARY_DIR}" NAME)
add_custom_target(pgo
    COMMAND "${CMAKE_COMMAND}"
        -DPGO_SOURCE_DIR=${CMAKE_SOURCE_DIR}
        -DPGO_PRESET=${_basilisk_pgo_preset}
        -DPGO_COMP=${COMP}
        -DPGO_TUNE=${TUNE}
        -DPGO_PORTABLE_BUILD=${_basilisk_pgo_portable}
        -DPGO_EXECUTABLE_SUFFIX=${CMAKE_EXECUTABLE_SUFFIX}
        -DPGO_DIST_DIR=${_basilisk_dist_dir}
        -DPGO_DIST_ASSET_NAME=${_basilisk_dist_pgo_asset_name}
        -P "${CMAKE_SOURCE_DIR}/cmake/pgo-build.cmake"
    COMMENT "Build Basilisk with profile-guided optimization"
    USES_TERMINAL
    VERBATIM
)

# ---------------------------------------------------------------------------
# Optional BMI2 PEXT support (faster sliding piece attacks on Haswell+)
# ---------------------------------------------------------------------------
option(USE_PEXT "Use BMI2 PEXT instruction for magic bitboards (requires Haswell+)" OFF)
option(USE_AVX2 "Build for x86-64 CPUs with AVX2" OFF)

if((USE_PEXT OR USE_AVX2) AND NOT CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|amd64|i.86")
    message(FATAL_ERROR "x86 CPU feature builds require an x86/x86-64 target")
endif()

if(USE_PEXT)
    foreach(_tgt IN LISTS _ALL_TARGETS)
        target_compile_definitions(${_tgt} PRIVATE USE_PEXT)
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
            target_compile_options(${_tgt} PRIVATE -mbmi2)
        elseif(MSVC)
            target_compile_options(${_tgt} PRIVATE /arch:AVX2)
        endif()
    endforeach()
endif()

if(USE_AVX2)
    foreach(_tgt IN LISTS _ALL_TARGETS)
        target_compile_definitions(${_tgt} PRIVATE USE_AVX2)
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
            target_compile_options(${_tgt} PRIVATE -mavx2 -msse4.1 -mpopcnt)
        elseif(MSVC)
            target_compile_options(${_tgt} PRIVATE /arch:AVX2)
        endif()
    endforeach()
endif()

# ---------------------------------------------------------------------------
# Static runtime (default ON on MinGW to avoid libc++.dll / libgcc_s.dll
# dependency failures when the binary is run outside the MSYS2 environment)
# ---------------------------------------------------------------------------
option(STATIC_RUNTIME "Link the C++ runtime and standard library statically" ${MINGW})
if(STATIC_RUNTIME)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        foreach(_tgt IN LISTS _ALL_TARGETS)
            target_link_options(${_tgt} PRIVATE -static)
        endforeach()
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
