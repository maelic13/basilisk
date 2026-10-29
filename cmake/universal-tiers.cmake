cmake_minimum_required(VERSION 3.24)

# Builds the universal binary's four tiers and writes the list of their
# renamed objects for the final link (CMakeLists.txt, BASILISK_UNIVERSAL).
#
# Each tier is the ordinary PGO build of the release preset with its
# instruction-set option, configured with BASILISK_UNIVERSAL_TIER so that its
# engine link keeps what LTO produced. Every symbol a tier defines is renamed
# under the tier's name (__basilisk_<tier>_<symbol>), and run_engine becomes
# basilisk_<tier>::run_engine, the dispatcher's entry point. Left shared are
# only type identities (_ZTI, _ZTS), which exception matching compares, and
# section-named data (.refptr pointers): data whose copies are identical.
# Without the rename the linker would keep one tier's copy of each shared
# template (std::string, std::stringstream) for every tier and say nothing.
#
# The assembly route (both compilers) keeps the LTO output as assembly,
# assembles it once to read the symbols it defines, renames them in the text
# (cmake/universal-asm.py) and assembles the result. The objects route (Clang)
# keeps the LTO objects and renames their symbols with objcopy.

foreach(_var UT_SOURCE_DIR UT_NAME UT_TIERS_DIR UT_ROUTE UT_COMPILER_ID UT_NM UT_OBJCOPY UT_OBJDUMP
             UT_STATIC_RUNTIME UT_CXX_COMPILER UT_CXX_COMPILER_VERSION)
    if(NOT DEFINED ${_var} OR "${${_var}}" STREQUAL "")
        message(FATAL_ERROR "${_var} is required")
    endif()
endforeach()
if(UT_ROUTE STREQUAL "assembly")
    if(NOT DEFINED UT_PYTHON OR UT_PYTHON STREQUAL "")
        message(FATAL_ERROR "UT_PYTHON is required for the assembly route")
    endif()
elseif(NOT (UT_ROUTE STREQUAL "objects" AND UT_COMPILER_ID MATCHES "Clang"))
    message(FATAL_ERROR "UT_ROUTE is assembly, or objects with Clang, not ${UT_ROUTE} with ${UT_COMPILER_ID}")
endif()
if(NOT DEFINED UT_COMP)
    set(UT_COMP "clang")
endif()
if(NOT DEFINED UT_DEV_ID)
    set(UT_DEV_ID "")
endif()
if(NOT DEFINED UT_ALIGN_FUNCTIONS)
    set(UT_ALIGN_FUNCTIONS "")
endif()

include("${CMAKE_CURRENT_LIST_DIR}/universal-check-tier.cmake")

# All three tiers must come from one source tree and one compiler: a tier built
# from other code, or by another clang, links without complaint. The source is
# hashed before the first tier and after each one, so an edit made while the
# tiers build fails here; each tier's build directory must name the compiler
# the dispatcher's build uses.
function(_ut_source_id out)
    file(GLOB_RECURSE _files LIST_DIRECTORIES false
        "${UT_SOURCE_DIR}/src/*" "${UT_SOURCE_DIR}/external/*" "${UT_SOURCE_DIR}/cmake/*")
    list(APPEND _files "${UT_SOURCE_DIR}/CMakeLists.txt" "${UT_SOURCE_DIR}/CMakePresets.json")
    list(SORT _files)
    set(_all "")
    foreach(_file IN LISTS _files)
        file(SHA256 "${_file}" _hash)
        string(APPEND _all "${_file} ${_hash}\n")
    endforeach()
    string(SHA256 _id "${_all}")
    set(${out} "${_id}" PARENT_SCOPE)
endfunction()

function(_ut_check_compiler tier dir)
    # The compiler as CMake resolved it: the cache may hold only the name
    # given (g++), which COMP sets.
    file(GLOB _compiler_files "${dir}/CMakeFiles/*/CMakeCXXCompiler.cmake")
    set(_compiler "")
    set(_version "")
    foreach(_file IN LISTS _compiler_files)
        file(STRINGS "${_file}" _line REGEX "^set\\(CMAKE_CXX_COMPILER \"")
        if(_line MATCHES "\"([^\"]+)\"")
            set(_compiler "${CMAKE_MATCH_1}")
        endif()
        file(STRINGS "${_file}" _line REGEX "^set\\(CMAKE_CXX_COMPILER_VERSION \"")
        if(_line MATCHES "\"([^\"]+)\"")
            set(_version "${CMAKE_MATCH_1}")
        endif()
    endforeach()
    if(_compiler STREQUAL "")
        message(FATAL_ERROR "Tier ${tier}: ${dir} records no C++ compiler")
    endif()
    file(REAL_PATH "${_compiler}" _compiler)
    file(REAL_PATH "${UT_CXX_COMPILER}" _expected)
    if(NOT _compiler STREQUAL _expected OR NOT _version STREQUAL UT_CXX_COMPILER_VERSION)
        message(FATAL_ERROR "Tier ${tier}: ${dir} was built by ${_compiler} ${_version}, "
                            "not ${_expected} ${UT_CXX_COMPILER_VERSION}")
    endif()
