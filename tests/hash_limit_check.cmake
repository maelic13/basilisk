# The engine as a process: the Hash maximum `uci` advertises is the limit the
# option enforces. One more than it is refused when the option is parsed, with
# exactly one notice and nothing allocated, and the engine keeps answering.
# cmake -DEXE=<engine> -DWORK=<scratch dir> -P hash_limit_check.cmake
function(run_engine input out_var)
    file(WRITE "${WORK}/hash_limit_check.in" "${input}")
    execute_process(
        COMMAND "${EXE}"
        INPUT_FILE "${WORK}/hash_limit_check.in"
        RESULT_VARIABLE code
        OUTPUT_VARIABLE out
        ERROR_VARIABLE err
        TIMEOUT 60)
    if(NOT code STREQUAL "0")
        message(FATAL_ERROR "exit status ${code}, expected 0\n${out}${err}")
    endif()
    set(${out_var} "${out}" PARENT_SCOPE)
endfunction()

run_engine("uci\nquit\n" listing)
if(NOT listing MATCHES "option name Hash type spin default 64 min 1 max ([0-9]+)\n")
    message(FATAL_ERROR "no Hash option with a maximum in:\n${listing}")
endif()
set(max "${CMAKE_MATCH_1}")
math(EXPR above "${max} + 1")

# isready makes the engine apply the setting: a Hash that had been accepted
# would be allocated here.
run_engine("uci\nsetoption name Hash value ${above}\nisready\nquit\n" out)
string(REGEX MATCHALL "info string Hash[^\n]*" notices "${out}")
list(LENGTH notices count)
set(wanted "info string Hash value ${above} is above the ${max} MiB this machine can hold; the setting is unchanged.")
if(NOT count EQUAL 1)
    message(FATAL_ERROR "expected exactly one Hash notice, got ${count}:\n${out}")
endif()
list(GET notices 0 notice)
if(NOT notice STREQUAL wanted)
    message(FATAL_ERROR "expected '${wanted}', got '${notice}'")
endif()
if(NOT out MATCHES "\nreadyok\n")
    message(FATAL_ERROR "no readyok after the refused setoption:\n${out}")
endif()
