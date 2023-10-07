cmake_minimum_required(VERSION 3.7.0)

project(pterm C)

set(PTERM_ENABLE_DEBUG_OUTPUT OFF CACHE BOOL "Print debug output")
if(${PTERM_ENABLE_DEBUG_OUTPUT})
    add_compile_definitions(PTERM_DEBUG)
endif()

add_executable(pterm "${CMAKE_CURRENT_SOURCE_DIR}/main.c")
target_include_directories(pterm PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

if(NOT WIN32)
    target_link_libraries(pterm m)
endif()
