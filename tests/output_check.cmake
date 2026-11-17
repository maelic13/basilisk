# Runs a program and compares its exit status and its whole standard output
# with a golden file: cmake -DEXE=<program> -DARGS=<a;b;c> -DEXPECT_CODE=<n>
#                           -DGOLDEN=<file> -P output_check.cmake
# A deliberate change to the output updates the golden file in its commit.
execute_process(
    COMMAND "${EXE}" ${ARGS}
    RESULT_VARIABLE code
    OUTPUT_VARIABLE out
    ERROR_VARIABLE err
    TIMEOUT 600)
if(NOT code STREQUAL "${EXPECT_CODE}")
    message(FATAL_ERROR "exit status ${code}, expected ${EXPECT_CODE}\n${out}${err}")
endif()
file(READ "${GOLDEN}" want)
string(REPLACE "\r\n" "\n" want "${want}")
string(REPLACE "\r\n" "\n" out "${out}")
if(NOT out STREQUAL want)
    message(FATAL_ERROR "the output differs from ${GOLDEN}:\n${out}")
endif()
