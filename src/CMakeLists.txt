cmake_minimum_required (VERSION 3.20.0)

list(APPEND headers
    "ccqueue.hpp"
    "task.hpp"
    "thread_pool.hpp"
    "scheduler.hpp"
    "timed_thread_pool.hpp"
)
    
list(APPEND sources
    "main.cpp"
    "thread_pool.cpp"
    "scheduler.cpp"
    "timed_thread_pool.cpp"
)

add_executable(${This} ${sources} ${headers})
add_library(${This}_lib STATIC ${sources} ${headers})

# Expose public includes to other
# subprojects through cache variable.
set(${This}_INCLUDE_DIRS
  ${PROJECT_SOURCE_DIR}/src
  CACHE INTERNAL "${This}: Include Directories" FORCE
)

target_compile_options(${This} PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:$<$<CXX_COMPILER_ID:Clang>:-Wall -Werror -Wextra>>
    $<$<COMPILE_LANGUAGE:CXX>:$<$<CXX_COMPILER_ID:GNU>:-Wall -Werror -Wextra>>
    $<$<COMPILE_LANGUAGE:CXX>:$<$<CXX_COMPILER_ID:MSVC>:/W3>>
)

install(TARGETS ${This} ${This}_lib DESTINATION ${CMAKE_BINARY_DIR}/bin)
