cmake_minimum_required(VERSION 3.12)
project(low_level_cyclic_control LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

add_executable(${PROJECT_NAME} main.cpp)

if(DEFINED KORTEX_API_DIR)
    # ── CMake-only mode ──────────────────────────────────────────────────────
    # Point -DKORTEX_API_DIR to the root of the extracted kortex_api_cpp
    # package (the folder that contains include/ and lib/).
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${KORTEX_API_DIR}/include/client
        ${KORTEX_API_DIR}/include/client_stubs
        ${KORTEX_API_DIR}/include/common
        ${KORTEX_API_DIR}/include/messages
        ${KORTEX_API_DIR}/include/participant
        ${KORTEX_API_DIR}/include/server
        ${KORTEX_API_DIR}/include/server_stubs
        ${KORTEX_API_DIR}/include/util
        ${KORTEX_API_DIR}/thirdParty/include
    )
    target_link_libraries(${PROJECT_NAME}
        ${KORTEX_API_DIR}/lib/libKortexApiCpp.a
        Threads::Threads
    )
else()
    # ── Conan mode ───────────────────────────────────────────────────────────
    # conan install is run automatically if conanbuildinfo.cmake is missing.
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
    # Guarded so an aggregate build (top-level CMakeLists.txt) that already ran
    # conan_basic_setup does not define the imported target a second time.
    if(NOT TARGET CONAN_PKG::kortex_api_cpp)
        include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
        conan_basic_setup(TARGETS)
    endif()
    target_link_libraries(${PROJECT_NAME}
        CONAN_PKG::kortex_api_cpp
        Threads::Threads
    )
endif()
