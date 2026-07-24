cmake_minimum_required(VERSION 3.12)
project(kortex_cpp_examples LANGUAGES CXX)

# ─────────────────────────────────────────────────────────────────────────────
# Aggregate build: compile every example in this repository at once.
#
# Each example under <category>/<nn-name>/ can still be built on its own from
# its own CMakeLists.txt. This top-level file simply performs the dependency
# setup one time and then adds every example as a subdirectory, so the same two
# modes as the individual examples are supported:
#
#   Conan (default):
#       mkdir build && cd build
#       conan install .. <extra args for Ubuntu 22/24 — see README>   # optional on Ubuntu 20
#       cmake .. -DCMAKE_BUILD_TYPE=Release
#       cmake --build .
#
#   CMake only (no Conan):
#       cmake .. -DCMAKE_BUILD_TYPE=Release -DKORTEX_API_DIR=<path/to/kortex_api_cpp>
#       cmake --build .
#
# All example executables are placed together in build/bin/.
# ─────────────────────────────────────────────────────────────────────────────

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Collect every example executable in one place: build/bin/.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(Threads REQUIRED)

if(NOT DEFINED KORTEX_API_DIR)
    # ── Conan mode ───────────────────────────────────────────────────────────
    # Resolve kortex_api_cpp once here; the guarded per-example CMakeLists.txt
    # files reuse the imported target instead of setting it up again.
    if(NOT EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
        message(STATUS "Running conan install...")
        execute_process(
            COMMAND conan install ${CMAKE_CURRENT_SOURCE_DIR}
                    --install-folder ${CMAKE_BINARY_DIR}
                    --build=missing
            RESULT_VARIABLE CONAN_RESULT
        )
        if(NOT CONAN_RESULT EQUAL 0)
            message(FATAL_ERROR
                "conan install failed. Make sure Conan is installed and the "
                "Kinova remote is configured:\n"
                "  conan remote add kinova "
                "https://artifactory.kinovaapps.com/artifactory/api/conan/conan-public")
        endif()
    endif()
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    # NO_OUTPUT_DIRS: keep our CMAKE_RUNTIME_OUTPUT_DIRECTORY above (build/bin).
    conan_basic_setup(TARGETS NO_OUTPUT_DIRS)
endif()

# Discover every example: <category>/<nn-name>/ containing a CMakeLists.txt.
# CONFIGURE_DEPENDS re-globs when examples are added or removed.
file(GLOB EXAMPLE_DIRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/[0-9][0-9][0-9]-*/[0-9][0-9]-*")

list(SORT EXAMPLE_DIRS)
foreach(dir ${EXAMPLE_DIRS})
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${dir}/CMakeLists.txt")
        message(STATUS "Adding example: ${dir}")
        add_subdirectory(${dir})
    endif()
endforeach()
