
cmake_minimum_required(VERSION 3.20.0)

set(This thread_pool)
project(${This} VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_TESTS          "Build the tests"           ON)
option(DOWNLOAD_GTEST       "Download googletest"       ON)
option(BUILD_STATIC_LIB     "Build the static library"  ON)
option(BUILD_EXE            "Build the executable"      ON)

add_subdirectory("src")

if(BUILD_TESTS)
    if(NOT BUILD_STATIC_LIB)
        message(FATAL_ERROR "Trying to build tests without linking threadyy-pool static lib")
    endif()
    enable_testing()
    if(DOWNLOAD_GTEST)
        # Download and unpack googletest at configure time
        configure_file(vendor/googletest.txt.in googletest-download/CMakeLists.txt)
        execute_process(
            COMMAND "${CMAKE_COMMAND}" -G "${CMAKE_GENERATOR}" .
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/googletest-download"
        )
        execute_process(
            COMMAND "${CMAKE_COMMAND}" --build .
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/googletest-download"
        )

        # Prevent GoogleTest from overriding our compiler/linker options
        # when building with Visual Studio
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

        # Add googletest directly to our build. This adds the following targets:
        # gtest, gtest_main, gmock and gmock_main
        add_subdirectory(
            ${CMAKE_BINARY_DIR}/googletest-src
            ${CMAKE_BINARY_DIR}/googletest-build
        )
    endif()
    add_subdirectory("tests")
endif()
