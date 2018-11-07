cmake_minimum_required(VERSION 3.12)
project(test_diamond LANGUAGES CXX)

find_package(export_template REQUIRED)

add_executable(A
	A_main.cpp
	A.h
	A.cpp
	A1.h
	A1.cpp
)

target_link_libraries(A B C)

add_library(B STATIC
	B.cpp
	B.h
)

target_link_libraries(B D)

add_library(C STATIC
	C.cpp
	C.h
)

target_link_libraries(C D)

add_library(D STATIC
	D.cpp
	D.h
	D1.h
	D1.cpp
)

set_property(TARGET A B C D PROPERTY CXX_STANDARD 17)

target_export_template(A B C D)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT A)