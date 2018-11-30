cmake_minimum_required(VERSION 3.12)
project(test_dll LANGUAGES CXX)

find_package(export_template REQUIRED)

# == A ==
add_executable(A
	A_main.cpp
	B_loader.cpp
)
add_dependencies(A B) # don't link with B
target_link_libraries(A C D)

# == B ==
add_library(B SHARED
	B.cpp
	B.h
)
target_compile_definitions(B
	PRIVATE B_DLL_EXPORT
)
target_link_libraries(B C)

# == C ==
add_library(C STATIC
	C.cpp
	C.h
)

# == D ==
add_library(D SHARED
	D.cpp
	D.h
)
target_compile_definitions(D 
	PRIVATE D_DLL_EXPORT
	INTERFACE D_DLL_IMPORT
)
target_link_libraries(D E)

# == E ==
add_library(E SHARED
	E.cpp
	E.h
	E1.cpp
	E1.h
)
target_compile_definitions(E
	PRIVATE E_DLL_EXPORT
	INTERFACE E_DLL_IMPORT
)

set_property(TARGET A B C D E PROPERTY CXX_STANDARD 17)

target_export_template(A B C D E)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT A)