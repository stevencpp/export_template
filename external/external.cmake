#
# add cppast
#
message(STATUS "Installing cppast via submodule")
execute_process(COMMAND git submodule update --init -- external/cppast
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_subdirectory(external/cppast EXCLUDE_FROM_ALL)
