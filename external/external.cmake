# help cppast find clang on windows
required_find_program(XT_CLANG_BINARY clang++ 
	PATHS "C:/Program Files/LLVM/bin" DOC "path to the clang++ binary")

execute_process(COMMAND ${XT_CLANG_BINARY} -v ERROR_VARIABLE XT_CLANG_VERSION_OUTPUT)
if(${XT_CLANG_VERSION_OUTPUT} MATCHES "clang version ([^ \t\r\n]*)")
	set(LLVM_VERSION_EXPLICIT ${CMAKE_MATCH_1} CACHE STRING "llvm version")
else()
	message(FATAL_ERROR "could not get clang version from: ${XT_CLANG_VERSION_OUTPUT}")
endif()

get_filename_component(XT_CLANG_BIN_PATH ${XT_CLANG_BINARY} DIRECTORY)
required_find_library(LIBCLANG_LIBRARY libclang HINTS ${XT_CLANG_BIN_PATH}/../lib)
required_find_path(LIBCLANG_INCLUDE_DIR clang-c/Index.h HINTS ${XT_CLANG_BIN_PATH}/../include)

#
# add cppast
#
message(STATUS "Installing cppast via submodule")
execute_process(COMMAND git submodule update --init -- external/cppast
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_subdirectory(external/cppast EXCLUDE_FROM_ALL)
