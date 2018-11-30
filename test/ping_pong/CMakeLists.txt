cmake_minimum_required(VERSION 3.12)
project(test_ping_pong LANGUAGES CXX)

find_package(export_template REQUIRED)

add_executable(A
	ping_pong.cpp
	ping.h
	ping.cpp
	pong.h
	pong.cpp
)

set_property(TARGET A PROPERTY CXX_STANDARD 17)

target_export_template(A)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT A)