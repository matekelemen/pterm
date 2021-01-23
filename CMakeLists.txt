cmake_minimum_required( VERSION 3.7.0 )

project( pterm C )

set( PTERM_ENABLE_DEBUG_OUTPUT OFF CACHE BOOL "Print debug output" )
if( ${PTERM_ENABLE_DEBUG_OUTPUT} )
    add_compile_definitions( PTERM_DEBUG )
endif()

include_directories( "${CMAKE_CURRENT_SOURCE_DIR}" )
add_executable( pterm "${CMAKE_CURRENT_SOURCE_DIR}/pterm.c" )
target_link_libraries( pterm m )