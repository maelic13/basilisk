cmake_minimum_required(VERSION 3.24)

# The checks on one universal tier's renamed objects (cmake/universal-tiers.cmake
# includes this file). Run on its own it checks a list of objects, so a planted
# object can be shown to fail:
#   cmake -DUCT_TIER=pext -DUCT_NM=llvm-nm -DUCT_OBJDUMP=llvm-objdump
#         -DUCT_OBJECTS=a.obj;b.obj -P cmake/universal-check-tier.cmake

# What a tier may share after the rename: its entry point, type identities
# (_ZTI, _ZTS, which exception matching compares) and section-named data
# (.refptr pointers), whose copies are identical. Anything else global would
# be resolved by link order across the tiers, silently. A static initialiser
# would run in every tier before the dispatcher has chosen one, and could
# execute that tier's instructions on any CPU.
function(basilisk_check_tier_objects tier nm objdump)
    set(_objects ${ARGN})
    set(_prefix "__basilisk_${tier}_")
    string(LENGTH "basilisk_${tier}" _ns_length)
    set(_entry "_ZN${_ns_length}basilisk_${tier}10run_engineEiPPc")
    execute_process(
        COMMAND "${nm}" --defined-only --extern-only ${_objects}
        OUTPUT_VARIABLE _nm
        COMMAND_ERROR_IS_FATAL ANY
    )
    string(REGEX MATCHALL "[^\n]+" _nm_lines "${_nm}")
    foreach(_line IN LISTS _nm_lines)
        if(_line MATCHES "^[0-9a-fA-F]+ ([A-Za-z]) (.+)$")
            set(_type "${CMAKE_MATCH_1}")
            set(_name "${CMAKE_MATCH_2}")
            if(NOT (_type MATCHES "^[aA]$" OR _name MATCHES "^\\." OR _name MATCHES "^_ZT[IS]"
                    OR _name STREQUAL _entry OR _name MATCHES "^${_prefix}"))
                message(FATAL_ERROR "Tier ${tier}: ${_name} is still global after the rename")
            endif()
        endif()
    endforeach()

    execute_process(
        COMMAND "${objdump}" -h ${_objects}
        OUTPUT_VARIABLE _sections
        COMMAND_ERROR_IS_FATAL ANY
    )
    if(_sections MATCHES "[ \t](\\.ctors|\\.dtors|\\.init_array|\\.fini_array|\\.preinit_array|\\.CRT\\$X[CIL][A-Za-z]*)[ \t\n]")
        message(FATAL_ERROR "Tier ${tier}: an object carries ${CMAKE_MATCH_1}, a static initialiser")
    endif()
endfunction()

if(CMAKE_SCRIPT_MODE_FILE STREQUAL CMAKE_CURRENT_LIST_FILE AND DEFINED UCT_TIER)
    basilisk_check_tier_objects("${UCT_TIER}" "${UCT_NM}" "${UCT_OBJDUMP}" ${UCT_OBJECTS})
    message(STATUS "Tier ${UCT_TIER}: objects pass")
endif()
