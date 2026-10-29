cmake_minimum_required(VERSION 3.24)

# Builds the universal binary's three tiers and writes the list of their
# renamed objects for the final link (CMakeLists.txt, BASILISK_UNIVERSAL).
#
# Each tier is the ordinary PGO build of its preset, configured with
# BASILISK_UNIVERSAL_TIER so that its engine link keeps the objects LTO
# produced. Every symbol a tier's objects define is renamed under the tier's
# name (__basilisk_<tier>_<symbol>), and run_engine becomes
# basilisk_<tier>::run_engine, the dispatcher's entry point. Left shared are
# only type identities (_ZTI, _ZTS), which exception matching compares, and
# section-named data (.refptr pointers): data whose copies are identical.
# Without the rename the linker would keep one tier's copy of each shared
# template (std::string, std::stringstream) for every tier and say nothing.

foreach(_var UT_SOURCE_DIR UT_NAME UT_TIERS_DIR UT_NM UT_OBJCOPY UT_OBJDUMP UT_STATIC_RUNTIME)
    if(NOT DEFINED ${_var} OR "${${_var}}" STREQUAL "")
        message(FATAL_ERROR "${_var} is required")
    endif()
endforeach()
if(NOT DEFINED UT_COMP)
    set(UT_COMP "clang")
endif()
if(NOT DEFINED UT_DEV_ID)
    set(UT_DEV_ID "")
endif()
if(NOT DEFINED UT_ALIGN_FUNCTIONS)
    set(UT_ALIGN_FUNCTIONS "")
endif()

set(_tiers portable avx2 pext)
set(_preset_portable release)
set(_preset_avx2 release-avx2)
set(_preset_pext release-pext)

file(REMOVE_RECURSE "${UT_TIERS_DIR}")
file(MAKE_DIRECTORY "${UT_TIERS_DIR}")
set(_rsp_lines "")

