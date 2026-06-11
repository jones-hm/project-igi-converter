cmake_minimum_required(VERSION 3.16)

project(gconv
  VERSION 1.0.0
  LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release")
endif ()
message("CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-D_DEBUG)
endif ()

if (WIN32)
    add_definitions(-DUNICODE -D_UNICODE)
endif()

if (MSVC)
    add_compile_options("/FS")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

# ─── shared engine subset (parsers + headers reused from the IGI editor) ──────
file(GLOB SOURCES_PARSERS "source/parsers/*.cpp" "source/parsers/*.h")

# ─── gconv CLI ───────────────────────────────────────────────────────────────
file(GLOB GCONV_SOURCES "gconv/*.cpp" "gconv/*.h")
if (WIN32)
    list(APPEND GCONV_SOURCES "assets/gconv.rc")
endif()

add_executable(gconv ${GCONV_SOURCES} ${SOURCES_PARSERS})

set_target_properties(gconv PROPERTIES
    OUTPUT_NAME "gconv"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PROJECT_SOURCE_DIR}/bin/Release"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${PROJECT_SOURCE_DIR}/bin/Debug"
)

# Single source of truth for the version string (also stamped in assets/gconv.rc).
target_compile_definitions(gconv PRIVATE GCONV_VERSION="${PROJECT_VERSION}")

target_include_directories(gconv PRIVATE
    ${PROJECT_SOURCE_DIR}/gconv          # gconv/pch.h, cmd_*.h
    ${PROJECT_SOURCE_DIR}/source         # common.h, logger.h, utils.h, config.h, level/, renderer/
    ${PROJECT_SOURCE_DIR}/source/parsers
    ${PROJECT_SOURCE_DIR}/third_party/glm-1.0.1
    ${PROJECT_SOURCE_DIR}/third_party
)

if (MSVC)
    # gconv uses no PCH; parsers #include "pch.h" expecting it force-included.
    # /FI force-includes gconv/pch.h (no GL/app deps) into every TU.
    target_compile_options(gconv PRIVATE
        "/bigobj" "/FS"
        "/FI${PROJECT_SOURCE_DIR}/gconv/pch.h"
    )
    set_target_properties(gconv PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
    )
endif()

# ─── tests ───────────────────────────────────────────────────────────────────
option(GCONV_BUILD_TESTS "Build gconv test suite" ON)

if (GCONV_BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    enable_testing()

    file(GLOB GCONV_TEST_SOURCES "tests/*.cpp")
    add_executable(gconv_tests ${GCONV_TEST_SOURCES})

    add_dependencies(gconv_tests gconv)

    if (MSVC)
        set_target_properties(gconv_tests PROPERTIES
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
        )
    endif()

    set_target_properties(gconv_tests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PROJECT_SOURCE_DIR}/bin/Release"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${PROJECT_SOURCE_DIR}/bin/Debug"
    )

    target_link_libraries(gconv_tests PRIVATE gtest_main)

    include(GoogleTest)
    gtest_discover_tests(gconv_tests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
endif()
