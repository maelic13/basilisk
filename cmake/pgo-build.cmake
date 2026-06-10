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
if(NOT DEFINED PGO_POSITION_DEPTH)
    set(PGO_POSITION_DEPTH "7")
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
set(_train_epd "${PGO_SOURCE_DIR}/cmake/pgo-train.epd")

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

if(EXISTS "${_train_epd}")
    file(STRINGS "${_train_epd}" _epd_lines)
    set(_epd_total 0)
    foreach(_epd_line IN LISTS _epd_lines)
        string(STRIP "${_epd_line}" _epd_line)
        if(_epd_line STREQUAL "" OR _epd_line MATCHES "^#")
            continue()
        endif()
        math(EXPR _epd_total "${_epd_total} + 1")
    endforeach()

    message(STATUS "PGO training positions: ${_epd_total} positions at depth ${PGO_POSITION_DEPTH}")
    set(_epd_index 0)
    foreach(_epd_line IN LISTS _epd_lines)
        string(STRIP "${_epd_line}" _epd_line)
        if(_epd_line STREQUAL "" OR _epd_line MATCHES "^#")
            continue()
        endif()
        if(NOT _epd_line MATCHES "^([^ ]+) ([bw]) ([^ ]+) ([^ ;]+);?")
            message(FATAL_ERROR "Invalid PGO EPD line: ${_epd_line}")
        endif()

        math(EXPR _epd_index "${_epd_index} + 1")
        set(_fen "${CMAKE_MATCH_1} ${CMAKE_MATCH_2} ${CMAKE_MATCH_3} ${CMAKE_MATCH_4} 0 1")
        set(_position_input "${_prof_dir}/position-${_epd_index}.in")
        set(_position_log "${_prof_dir}/position-${_epd_index}.log")
        file(WRITE "${_position_input}" "position fen ${_fen}\ngo depth ${PGO_POSITION_DEPTH}\nquit\n")

        message(STATUS "PGO training position ${_epd_index}/${_epd_total}: depth ${PGO_POSITION_DEPTH}")
        execute_process(
            COMMAND "${CMAKE_COMMAND}" -E env "LLVM_PROFILE_FILE=${_prof_dir}/position-${_epd_index}-%p.profraw" "${_instrumented_binary}"
            INPUT_FILE "${_position_input}"
            OUTPUT_FILE "${_position_log}"
            ERROR_FILE "${_position_log}"
            WORKING_DIRECTORY "${PGO_SOURCE_DIR}"
            COMMAND_ERROR_IS_FATAL ANY
        )
        _basilisk_check_training_log("${_position_log}")
    endforeach()
endif()

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
