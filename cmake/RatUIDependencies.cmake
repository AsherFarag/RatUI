# =============================================================================
# RatUIDependencies.cmake
#
# Small helper layer around find_package()/FetchContent so that every RatUI
# dependency follows the same rule:
#
#   1. If the target already exists (the user added it to their own tree), use it.
#   2. Otherwise try find_package() to pick up a system / vcpkg / Conan install.
#   3. Otherwise, if RATUI_FETCH_DEPENDENCIES is ON, download and build it.
#   4. Otherwise fail with a message that tells the user exactly what to do.
# =============================================================================

include_guard(GLOBAL)
include(FetchContent)

# Record a dependency + how it was satisfied, for the configure summary.
set_property(GLOBAL PROPERTY RATUI_DEPENDENCY_REPORT "")

function(_ratui_report_dependency name how)
    get_property(report GLOBAL PROPERTY RATUI_DEPENDENCY_REPORT)
    list(APPEND report "${name}|${how}")
    set_property(GLOBAL PROPERTY RATUI_DEPENDENCY_REPORT "${report}")
endfunction()

#[[
ratui_require_dependency(
    NAME            <find_package name>
    TARGET          <imported target the rest of the build links against>
    [FETCH_NAME     <FetchContent name, defaults to NAME>]
    [GIT_REPOSITORY <url> GIT_TAG <tag>]
    [URL            <archive url>]
    [ALIAS_FROM     <target created by the fetched project, aliased to TARGET>]
    [SOURCE_ONLY]                     # only populate sources; caller makes TARGET
    [CACHE_ARGS     <VAR VALUE>...]   # cache variables forced before fetching
    [FEATURE        <human readable name of the feature that needs this>]
)
]]
function(ratui_require_dependency)
    cmake_parse_arguments(ARG
        "SOURCE_ONLY"
        "NAME;TARGET;FETCH_NAME;GIT_REPOSITORY;GIT_TAG;URL;ALIAS_FROM;FEATURE"
        "CACHE_ARGS"
        ${ARGN})

    if(NOT ARG_NAME OR NOT ARG_TARGET)
        message(FATAL_ERROR "ratui_require_dependency: NAME and TARGET are required")
    endif()

    if(TARGET ${ARG_TARGET})
        _ratui_report_dependency("${ARG_NAME}" "provided by the parent project")
        return()
    endif()

    find_package(${ARG_NAME} QUIET)
    if(TARGET ${ARG_TARGET})
        _ratui_report_dependency("${ARG_NAME}" "found on the system")
        return()
    endif()

    if(NOT RATUI_FETCH_DEPENDENCIES)
        set(feature "${ARG_FEATURE}")
        if(NOT feature)
            set(feature "a requested RatUI backend")
        endif()
        message(FATAL_ERROR
            "RatUI: '${ARG_NAME}' is required by ${feature} but was not found.\n"
            "Either install it and make it visible to find_package(), or configure with\n"
            "    -DRATUI_FETCH_DEPENDENCIES=ON\n"
            "to let RatUI download and build it automatically.")
    endif()

    if(NOT ARG_FETCH_NAME)
        set(ARG_FETCH_NAME "${ARG_NAME}")
    endif()

    # Force any build options the dependency needs before it is configured.
    if(ARG_CACHE_ARGS)
        list(LENGTH ARG_CACHE_ARGS _count)
        math(EXPR _rem "${_count} % 2")
        if(NOT _rem EQUAL 0)
            message(FATAL_ERROR "ratui_require_dependency: CACHE_ARGS must be VAR VALUE pairs")
        endif()
        math(EXPR _last "${_count} - 1")
        foreach(i RANGE 0 ${_last} 2)
            math(EXPR j "${i} + 1")
            list(GET ARG_CACHE_ARGS ${i} _var)
            list(GET ARG_CACHE_ARGS ${j} _val)
            set(${_var} "${_val}" CACHE INTERNAL "Forced by RatUI" FORCE)
        endforeach()
    endif()

    message(STATUS "RatUI: fetching ${ARG_NAME} ...")

    # Several upstream projects still declare cmake_minimum_required(VERSION 3.x)
    # with an x that CMake 4 refuses outright. Relax that just for the fetched
    # subproject; it does not affect RatUI itself.
    if(NOT DEFINED CMAKE_POLICY_VERSION_MINIMUM)
        set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
    endif()

    if(ARG_URL)
        FetchContent_Declare(${ARG_FETCH_NAME} URL ${ARG_URL})
    elseif(ARG_GIT_REPOSITORY)
        FetchContent_Declare(${ARG_FETCH_NAME}
            GIT_REPOSITORY ${ARG_GIT_REPOSITORY}
            GIT_TAG        ${ARG_GIT_TAG}
            GIT_SHALLOW    TRUE
            GIT_PROGRESS   TRUE)
    else()
        message(FATAL_ERROR "ratui_require_dependency(${ARG_NAME}): no GIT_REPOSITORY or URL to fetch from")
    endif()

    FetchContent_MakeAvailable(${ARG_FETCH_NAME})

    # FetchContent sets these in the calling (function) scope only.
    string(TOLOWER "${ARG_FETCH_NAME}" _lower_name)
    set(${ARG_FETCH_NAME}_SOURCE_DIR "${${_lower_name}_SOURCE_DIR}" PARENT_SCOPE)
    set(${ARG_FETCH_NAME}_BINARY_DIR "${${_lower_name}_BINARY_DIR}" PARENT_SCOPE)
    set(${_lower_name}_SOURCE_DIR    "${${_lower_name}_SOURCE_DIR}" PARENT_SCOPE)
    set(${_lower_name}_BINARY_DIR    "${${_lower_name}_BINARY_DIR}" PARENT_SCOPE)

    # Some dependencies ship no usable CMake project; the caller builds a target
    # itself from <name>_SOURCE_DIR.
    if(ARG_SOURCE_ONLY)
        _ratui_report_dependency("${ARG_NAME}" "fetched (sources)")
        return()
    endif()

    if(ARG_ALIAS_FROM AND NOT TARGET ${ARG_TARGET} AND TARGET ${ARG_ALIAS_FROM})
        add_library(${ARG_TARGET} ALIAS ${ARG_ALIAS_FROM})
    endif()

    if(NOT TARGET ${ARG_TARGET})
        message(FATAL_ERROR
            "RatUI: fetched '${ARG_NAME}' but the target '${ARG_TARGET}' was not created.")
    endif()

    _ratui_report_dependency("${ARG_NAME}" "fetched")
endfunction()

# Print how every dependency was satisfied.
function(ratui_print_dependency_summary)
    get_property(report GLOBAL PROPERTY RATUI_DEPENDENCY_REPORT)
    if(NOT report)
        return()
    endif()
    message(STATUS "  Dependencies:")
    foreach(entry IN LISTS report)
        string(REPLACE "|" ";" parts "${entry}")
        list(GET parts 0 name)
        list(GET parts 1 how)
        message(STATUS "    ${name}: ${how}")
    endforeach()
endfunction()
