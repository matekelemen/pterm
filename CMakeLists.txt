cmake_minimum_required( VERSION 3.7.0 )

project( pterm C )

include_directories( "${CMAKE_CURRENT_SOURCE_DIR}" )

add_executable( pterm "${CMAKE_CURRENT_SOURCE_DIR}/pterm.c" )
target_link_libraries( pterm m )