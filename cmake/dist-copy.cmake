cmake_minimum_required(VERSION 3.24)

# Copies a built binary into build/dist under a name that says what it is:
#   basilisk-<version>-<os>-<arch>[-<tier>][-<kind>]<ext>
# The version is the one the engine itself reports (src/version.cpp): X.Y.Z for
# a release, else dev-<commit date>[-m]-<hash> from the header regenerated at
# every build, so a commit made without reconfiguring is named correctly.
#
# Inputs: SRC_DIR, HEADER (generated basilisk_git_version.h), BINARY,
# DIST_DIR, STEM (<os>-<arch>), SUFFIX (tier and kind, may be empty), EXT.

foreach(_var SRC_DIR HEADER BINARY DIST_DIR STEM)
    if(NOT DEFINED ${_var} OR "${${_var}}" STREQUAL "")
        message(FATAL_ERROR "dist-copy.cmake needs ${_var}")
    endif()
endforeach()

file(STRINGS "${SRC_DIR}/src/constants.h" _line REGEX "engineVersion = \"")
if(NOT _line MATCHES "engineVersion = \"([^\"]+)\"")
    message(FATAL_ERROR "No engineVersion in ${SRC_DIR}/src/constants.h")
endif()
set(_version "${CMAKE_MATCH_1}")

if(_version MATCHES "-dev$")
    set(_id "")
    set(_date "")
    set(_sha "")
    set(_modified FALSE)
    if(EXISTS "${HEADER}")
        file(STRINGS "${HEADER}" _defines REGEX "^#define BASILISK_")
        foreach(_define IN LISTS _defines)
            if(_define MATCHES "^#define BASILISK_DEV_ID \"([^\"]*)\"")
                set(_id "${CMAKE_MATCH_1}")
            elseif(_define MATCHES "^#define BASILISK_GIT_DATE \"([^\"]*)\"")
                set(_date "${CMAKE_MATCH_1}")
            elseif(_define MATCHES "^#define BASILISK_GIT_SHA \"([^\"]*)\"")
                set(_sha "${CMAKE_MATCH_1}")
            elseif(_define MATCHES "^#define BASILISK_GIT_MODIFIED")
                set(_modified TRUE)
            endif()
        endforeach()
    endif()
    if(_id)
        set(_version "dev-${_id}")
    else()
        if(NOT _date)
            string(TIMESTAMP _date "%Y%m%d")
        endif()
        if(NOT _sha)
            set(_sha "nogit")
        endif()
        set(_version "dev-${_date}")
        if(_modified)
            string(APPEND _version "-m")
        endif()
        string(APPEND _version "-${_sha}")
    endif()
endif()

set(_name "basilisk-${_version}-${STEM}${SUFFIX}${EXT}")
file(MAKE_DIRECTORY "${DIST_DIR}")
file(COPY_FILE "${BINARY}" "${DIST_DIR}/${_name}" ONLY_IF_DIFFERENT)
message(STATUS "Copied to ${DIST_DIR}/${_name}")
