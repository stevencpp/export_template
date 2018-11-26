cmake_minimum_required(VERSION 3.12)

set(CMAKE_VS_INCLUDE_INSTALL_TO_DEFAULT_BUILD 1)
project(export_template LANGUAGES CXX)

macro(required_find_common var name)
	if(NOT ${var})
		message(FATAL_ERROR "${type} ${name} not found")
	endif()
endmacro()
macro(required_find_path var name)
	find_path(${var} ${name} ${ARGN})
	required_find_common(${var} ${name})
endmacro()
macro(required_find_library var name)
	find_library(${var} ${name} ${ARGN})
	required_find_common(${var} ${name})
endmacro()
macro(required_find_program var name)
	find_program(${var} ${name} ${ARGN})
	required_find_common(${var} ${name})
endmacro()

# add external libraries
include(external/external.cmake)

add_subdirectory(src)