endfunction()

# Assembles one file with the tiers' compiler driver.
function(_ut_assemble source object)
    execute_process(
        COMMAND "${UT_CXX_COMPILER}" -c "${source}" -o "${object}"
        RESULT_VARIABLE _result
        ERROR_VARIABLE _error
    )
    if(NOT _result EQUAL 0)
        message(FATAL_ERROR "Assembling ${source} failed: ${_error}")
    endif()
endfunction()

# Clang's tiers leave the type identities shared: COMDATs whose copies are
# identical. GCC's LTO output also defines those of the static libstdc++ it
# pulls in, as ordinary data, so two tiers' copies would collide; a GCC tier
# renames them with the rest. libstdc++ compares type_info by its name string
# (__GXX_MERGED_TYPEINFO_NAMES is 0), so a tier still catches what the
# runtime throws.
if(UT_COMPILER_ID MATCHES "Clang")
    set(_share_types TRUE)
else()
    set(_share_types FALSE)
endif()

_ut_source_id(_source_id)
message(STATUS "Universal tiers: source ${_source_id}, ${UT_CXX_COMPILER} ${UT_CXX_COMPILER_VERSION}, "
               "${UT_ROUTE} route")

# Each tier is the release preset with its instruction-set option.
set(_tiers base sse41_popcnt avx2 pext)
set(_isa_base -DUSE_PEXT=OFF -DUSE_AVX2=OFF -DUSE_SSE41_POPCNT=OFF)
set(_isa_sse41_popcnt -DUSE_PEXT=OFF -DUSE_AVX2=OFF -DUSE_SSE41_POPCNT=ON)
set(_isa_avx2 -DUSE_PEXT=OFF -DUSE_AVX2=ON -DUSE_SSE41_POPCNT=OFF)
set(_isa_pext -DUSE_PEXT=ON -DUSE_AVX2=OFF -DUSE_SSE41_POPCNT=OFF)

file(REMOVE_RECURSE "${UT_TIERS_DIR}")
file(MAKE_DIRECTORY "${UT_TIERS_DIR}")
set(_rsp_lines "")

