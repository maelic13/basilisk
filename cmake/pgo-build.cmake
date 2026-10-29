cmake_minimum_required(VERSION 3.24)

if(NOT DEFINED PGO_SOURCE_DIR)
    message(FATAL_ERROR "PGO_SOURCE_DIR is required")
endif()
if(NOT DEFINED PGO_PRESET)
    message(FATAL_ERROR "PGO_PRESET is required")
endif()
if(NOT DEFINED PGO_NAME OR PGO_NAME STREQUAL "")
    set(PGO_NAME "${PGO_PRESET}")
endif()
if(NOT DEFINED PGO_UNIVERSAL_TIER OR PGO_UNIVERSAL_TIER STREQUAL "")
    set(PGO_UNIVERSAL_TIER "OFF")
endif()
if(NOT DEFINED PGO_ALIGN_FUNCTIONS)
    set(PGO_ALIGN_FUNCTIONS "")
endif()
if(NOT DEFINED PGO_COMP)
    set(PGO_COMP "auto")
endif()
if(NOT DEFINED PGO_COMPILER_ID)
    message(FATAL_ERROR "PGO_COMPILER_ID is required")
endif()
if(NOT DEFINED PGO_PORTABLE_BUILD)
    set(PGO_PORTABLE_BUILD "OFF")
endif()
foreach(_isa PGO_USE_PEXT PGO_USE_AVX2 PGO_USE_SSE41_POPCNT)
    if(NOT DEFINED ${_isa} OR "${${_isa}}" STREQUAL "")
        set(${_isa} "OFF")
    endif()
endforeach()
if(NOT DEFINED PGO_STATIC_RUNTIME OR PGO_STATIC_RUNTIME STREQUAL "")
    message(FATAL_ERROR "PGO_STATIC_RUNTIME is required")
endif()
if(NOT DEFINED PGO_TUNE)
    set(PGO_TUNE "OFF")
endif()
if(NOT DEFINED PGO_DIAGNOSTIC)
    set(PGO_DIAGNOSTIC "OFF")
endif()
if(NOT DEFINED PGO_ABLATION)
    set(PGO_ABLATION "OFF")
endif()
if(NOT DEFINED PGO_PROFILE_SYMBOLS)
    set(PGO_PROFILE_SYMBOLS "OFF")
endif()
if(NOT DEFINED PGO_DEV_ID)
    set(PGO_DEV_ID "")
endif()
if(NOT DEFINED PGO_ARM_OPTION)
    set(PGO_ARM_OPTION "")
endif()
if(NOT DEFINED PGO_ARM_STATE)
    set(PGO_ARM_STATE "OFF")
endif()
if(NOT DEFINED PGO_EXECUTABLE_SUFFIX)
    set(PGO_EXECUTABLE_SUFFIX "")
endif()
if(NOT DEFINED PGO_TRAIN_DEPTH)
    set(PGO_TRAIN_DEPTH "13")
endif()
if(NOT DEFINED PGO_DIST_DIR)
    set(PGO_DIST_DIR "")
endif()
foreach(_var PGO_DIST_HEADER PGO_DIST_STEM PGO_DIST_SUFFIX)
    if(NOT DEFINED ${_var})
        set(${_var} "")
    endif()
endforeach()

set(_build_root "${PGO_SOURCE_DIR}/build")
set(_gen_dir "${_build_root}/${PGO_NAME}-pgo-generate")
set(_use_dir "${_build_root}/${PGO_NAME}-pgo")
set(_prof_dir "${_build_root}/${PGO_NAME}-pgo-profile")
set(_profdata "${_prof_dir}/basilisk.profdata")
set(_train_input "${_prof_dir}/bench.in")
set(_train_log "${_prof_dir}/bench.log")

file(REMOVE_RECURSE "${_gen_dir}" "${_use_dir}" "${_prof_dir}")
file(MAKE_DIRECTORY "${_prof_dir}")
file(WRITE "${_train_input}" "bench ${PGO_TRAIN_DEPTH}\nquit\n")

