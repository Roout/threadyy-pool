cmake_minimum_required(VERSION 3.20.0)

set(This executor_tests)

set(headers 
    thread_pool_test.hpp
)

set(sources
    main.cpp
)

add_executable(${This} ${sources} ${headers})

target_compile_options(${This} PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:$<$<CXX_COMPILER_ID:Clang>:-Wall -Werror -Wextra>>
    $<$<COMPILE_LANGUAGE:CXX>:$<$<CXX_COMPILER_ID:GNU>:-Wall -Werror -Wextra>>
    $<$<COMPILE_LANGUAGE:CXX>:$<$<CXX_COMPILER_ID:MSVC>:/W3>>
)

target_include_directories(${This} PUBLIC
    ${thread_pool_INCLUDE_DIRS}
)

target_link_libraries(${This} PUBLIC 
    gtest_main # target provided by gtest
    thread_pool_lib # main library
)

add_test(
    NAME ${This}
    COMMAND ${This}
)
