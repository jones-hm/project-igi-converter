cmake_minimum_required(VERSION 3.16)

project(igi1conv
  VERSION 1.0.0
  DESCRIPTION "Project IGI 1 Game Asset Converter"
  LANGUAGES CXX C)

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

# ─── igi1conv CLI ───────────────────────────────────────────────────────────────
file(GLOB IGI1CONV_SOURCES "igi1conv/*.cpp" "igi1conv/*.h")
if (WIN32)
    list(APPEND IGI1CONV_SOURCES "assets/igi1conv.rc")
endif()

add_executable(igi1conv ${IGI1CONV_SOURCES} ${SOURCES_PARSERS})

set_target_properties(igi1conv PROPERTIES
    OUTPUT_NAME "igi1conv"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PROJECT_SOURCE_DIR}/bin/Release"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${PROJECT_SOURCE_DIR}/bin/Debug"
)

# --- GUI Dependencies (Qt6) ---
set(CMAKE_PREFIX_PATH "D:/Qt/6.5.3/msvc2019_64")
find_package(Qt6 COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

target_link_libraries(igi1conv PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets)
# ---------------------------------------

# Single source of truth for the version string (also stamped in assets/igi1conv.rc).
target_compile_definitions(igi1conv PRIVATE IGI1CONV_VERSION="${PROJECT_VERSION}")

target_include_directories(igi1conv PRIVATE
    ${PROJECT_SOURCE_DIR}/igi1conv          # igi1conv/pch.h, cmd_*.h
    ${PROJECT_SOURCE_DIR}/source         # common.h, logger.h, utils.h, config.h, level/, renderer/
    ${PROJECT_SOURCE_DIR}/source/parsers
    ${PROJECT_SOURCE_DIR}/third_party/glm-1.0.1
    ${PROJECT_SOURCE_DIR}/third_party
)

if (MSVC)
    # igi1conv uses no PCH; parsers #include "pch.h" expecting it force-included.
    # /FI force-includes igi1conv/pch.h (no GL/app deps) into every TU.
    target_compile_options(igi1conv PRIVATE
        "/bigobj" "/FS"
        "/FI${PROJECT_SOURCE_DIR}/igi1conv/pch.h"
    )
    set_target_properties(igi1conv PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
    )
endif()

# ─── tests ───────────────────────────────────────────────────────────────────
option(IGI1CONV_BUILD_TESTS "Build igi1conv test suite" ON)

if (IGI1CONV_BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    enable_testing()

    file(GLOB IGI1CONV_TEST_SOURCES "tests/*.cpp")
    add_executable(igi1conv_tests ${IGI1CONV_TEST_SOURCES})

    add_dependencies(igi1conv_tests igi1conv)

    if (MSVC)
        set_target_properties(igi1conv_tests PROPERTIES
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
        )
    endif()

    set_target_properties(igi1conv_tests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PROJECT_SOURCE_DIR}/bin/Release"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${PROJECT_SOURCE_DIR}/bin/Debug"
    )

    target_link_libraries(igi1conv_tests PRIVATE gtest)

    include(GoogleTest)
    gtest_discover_tests(igi1conv_tests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
endif()