set(_pgo_configure_args
    -DCOMP=${PGO_COMP}
    -DPORTABLE_BUILD=${PGO_PORTABLE_BUILD}
    -DSTATIC_RUNTIME=${PGO_STATIC_RUNTIME}
    -DTUNE=${PGO_TUNE}
    -DDIAGNOSTIC=${PGO_DIAGNOSTIC}
    -DABLATION=${PGO_ABLATION}
    -DPROFILE_SYMBOLS=${PGO_PROFILE_SYMBOLS}
    -DBASILISK_DEV_ID=${PGO_DEV_ID}
    -DBASILISK_UNIVERSAL_TIER=${PGO_UNIVERSAL_TIER}
    -DBASILISK_ALIGN_FUNCTIONS=${PGO_ALIGN_FUNCTIONS}
    -DUSE_PEXT=${PGO_USE_PEXT}
    -DUSE_AVX2=${PGO_USE_AVX2}
    -DUSE_SSE41_POPCNT=${PGO_USE_SSE41_POPCNT}
)
if(PGO_ARM_OPTION)
    if(NOT PGO_ARM_OPTION MATCHES "^[A-Za-z_][A-Za-z0-9_]*$")
        message(FATAL_ERROR "PGO_ARM_OPTION must be a CMake identifier")
    endif()
    if(NOT PGO_ARM_STATE MATCHES "^(ON|OFF)$")
        message(FATAL_ERROR "PGO_ARM_STATE must be ON or OFF")
    endif()
    list(APPEND _pgo_configure_args
        -DBASILISK_PGO_ARM_OPTION=${PGO_ARM_OPTION}
        -DBASILISK_PGO_ARM_STATE=${PGO_ARM_STATE}
        -D${PGO_ARM_OPTION}=${PGO_ARM_STATE}
    )
endif()

function(_basilisk_check_training_log _log)
    if(NOT EXISTS "${_log}")
        return()
    endif()

    file(READ "${_log}" _log_contents)
    if(_log_contents MATCHES "Invalid FEN|Invalid en-passant|Unsupported position|King can be captured|more than 8 pawns")
        message(FATAL_ERROR "PGO training produced invalid position output; see ${_log}")
    endif()
endfunction()

message(STATUS "PGO preset: ${PGO_PRESET}")
message(STATUS "PGO generate build: ${_gen_dir}")
execute_process(
    COMMAND "${CMAKE_COMMAND}" --preset "${PGO_PRESET}" -B "${_gen_dir}"
            ${_pgo_configure_args}
            -DBASILISK_PGO=GENERATE
            -DBASILISK_PGO_PROFILE_DIR=${_prof_dir}
    WORKING_DIRECTORY "${PGO_SOURCE_DIR}"
    COMMAND_ERROR_IS_FATAL ANY
)
execute_process(
    COMMAND "${CMAKE_COMMAND}" --build "${_gen_dir}" --target basilisk
    WORKING_DIRECTORY "${PGO_SOURCE_DIR}"
    COMMAND_ERROR_IS_FATAL ANY
)

set(_instrumented_binary "${_gen_dir}/basilisk${PGO_EXECUTABLE_SUFFIX}")
if(NOT EXISTS "${_instrumented_binary}")
    message(FATAL_ERROR "Instrumented binary not found: ${_instrumented_binary}")
endif()

message(STATUS "PGO training bench: depth ${PGO_TRAIN_DEPTH}")
if(PGO_COMPILER_ID MATCHES "Clang")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E env "LLVM_PROFILE_FILE=${_prof_dir}/basilisk-%p.profraw" "${_instrumented_binary}"
        INPUT_FILE "${_train_input}"
        OUTPUT_FILE "${_train_log}"
        ERROR_FILE "${_train_log}"
        WORKING_DIRECTORY "${PGO_SOURCE_DIR}"
        COMMAND_ERROR_IS_FATAL ANY
    )
else()
    # GCC writes .gcda files through -fprofile-generate's directory argument,
    # not through LLVM_PROFILE_FILE.
    execute_process(
        COMMAND "${_instrumented_binary}"
        INPUT_FILE "${_train_input}"
        OUTPUT_FILE "${_train_log}"
        ERROR_FILE "${_train_log}"
        WORKING_DIRECTORY "${PGO_SOURCE_DIR}"
        COMMAND_ERROR_IS_FATAL ANY
    )
endif()
_basilisk_check_training_log("${_train_log}")

# Training input is the 40-position `bench` suite only (depth 13). That suite is
# diverse enough on its own — openings, quiet and tactical middlegames, a broad
# range of endgames, mates, and fortresses (30 pieces down to 8) — to be a
# representative PGO profile, matching the Stockfish convention of training PGO
# straight from `bench`. The older per-position EPD loop (cmake/pgo-train.epd,
# depth 7) was dropped when bench grew from 16 to 40 positions: with a
# well-spread bench it was redundant (it mostly re-sampled the same hot search
# loop). See docs/PLAN.md / GUIDE.md.

