# Runs a program that must not succeed (a negative control that ends in a
# crash or an error), and fails only if it exits 0:
#   cmake -DEXE=<program> -DARGS=<a;b;c> -P expect_failure.cmake
execute_process(
    COMMAND "${EXE}" ${ARGS}
    RESULT_VARIABLE code
    OUTPUT_VARIABLE out
    ERROR_VARIABLE err
    TIMEOUT 120)
if(code STREQUAL "0")
    message(FATAL_ERROR "the control succeeded, so it controls nothing:\n${out}${err}")
endif()
message(STATUS "the control failed as it must: ${code}")
