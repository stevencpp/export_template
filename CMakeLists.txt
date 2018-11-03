cmake_minimum_required(VERSION 3.1)

set(CMAKE_VS_INCLUDE_INSTALL_TO_DEFAULT_BUILD 1)
project(export_template LANGUAGES CXX)

# add external libraries
include(external/external.cmake)

add_subdirectory(src)