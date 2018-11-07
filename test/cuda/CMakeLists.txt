cmake_minimum_required(VERSION 3.12)
project(test_cuda LANGUAGES CXX CUDA)

find_package(export_template REQUIRED)

add_executable(A
	A_main.cpp
	A.cu
	A.h
)

target_link_libraries(A B)

add_library(B STATIC
	B.cu
	B.h
)

set_property(TARGET A B 
	PROPERTY CXX_STANDARD 14
	PROPERTY CUDA_SEPARABLE_COMPILATION ON
)

target_export_template(A B)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT A)