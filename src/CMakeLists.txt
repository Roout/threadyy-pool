cmake_minimum_required (VERSION 3.20)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(headers)
set(sources)

list(APPEND headers
    "ccqueue.hpp"
    "thread_pool.hpp"
)

list(APPEND sources 
    "main.cpp"
    "thread_pool.cpp"
)

add_executable(${This} ${sources} ${headers})

target_compile_options(${This} PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:$<$<CXX_COMPILER_ID:Clang>:-Wall -Werror -Wextra>>
    $<$<COMPILE_LANGUAGE:CXX>:$<$<CXX_COMPILER_ID:GNU>:-Wall -Werror -Wextra>>
    $<$<COMPILE_LANGUAGE:CXX>:$<$<CXX_COMPILER_ID:MSVC>:/W3>>
)

install(TARGETS ${This} DESTINATION ${CMAKE_BINARY_DIR}/bin)
