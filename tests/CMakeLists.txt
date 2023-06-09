cmake_minimum_required(VERSION 3.20.0)

set(This executor_tests)

set(headers 
    thread_pool_test.hpp
    timed_thread_pool_test.hpp
    scheduler_test.hpp
)

set(sources
    main.cpp
)

add_executable(${This} ${sources} ${headers})

target_compile_options(${This} PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:$<$<CXX_COMPILER_ID:Clang>:-Wall -Werror -Wextra>>
    $<$<COMPILE_LANGUAGE:CXX>:$<$<CXX_COMPILER_ID:GNU>:-Wall -Werror -Wextra -fsanitize=thread>>
    $<$<COMPILE_LANGUAGE:CXX>:$<$<CXX_COMPILER_ID:MSVC>:/W3>>
)

target_include_directories(${This} PUBLIC
    ${thread_pool_INCLUDE_DIRS}
)

target_link_libraries(${This} PUBLIC 
    gtest_main # target provided by gtest
    thread_pool_lib # main library
    $<$<CXX_COMPILER_ID:GNU>:tsan>
)

add_test(NAME ${This} COMMAND ${This})
