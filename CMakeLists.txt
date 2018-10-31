cmake_minimum_required(VERSION 3.1)
project(export_template LANGUAGES CXX)

# add external libraries
include(external/external.cmake)

add_subdirectory(src)