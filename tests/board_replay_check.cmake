# Records one search's board trace and replays it (a BASILISK_BOARD_TRACE
# tree): every recorded result must be reproduced, and a planted wrong answer
# of each kind must fail verification at exactly the planted record.
#   cmake -DENGINE=<basilisk> -DREPLAY=<board_replay> -DWORK=<dir> -P board_replay_check.cmake
set(trace "${WORK}/board_replay_check.trace")
set(commands "${WORK}/board_replay_check.uci")
file(REMOVE "${trace}" "${trace}.fens")
# The second bench root (Kiwipete: castling both ways, en passant, promotions
# in reach) at a node cap.
file(WRITE "${commands}"
     "setoption name Threads value 1\n"
     "position fen r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1\n"
     "go nodes 20000\n"
     "quit\n")
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env "BASILISK_BOARD_TRACE_FILE=${trace}" "${ENGINE}"
    INPUT_FILE "${commands}"
    RESULT_VARIABLE code
    OUTPUT_VARIABLE out
    ERROR_VARIABLE err
    TIMEOUT 120)
if(NOT code STREQUAL "0" OR NOT out MATCHES "bestmove")
    message(FATAL_ERROR "recording search: exit ${code}\n${out}${err}")
endif()
if(NOT EXISTS "${trace}" OR NOT EXISTS "${trace}.fens")
    message(FATAL_ERROR "the engine wrote no trace to ${trace}")
endif()

execute_process(COMMAND "${REPLAY}" "${trace}" --verify
                RESULT_VARIABLE code OUTPUT_VARIABLE out ERROR_VARIABLE err TIMEOUT 120)
if(NOT code STREQUAL "0" OR NOT out MATCHES "verify: PASS")
    message(FATAL_ERROR "verify on the recorded trace: exit ${code}\n${out}${err}")
endif()
message(STATUS "${out}")

foreach(op MAKE GEN_CAPS GEN_QUIETS SEE GIVES_CHECK CHECK_SQUARES IS_LEGAL IS_DRAW_PLY
           UPCOMING_REPETITION IN_CHECK HAS_NPM)
    execute_process(COMMAND "${REPLAY}" "${trace}" --verify --plant ${op}
                    RESULT_VARIABLE code OUTPUT_VARIABLE out ERROR_VARIABLE err TIMEOUT 120)
    if(NOT out MATCHES "planted: a wrong ${op} result at record ([0-9]+)")
        message(FATAL_ERROR "planting ${op}: exit ${code}\n${out}${err}")
    endif()
    set(at "${CMAKE_MATCH_1}")
    if(NOT code STREQUAL "1" OR NOT out MATCHES "verify: FAIL at record ${at} \\(op ${op}\\)")
        message(FATAL_ERROR "a wrong ${op} result at record ${at} was not caught there: exit ${code}\n${out}${err}")
    endif()
    message(STATUS "planted ${op} at record ${at}: caught")
endforeach()
file(REMOVE "${trace}" "${trace}.fens" "${commands}")
