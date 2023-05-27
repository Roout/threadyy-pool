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

set(BUILD_TARGETS)

if(BUILD_EXE)
    list(APPEND BUILD_TARGETS ${This})
    add_executable(${This} ${sources} ${headers})
endif()

if(BUILD_STATIC_LIB OR BUILD_TESTS)
    list(APPEND BUILD_TARGETS ${This}_lib)
    add_library(${This}_lib STATIC ${sources} ${headers})
endif()

foreach(build_target ${BUILD_TARGETS})
    message(STATUS "Apply compile options for ${build_target}")
    target_compile_options(${build_target} PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:$<$<CXX_COMPILER_ID:Clang>:-Wall -Werror -Wextra>>
        $<$<COMPILE_LANGUAGE:CXX>:$<$<CXX_COMPILER_ID:GNU>:-Wall -Werror -Wextra>>
        $<$<COMPILE_LANGUAGE:CXX>:$<$<CXX_COMPILER_ID:MSVC>:/W3>>
    )
endforeach()

# Expose public includes to other
# subprojects through cache variable.
set(${This}_INCLUDE_DIRS
  ${PROJECT_SOURCE_DIR}/src
  CACHE INTERNAL "${This}: Include Directories" FORCE
)

install(TARGETS ${BUILD_TARGETS} DESTINATION ${CMAKE_BINARY_DIR}/bin)