foreach(_tier IN LISTS _tiers)
    set(_preset "${_preset_${_tier}}")
    set(_dir "${UT_SOURCE_DIR}/build/${UT_NAME}-${_tier}")
    set(_use_dir "${_dir}-pgo")
    message(STATUS "Universal tier ${_tier}: ${_preset} PGO in ${_dir}")
    file(REMOVE_RECURSE "${_dir}")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" --preset "${_preset}" -B "${_dir}"
                -DCOMP=${UT_COMP}
                -DPORTABLE_BUILD=ON
                -DSTATIC_RUNTIME=${UT_STATIC_RUNTIME}
                -DBASILISK_UNIVERSAL_TIER=ON
                -DBASILISK_PGO_PRESET=${_preset}
                -DBASILISK_DEV_ID=${UT_DEV_ID}
                -DBASILISK_ALIGN_FUNCTIONS=${UT_ALIGN_FUNCTIONS}
        WORKING_DIRECTORY "${UT_SOURCE_DIR}"
        COMMAND_ERROR_IS_FATAL ANY
    )
    execute_process(
        COMMAND "${CMAKE_COMMAND}" --build "${_dir}" --target pgo
        WORKING_DIRECTORY "${UT_SOURCE_DIR}"
        COMMAND_ERROR_IS_FATAL ANY
    )

    if(CMAKE_HOST_WIN32)
        file(GLOB_RECURSE _objects "${_use_dir}/*.exe.lto*.obj")
        set(_ext obj)
    else()
        file(GLOB _objects "${_use_dir}/basilisk.lto.o*")
        set(_ext o)
    endif()
    list(LENGTH _objects _object_count)
    if(_object_count EQUAL 0)
        message(FATAL_ERROR "Tier ${_tier}: the engine link in ${_use_dir} kept no LTO objects")
    endif()

    execute_process(
        COMMAND "${UT_NM}" --defined-only ${_objects}
        OUTPUT_VARIABLE _nm
        RESULT_VARIABLE _nm_result
    )
    if(NOT _nm_result EQUAL 0)
        message(FATAL_ERROR "Tier ${_tier}: ${UT_NM} failed")
    endif()
    string(REGEX MATCHALL "[^\n]+" _nm_lines "${_nm}")
    set(_prefix "__basilisk_${_tier}_")
    string(LENGTH "basilisk_${_tier}" _ns_length)
    set(_entry "_ZN${_ns_length}basilisk_${_tier}10run_engineEiPPc")
    set(_renames "")
    set(_entry_found FALSE)
    foreach(_line IN LISTS _nm_lines)
        # "<value> <type> <name>"; absolute symbols (@feat.00) are not renamed.
        if(NOT _line MATCHES "^[0-9a-fA-F]+ ([A-Za-z]) (.+)$")
            continue()
        endif()
        set(_type "${CMAKE_MATCH_1}")
        set(_name "${CMAKE_MATCH_2}")
        if(_type MATCHES "^[aA]$" OR _name MATCHES "^\\." OR _name MATCHES "^_ZT[IS]")
            continue()
        endif()
        if(_name STREQUAL "_Z10run_engineiPPc")
            set(_entry_found TRUE)
            list(APPEND _renames "${_name} ${_entry}")
        else()
            list(APPEND _renames "${_name} ${_prefix}${_name}")
        endif()
    endforeach()
    if(NOT _entry_found)
        message(FATAL_ERROR "Tier ${_tier}: no object defines run_engine(int, char**)")
    endif()
    list(REMOVE_DUPLICATES _renames)
    list(LENGTH _renames _rename_count)
    set(_map "${UT_TIERS_DIR}/${_tier}.renames")
    list(JOIN _renames "\n" _map_text)
    file(WRITE "${_map}" "${_map_text}\n")

    set(_tier_dir "${UT_TIERS_DIR}/${_tier}")
    file(MAKE_DIRECTORY "${_tier_dir}")
    set(_renamed "")
    set(_index 0)
    foreach(_object IN LISTS _objects)
        set(_out "${_tier_dir}/${_index}.${_ext}")
        execute_process(
            COMMAND "${UT_OBJCOPY}" "--redefine-syms=${_map}" "${_object}" "${_out}"
            COMMAND_ERROR_IS_FATAL ANY
        )
        list(APPEND _renamed "${_out}")
        math(EXPR _index "${_index} + 1")
    endforeach()

    # What the tier shares after the rename: its entry point, type identities
    # and section-named data. Anything else global would be resolved by link
    # order across the tiers.
    execute_process(
        COMMAND "${UT_NM}" --defined-only --extern-only ${_renamed}
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
                message(FATAL_ERROR "Tier ${_tier}: ${_name} is still global after the rename")
            endif()
        endif()
    endforeach()

    # An initialiser in a tier would run in every tier, before the dispatcher
    # has chosen one, and could execute that tier's instructions on any CPU.
    execute_process(
        COMMAND "${UT_OBJDUMP}" -h ${_renamed}
        OUTPUT_VARIABLE _sections
        COMMAND_ERROR_IS_FATAL ANY
    )
    if(_sections MATCHES "[ \t](\\.ctors|\\.dtors|\\.init_array|\\.fini_array|\\.preinit_array|\\.CRT\\$X[CIL][A-Za-z]*)[ \t\n]")
        message(FATAL_ERROR "Tier ${_tier}: an object carries ${CMAKE_MATCH_1}, a static initialiser")
    endif()

    message(STATUS "Universal tier ${_tier}: ${_object_count} objects, ${_rename_count} symbols renamed")
    foreach(_object IN LISTS _renamed)
        list(APPEND _rsp_lines "\"${_object}\"")
    endforeach()
endforeach()

list(JOIN _rsp_lines "\n" _rsp_text)
file(WRITE "${UT_TIERS_DIR}/objects.rsp" "${_rsp_text}\n")
