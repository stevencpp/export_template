cmake_minimum_required(VERSION 3.12)
project(test_dll LANGUAGES CXX)

find_package(export_template REQUIRED)

add_executable(A
	A_main.cpp
	B_loader.cpp
)

# TODO:
# B is meant to be loaded at runtime by A so
# A should not link with B but B should build before A
# so in VS, B should be a dependency of A but should not be referenced by it
target_link_libraries(A B C D)

add_library(B SHARED
	B.cpp
	B.h
)

add_library(D SHARED
	D.cpp
	D.h
)

target_link_libraries(B C)

add_library(C STATIC
	C.cpp
	C.h
)

set_property(TARGET A B C PROPERTY CXX_STANDARD 17)

target_export_template(A B C D)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT A)