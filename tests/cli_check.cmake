# Runs the engine with command-line arguments and checks its exit status and
# output: cmake -DEXE=<engine> -DARGS=<a;b;c> -DEXPECT_CODE=<n> -DREGEX=<re>
#              [-DFORBID=<re>] -P cli_check.cmake
execute_process(
    COMMAND "${EXE}" ${ARGS}
    RESULT_VARIABLE code
    OUTPUT_VARIABLE out
    ERROR_VARIABLE err
    TIMEOUT 120)
if(NOT code STREQUAL "${EXPECT_CODE}")
    message(FATAL_ERROR "exit status ${code}, expected ${EXPECT_CODE}\n${out}${err}")
endif()
if(NOT out MATCHES "${REGEX}")
    message(FATAL_ERROR "output does not match '${REGEX}':\n${out}")
endif()
if(DEFINED FORBID AND out MATCHES "${FORBID}")
    message(FATAL_ERROR "output matches the forbidden '${FORBID}':\n${out}")
endif()
