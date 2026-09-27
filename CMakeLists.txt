cmake_minimum_required(VERSION 3.16)

# Give URL downloads (GLEW) fresh timestamps so a changed archive triggers a rebuild.
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

# The version lives in Config.h so code and package agree; bump it there.
file(STRINGS "${CMAKE_CURRENT_LIST_DIR}/Include/RatUI/Core/Config.h" _ratui_version_lines
    REGEX "^#define RATUI_VERSION_(MAJOR|MINOR|PATCH) [0-9]+")
foreach(_part MAJOR MINOR PATCH)
    string(REGEX MATCH "RATUI_VERSION_${_part} ([0-9]+)" _ "${_ratui_version_lines}")
    set(_ratui_version_${_part} ${CMAKE_MATCH_1})
endforeach()

project(RatUI
    VERSION     ${_ratui_version_MAJOR}.${_ratui_version_MINOR}.${_ratui_version_PATCH}
    LANGUAGES   CXX
    DESCRIPTION "A lightweight, retained-mode C++ UI library"
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

include(GNUInstallDirs)
include(CMakeDependentOption)
include(RatUIDependencies)

# RatUI is being consumed via add_subdirectory()/FetchContent when it is not the
# top level project. Default the "developer" switches off in that case.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(RATUI_IS_TOP_LEVEL ON)
else()
    set(RATUI_IS_TOP_LEVEL OFF)
endif()

# =============================================================================
# Options
# =============================================================================

# --- Dependency resolution ---------------------------------------------------
option(RATUI_FETCH_DEPENDENCIES
    "Download and build any dependency that find_package() cannot locate" ${RATUI_IS_TOP_LEVEL})

# --- Backends ----------------------------------------------------------------
# Each backend is opt-in and pulls in exactly the third party libraries it needs.
option(RATUI_BACKEND_FREETYPE "Enable the FreeType text backend (FreeType + HarfBuzz + msdfgen)" OFF)
option(RATUI_BACKEND_OPENGL   "Enable the OpenGL renderer backend"                                OFF)
option(RATUI_BACKEND_BGFX     "Enable the bgfx renderer backend (experimental)"                      OFF)

# --- What to build -----------------------------------------------------------
option(RATUI_BUILD_TESTS    "Build the RatUI test suite"          OFF)
option(RATUI_BUILD_EXAMPLES "Build the RatUI examples"            OFF)
option(RATUI_INSTALL        "Generate RatUI install/export rules" ${RATUI_IS_TOP_LEVEL})

# --- Misc --------------------------------------------------------------------
option(RATUI_ENABLE_ASSERTS "Enable RatUI runtime assertions" ON)

cmake_dependent_option(RATUI_EXPORT_COMPILE_COMMANDS
    "Write compile_commands.json" ON "RATUI_IS_TOP_LEVEL" OFF)

if(RATUI_EXPORT_COMPILE_COMMANDS)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

# The examples need a window and a GPU renderer, so they turn on what they use.
if(RATUI_BUILD_EXAMPLES AND NOT RATUI_BACKEND_OPENGL AND NOT RATUI_BACKEND_BGFX)
    message(STATUS "RatUI: examples requested, enabling the OpenGL backend")
    set(RATUI_BACKEND_OPENGL ON)
endif()

# The examples load TTF fonts through the FreeType backend.
if(RATUI_BUILD_EXAMPLES AND NOT RATUI_BACKEND_FREETYPE)
    message(STATUS "RatUI: examples requested, enabling the FreeType backend")
    set(RATUI_BACKEND_FREETYPE ON)
endif()

# =============================================================================
# Dependency versions (override on the command line to pin something else)
# =============================================================================
set(RATUI_FREETYPE_TAG "VER-2-13-2"     CACHE STRING "FreeType git tag used when fetching")
set(RATUI_HARFBUZZ_TAG "8.3.0"          CACHE STRING "HarfBuzz git tag used when fetching")
set(RATUI_MSDFGEN_TAG  "v1.12"          CACHE STRING "msdfgen git tag used when fetching")
set(RATUI_BGFX_TAG     "v1.161.9510-579" CACHE STRING "bgfx.cmake git tag used when fetching")
set(RATUI_SDL2_TAG     "release-2.30.9" CACHE STRING "SDL2 git tag used when fetching")
set(RATUI_CATCH2_TAG   "v3.5.0"         CACHE STRING "Catch2 git tag used when fetching")
set(RATUI_GLEW_URL "https://downloads.sourceforge.net/project/glew/glew/2.2.0/glew-2.2.0.tgz"
    CACHE STRING "GLEW source archive used when fetching")
mark_as_advanced(
    RATUI_FREETYPE_TAG RATUI_HARFBUZZ_TAG RATUI_MSDFGEN_TAG
    RATUI_GLEW_URL RATUI_BGFX_TAG RATUI_SDL2_TAG RATUI_CATCH2_TAG)

# =============================================================================
# Core library
# =============================================================================
set(RATUI_SOURCES
    Source/RatUI/Layout/Layout.cpp
    Source/RatUI/Layout/LayoutEngine.cpp
    Source/RatUI/Renderer/DrawBatcher.cpp
    Source/RatUI/Text/FontLibrary.cpp
    Source/RatUI/Text/GlyphAtlas.cpp
    Source/RatUI/Text/SpriteSheetFont.cpp
    Source/RatUI/Text/TextEdit.cpp
    Source/RatUI/Text/TextMetrics.cpp
    Source/RatUI/Widget/IWidget.cpp
    Source/RatUI/Widget/Scene.cpp
    Source/RatUI/Widget/Theme.cpp
)

add_library(RatUI ${RATUI_SOURCES})
add_library(RatUI::RatUI ALIAS RatUI)

target_include_directories(RatUI
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(RatUI PUBLIC cxx_std_20)

if(NOT RATUI_ENABLE_ASSERTS)
    target_compile_definitions(RatUI PUBLIC RATUI_ENABLE_ASSERTS=false)
endif()

set_target_properties(RatUI PROPERTIES
    VERSION     ${PROJECT_VERSION}
    EXPORT_NAME RatUI
)

# =============================================================================
# Backends
# =============================================================================

# --- FreeType text backend ---------------------------------------------------
if(RATUI_BACKEND_FREETYPE)
    ratui_require_dependency(
        NAME           Freetype
        TARGET         Freetype::Freetype
        FETCH_NAME     freetype
        FEATURE        "the FreeType text backend"
        GIT_REPOSITORY https://gitlab.freedesktop.org/freetype/freetype.git
        GIT_TAG        ${RATUI_FREETYPE_TAG}
        ALIAS_FROM     freetype
    )

    ratui_require_dependency(
        NAME           harfbuzz
        TARGET         harfbuzz::harfbuzz
        FEATURE        "the FreeType text backend"
        GIT_REPOSITORY https://github.com/harfbuzz/harfbuzz.git
        GIT_TAG        ${RATUI_HARFBUZZ_TAG}
        CACHE_ARGS     HB_BUILD_SUBSET OFF
        ALIAS_FROM     harfbuzz
    )

    ratui_require_dependency(
        NAME           msdfgen
        TARGET         msdfgen::msdfgen
        FEATURE        "the FreeType text backend"
        GIT_REPOSITORY https://github.com/Chlumsky/msdfgen.git
        GIT_TAG        ${RATUI_MSDFGEN_TAG}
        CACHE_ARGS
            MSDFGEN_BUILD_STANDALONE OFF
            MSDFGEN_INSTALL          OFF
            MSDFGEN_USE_VCPKG        OFF
            MSDFGEN_DYNAMIC_RUNTIME  ON
            MSDFGEN_USE_SKIA         OFF
            MSDFGEN_DISABLE_SVG      ON
            MSDFGEN_DISABLE_PNG      ON
    )

    target_sources(RatUI PRIVATE
        Source/RatUI/Backends/FreeType/FontLoader.cpp
    )

    # The public header of this backend includes <ft2build.h> and <hb.h>, so
    # those dependencies have to be PUBLIC. msdfgen is only used internally.
    target_link_libraries(RatUI PUBLIC
        Freetype::Freetype
        harfbuzz::harfbuzz
    )
    target_link_libraries(RatUI PRIVATE
        msdfgen::msdfgen
    )
    target_compile_definitions(RatUI PUBLIC RATUI_BACKEND_FREETYPE=1)
endif()

# --- OpenGL renderer backend -------------------------------------------------
if(RATUI_BACKEND_OPENGL AND EMSCRIPTEN)
    # WebGL2 is provided by the browser, so there is nothing to find or load.
    target_link_options(RatUI PUBLIC -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2)
    target_compile_definitions(RatUI PUBLIC RATUI_BACKEND_OPENGL=1)
elseif(RATUI_BACKEND_OPENGL)
    find_package(OpenGL REQUIRED)

    # The GLEW release archive ships sources only, so the target is built here.
    ratui_require_dependency(
        NAME       GLEW
        TARGET     GLEW::GLEW
        FETCH_NAME glew
        FEATURE    "the OpenGL renderer backend"
        URL        ${RATUI_GLEW_URL}
        SOURCE_ONLY
    )

    if(NOT TARGET GLEW::GLEW)
        enable_language(C)
        add_library(RatUI_glew STATIC "${glew_SOURCE_DIR}/src/glew.c")
        target_include_directories(RatUI_glew PUBLIC "${glew_SOURCE_DIR}/include")
        # GLEW_NO_GLU: RatUI never uses GLU, so don't make glew.h require its headers.
        target_compile_definitions(RatUI_glew PUBLIC GLEW_STATIC GLEW_NO_GLU)
        target_link_libraries(RatUI_glew PUBLIC OpenGL::GL)
        add_library(GLEW::GLEW ALIAS RatUI_glew)
    endif()

    target_link_libraries(RatUI PUBLIC GLEW::GLEW OpenGL::GL)
    target_compile_definitions(RatUI PUBLIC RATUI_BACKEND_OPENGL=1)
endif()

# --- bgfx renderer backend ---------------------------------------------------
if(RATUI_BACKEND_BGFX)
    ratui_require_dependency(
        NAME           bgfx
        TARGET         bgfx
        FEATURE        "the bgfx renderer backend"
        GIT_REPOSITORY https://github.com/bkaradzic/bgfx.cmake.git
        GIT_TAG        ${RATUI_BGFX_TAG}
        CACHE_ARGS
            BGFX_BUILD_EXAMPLES OFF
            BGFX_BUILD_TOOLS    OFF
    )

    target_link_libraries(RatUI PUBLIC bgfx)
    target_compile_definitions(RatUI PUBLIC RATUI_BACKEND_BGFX=1)
endif()

# =============================================================================
# Subdirectories
# =============================================================================
if(RATUI_BUILD_TESTS)
    enable_testing()
    add_subdirectory(Tests)
endif()

if(RATUI_BUILD_EXAMPLES)
    add_subdirectory(Examples)
endif()

# =============================================================================
# Install / export
# =============================================================================

# An exported target may not link against dependencies that are not themselves
# exported, which is exactly what FetchContent produces. Installing a build that
# fetched its dependencies would emit a broken package, so skip it instead.
if(RATUI_INSTALL)
    get_property(_ratui_dep_report GLOBAL PROPERTY RATUI_DEPENDENCY_REPORT)
    foreach(_entry IN LISTS _ratui_dep_report)
        if(_entry MATCHES "\|fetched")
            message(WARNING
                "RatUI: install rules disabled because at least one dependency was "
                "fetched rather than found. Install the dependencies system-wide "
                "(or via vcpkg/Conan) to produce an installable RatUI package.")
            set(RATUI_INSTALL OFF)
            break()
        endif()
    endforeach()
endif()

if(RATUI_INSTALL)
    include(CMakePackageConfigHelpers)

    install(TARGETS RatUI
        EXPORT RatUITargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    install(DIRECTORY Include/RatUI DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

    install(EXPORT RatUITargets
        FILE        RatUITargets.cmake
        NAMESPACE   RatUI::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/RatUI
    )

    set(RATUI_EXPORTED_BACKEND_FREETYPE ${RATUI_BACKEND_FREETYPE})
    set(RATUI_EXPORTED_BACKEND_OPENGL   ${RATUI_BACKEND_OPENGL})
    set(RATUI_EXPORTED_BACKEND_BGFX     ${RATUI_BACKEND_BGFX})

    configure_package_config_file(
        "${CMAKE_CURRENT_LIST_DIR}/cmake/RatUIConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/RatUIConfig.cmake"
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/RatUI
    )

    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/RatUIConfigVersion.cmake"
        COMPATIBILITY SameMajorVersion
    )

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/RatUIConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/RatUIConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/RatUI
    )
endif()

# =============================================================================
# Configure summary
# =============================================================================
if(RATUI_IS_TOP_LEVEL)
    message(STATUS "")
    message(STATUS "RatUI ${PROJECT_VERSION}")
    message(STATUS "  Backends: FreeType=${RATUI_BACKEND_FREETYPE} OpenGL=${RATUI_BACKEND_OPENGL} bgfx=${RATUI_BACKEND_BGFX}")
    message(STATUS "  Tests=${RATUI_BUILD_TESTS} Examples=${RATUI_BUILD_EXAMPLES} Install=${RATUI_INSTALL}")
    message(STATUS "  Fetch missing dependencies: ${RATUI_FETCH_DEPENDENCIES}")
    ratui_print_dependency_summary()
    message(STATUS "")
endif()
