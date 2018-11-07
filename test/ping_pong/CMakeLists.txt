cmake_minimum_required(VERSION 3.12)
project(test_ping_pong LANGUAGES CXX)

find_package(export_template REQUIRED)

add_executable(ping_pong
	ping_pong.cpp
	ping.h
	ping.cpp
	pong.h
	pong.cpp
)

set_property(TARGET ping_pong PROPERTY CXX_STANDARD 17)

target_export_template(ping_pong)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ping_pong)