if(PGO_COMPILER_ID STREQUAL "GNU")
    file(GLOB_RECURSE _gcda_files "${_prof_dir}/*.gcda")
    if(NOT _gcda_files)
        message(FATAL_ERROR "GCC training did not produce any .gcda files in ${_prof_dir}")
    endif()
    list(LENGTH _gcda_files _gcda_count)

    # GCC keys profile files to the object-file paths. Reusing the generation
    # build tree keeps those paths identical; reconfiguration changes the
    # compile/link commands, so Ninja rebuilds the instrumented objects with
    # -fprofile-use and the final optimized binary replaces the training one.
    message(STATUS "PGO final GCC rebuild: ${_gen_dir} (${_gcda_count} .gcda files)")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" --preset "${PGO_PRESET}" -B "${_gen_dir}"
                ${_pgo_configure_args}
                -DBASILISK_PGO=USE
                -DBASILISK_PGO_PROFILE_DIR=${_prof_dir}
        WORKING_DIRECTORY "${PGO_SOURCE_DIR}"
        COMMAND_ERROR_IS_FATAL ANY
    )
    execute_process(
        COMMAND "${CMAKE_COMMAND}" --build "${_gen_dir}" --target basilisk
        WORKING_DIRECTORY "${PGO_SOURCE_DIR}"
        COMMAND_ERROR_IS_FATAL ANY
    )
    set(_pgo_binary "${_gen_dir}/basilisk${PGO_EXECUTABLE_SUFFIX}")
elseif(PGO_COMPILER_ID MATCHES "Clang")
    file(GLOB _profraw_files "${_prof_dir}/*.profraw")
    if(NOT _profraw_files)
        message(FATAL_ERROR "No .profraw files were produced in ${_prof_dir}")
    endif()

    if(DEFINED PGO_LLVM_PROFDATA AND PGO_LLVM_PROFDATA)
        if(NOT EXISTS "${PGO_LLVM_PROFDATA}")
            message(FATAL_ERROR "Configured llvm-profdata does not exist: ${PGO_LLVM_PROFDATA}")
        endif()
        set(_profdata_command "${PGO_LLVM_PROFDATA}")
    else()
        message(FATAL_ERROR
            "PGO_LLVM_PROFDATA was not provided; configure through the pgo target "
            "so llvm-profdata matches the selected Clang toolchain"
        )
    endif()

    message(STATUS "PGO merge: ${_profdata} (using ${PGO_LLVM_PROFDATA})")
    execute_process(
        COMMAND ${_profdata_command} merge -sparse ${_profraw_files} -o "${_profdata}"
        WORKING_DIRECTORY "${PGO_SOURCE_DIR}"
        COMMAND_ERROR_IS_FATAL ANY
    )

    message(STATUS "PGO final build: ${_use_dir}")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" --preset "${PGO_PRESET}" -B "${_use_dir}"
                ${_pgo_configure_args}
                -DBASILISK_PGO=USE
                -DBASILISK_PGO_PROFILE_FILE=${_profdata}
        WORKING_DIRECTORY "${PGO_SOURCE_DIR}"
        COMMAND_ERROR_IS_FATAL ANY
    )
    execute_process(
        COMMAND "${CMAKE_COMMAND}" --build "${_use_dir}" --target basilisk
        WORKING_DIRECTORY "${PGO_SOURCE_DIR}"
        COMMAND_ERROR_IS_FATAL ANY
    )
    set(_pgo_binary "${_use_dir}/basilisk${PGO_EXECUTABLE_SUFFIX}")
else()
    message(FATAL_ERROR "Unsupported PGO compiler id: ${PGO_COMPILER_ID}")
endif()

message(STATUS "PGO binary: ${_pgo_binary}")

if(PGO_DIST_DIR)
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -DSRC_DIR=${PGO_SOURCE_DIR} -DHEADER=${PGO_DIST_HEADER}
                -DBINARY=${_pgo_binary} -DDIST_DIR=${PGO_DIST_DIR} -DSTEM=${PGO_DIST_STEM}
                -DSUFFIX=${PGO_DIST_SUFFIX} -DEXT=${PGO_EXECUTABLE_SUFFIX}
                -P "${PGO_SOURCE_DIR}/cmake/dist-copy.cmake"
        COMMAND_ERROR_IS_FATAL ANY
    )
endif()
