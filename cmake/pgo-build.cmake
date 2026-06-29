cmake_minimum_required(VERSION 3.24)

if(NOT DEFINED PGO_SOURCE_DIR)
    message(FATAL_ERROR "PGO_SOURCE_DIR is required")
endif()
if(NOT DEFINED PGO_PRESET)
    message(FATAL_ERROR "PGO_PRESET is required")
endif()
if(NOT DEFINED PGO_COMP)
    set(PGO_COMP "auto")
endif()
if(NOT DEFINED PGO_PORTABLE_BUILD)
    set(PGO_PORTABLE_BUILD "OFF")
endif()
if(NOT DEFINED PGO_TUNE)
    set(PGO_TUNE "OFF")
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
if(NOT DEFINED PGO_DIST_ASSET_NAME)
    set(PGO_DIST_ASSET_NAME "")
endif()

set(_build_root "${PGO_SOURCE_DIR}/build")
set(_gen_dir "${_build_root}/${PGO_PRESET}-pgo-generate")
set(_use_dir "${_build_root}/${PGO_PRESET}-pgo")
set(_prof_dir "${_build_root}/${PGO_PRESET}-pgo-profile")
set(_profdata "${_prof_dir}/basilisk.profdata")
set(_train_input "${_prof_dir}/bench.in")
set(_train_log "${_prof_dir}/bench.log")

file(REMOVE_RECURSE "${_gen_dir}" "${_use_dir}" "${_prof_dir}")
file(MAKE_DIRECTORY "${_prof_dir}")
file(WRITE "${_train_input}" "bench ${PGO_TRAIN_DEPTH}\nquit\n")

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
            -DCOMP=${PGO_COMP}
            -DPORTABLE_BUILD=${PGO_PORTABLE_BUILD}
            -DTUNE=${PGO_TUNE}
            -DBASILISK_PGO=GENERATE
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
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env "LLVM_PROFILE_FILE=${_prof_dir}/basilisk-%p.profraw" "${_instrumented_binary}"
    INPUT_FILE "${_train_input}"
    OUTPUT_FILE "${_train_log}"
    ERROR_FILE "${_train_log}"
    WORKING_DIRECTORY "${PGO_SOURCE_DIR}"
    COMMAND_ERROR_IS_FATAL ANY
)
_basilisk_check_training_log("${_train_log}")

# Training input is the 40-position `bench` suite only (depth 13). That suite is
# diverse enough on its own — openings, quiet and tactical middlegames, a broad
# range of endgames, mates, and fortresses (30 pieces down to 8) — to be a
# representative PGO profile, matching the Stockfish convention of training PGO
# straight from `bench`. The older per-position EPD loop (cmake/pgo-train.epd,
# depth 7) was dropped when bench grew from 16 to 40 positions: with a
# well-spread bench it was redundant (it mostly re-sampled the same hot search
# loop). See PLAN.md / user_dev_guide.md.

file(GLOB _profraw_files "${_prof_dir}/*.profraw")
if(NOT _profraw_files)
    message(FATAL_ERROR "No .profraw files were produced in ${_prof_dir}")
endif()

find_program(_llvm_profdata llvm-profdata)
if(_llvm_profdata)
    set(_profdata_command "${_llvm_profdata}")
else()
    find_program(_xcrun xcrun)
    if(_xcrun)
        set(_profdata_command "${_xcrun}" llvm-profdata)
    else()
        message(FATAL_ERROR "llvm-profdata was not found")
    endif()
endif()

message(STATUS "PGO merge: ${_profdata}")
execute_process(
    COMMAND ${_profdata_command} merge -sparse ${_profraw_files} -o "${_profdata}"
    WORKING_DIRECTORY "${PGO_SOURCE_DIR}"
    COMMAND_ERROR_IS_FATAL ANY
)

message(STATUS "PGO final build: ${_use_dir}")
execute_process(
    COMMAND "${CMAKE_COMMAND}" --preset "${PGO_PRESET}" -B "${_use_dir}"
            -DCOMP=${PGO_COMP}
            -DPORTABLE_BUILD=${PGO_PORTABLE_BUILD}
            -DTUNE=${PGO_TUNE}
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
message(STATUS "PGO binary: ${_pgo_binary}")

if(PGO_DIST_DIR AND PGO_DIST_ASSET_NAME)
    set(_pgo_dist_binary "${PGO_DIST_DIR}/${PGO_DIST_ASSET_NAME}")
    file(MAKE_DIRECTORY "${PGO_DIST_DIR}")
    file(COPY_FILE "${_pgo_binary}" "${_pgo_dist_binary}" ONLY_IF_DIFFERENT)
    message(STATUS "PGO dist binary: ${_pgo_dist_binary}")
endif()