foreach(_tier IN LISTS _tiers)
    set(_dir "${UT_SOURCE_DIR}/build/${UT_NAME}-${_tier}")
    # GCC's profile is keyed to the object paths, so its PGO use build
    # rebuilds the instrumented tree in place (cmake/pgo-build.cmake).
    if(UT_COMPILER_ID STREQUAL "GNU")
        set(_use_dir "${_dir}-pgo-generate")
    else()
        set(_use_dir "${_dir}-pgo")
    endif()
    message(STATUS "Universal tier ${_tier}: release ${_isa_${_tier}} PGO in ${_dir}")
    file(REMOVE_RECURSE "${_dir}")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" --preset release -B "${_dir}"
                ${_isa_${_tier}}
                -DCOMP=${UT_COMP}
                -DPORTABLE_BUILD=ON
                -DSTATIC_RUNTIME=${UT_STATIC_RUNTIME}
                -DBASILISK_UNIVERSAL_TIER=ON
                -DBASILISK_UNIVERSAL_ROUTE=${UT_ROUTE}
                -DBASILISK_PGO_PRESET=release
                -DBASILISK_DEV_ID=${UT_DEV_ID}
                -DBASILISK_ALIGN_FUNCTIONS=${UT_ALIGN_FUNCTIONS}
        WORKING_DIRECTORY "${UT_SOURCE_DIR}"
        COMMAND_ERROR_IS_FATAL ANY
    )
    _ut_check_compiler(${_tier} "${_dir}")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" --build "${_dir}" --target pgo
        WORKING_DIRECTORY "${UT_SOURCE_DIR}"
        COMMAND_ERROR_IS_FATAL ANY
    )
    _ut_check_compiler(${_tier} "${_use_dir}")
    _ut_source_id(_now)
    if(NOT _now STREQUAL _source_id)
        message(FATAL_ERROR "Tier ${_tier}: the source changed while the tiers were building")
    endif()

    set(_tier_dir "${UT_TIERS_DIR}/${_tier}")
    file(MAKE_DIRECTORY "${_tier_dir}")
    if(CMAKE_HOST_WIN32)
        set(_ext obj)
    else()
        set(_ext o)
    endif()
    if(UT_ROUTE STREQUAL "objects")
        if(CMAKE_HOST_WIN32)
            file(GLOB_RECURSE _objects "${_use_dir}/*.exe.lto*.obj")
        else()
            file(GLOB _objects "${_use_dir}/basilisk.lto.o*")
        endif()
        list(LENGTH _objects _object_count)
        if(_object_count EQUAL 0)
            message(FATAL_ERROR "Tier ${_tier}: the engine link in ${_use_dir} kept no LTO objects")
        endif()
    else()
        if(UT_COMPILER_ID STREQUAL "GNU")
            file(GLOB _sources "${_use_dir}/basilisk.ltrans*.ltrans.s")
        else()
            execute_process(
                COMMAND "${CMAKE_COMMAND}" --build "${_use_dir}" --target basilisk_tier_asm
                WORKING_DIRECTORY "${UT_SOURCE_DIR}"
                COMMAND_ERROR_IS_FATAL ANY
            )
            # lld writes a file for each module beside its input
            # (basilisk_tier_asm.exe.lto.<module>.s); the gold plugin numbers
            # them after the regular LTO partition's, which is empty here.
            file(GLOB_RECURSE _candidates LIST_DIRECTORIES false "${_use_dir}/basilisk_tier_asm*")
            set(_sources "")
            foreach(_candidate IN LISTS _candidates)
                file(SIZE "${_candidate}" _size)
                if(_size GREATER 0)
                    list(APPEND _sources "${_candidate}")
                endif()
            endforeach()
        endif()
        list(SORT _sources)
        list(LENGTH _sources _object_count)
        if(_object_count EQUAL 0)
            message(FATAL_ERROR "Tier ${_tier}: the engine link in ${_use_dir} kept no LTO assembly")
        endif()
        # Assembled once as written, to read what the tier defines.
        set(_objects "")
        set(_index 0)
        foreach(_source IN LISTS _sources)
            set(_plain "${_tier_dir}/plain${_index}.${_ext}")
            _ut_assemble("${_source}" "${_plain}")
            list(APPEND _objects "${_plain}")
            math(EXPR _index "${_index} + 1")
        endforeach()
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
        if(_type MATCHES "^[aA]$" OR _name MATCHES "^\\." OR (_share_types AND _name MATCHES "^_ZT[IS]"))
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

    set(_renamed "")
    set(_index 0)
    if(UT_ROUTE STREQUAL "objects")
        foreach(_object IN LISTS _objects)
            set(_out "${_tier_dir}/${_index}.${_ext}")
            execute_process(
                COMMAND "${UT_OBJCOPY}" "--redefine-syms=${_map}" "${_object}" "${_out}"
                COMMAND_ERROR_IS_FATAL ANY
            )
            list(APPEND _renamed "${_out}")
            math(EXPR _index "${_index} + 1")
        endforeach()
    else()
        foreach(_source IN LISTS _sources)
            set(_text "${_tier_dir}/${_index}.s")
            set(_out "${_tier_dir}/${_index}.${_ext}")
            execute_process(
                COMMAND "${UT_PYTHON}" "${UT_SOURCE_DIR}/cmake/universal-asm.py"
                        rename "${_map}" "${_source}" "${_text}"
                COMMAND_ERROR_IS_FATAL ANY
            )
            _ut_assemble("${_text}" "${_out}")
            list(APPEND _renamed "${_out}")
            math(EXPR _index "${_index} + 1")
        endforeach()
        execute_process(
            COMMAND "${UT_PYTHON}" "${UT_SOURCE_DIR}/cmake/universal-asm.py" check-sections
                    ${_tier} "${UT_NM}" "${UT_OBJDUMP}" ${_renamed}
            RESULT_VARIABLE _sections_result
        )
        if(NOT _sections_result EQUAL 0)
            message(FATAL_ERROR "Tier ${_tier}: a section keeps the name of a renamed symbol")
        endif()
    endif()

    basilisk_check_tier_objects(${_tier} "${UT_NM}" "${UT_OBJDUMP}" ${_share_types} ${_renamed})

    message(STATUS "Universal tier ${_tier}: ${_object_count} ${UT_ROUTE} files, ${_rename_count} symbols renamed")
    foreach(_object IN LISTS _renamed)
        list(APPEND _rsp_lines "\"${_object}\"")
    endforeach()
endforeach()

list(JOIN _rsp_lines "\n" _rsp_text)
file(WRITE "${UT_TIERS_DIR}/objects.rsp" "${_rsp_text}\n")
