find_package(fmt CONFIG REQUIRED)

#find_package(pcre REQUIRED)
required_find_path(PCRE_INCLUDE_DIR pcrecpp.h)
required_find_library(PCRE_LIBRARY_DEBUG pcrecppd)
required_find_library(PCRE_LIBRARY_RELEASE pcrecpp)

set(src
	xt_comp.cpp
	xt_filt.cpp
	xt_link.cpp
	xt_main.cpp
	xt.h
	xt_inst_file.h
	xt_util.h
)

add_executable(xt_inst_gen ${src})

set_property(TARGET xt_inst_gen PROPERTY CXX_STANDARD 17)

target_include_directories(xt_inst_gen
	PRIVATE ${PCRE_INCLUDE_DIR}
)

if(MSVC)
	#doesn't work due to MSVC bug triggered by type_safe
	#target_compile_options(xt_inst_gen PRIVATE /permissive-)
endif()

target_link_libraries(xt_inst_gen 
	PRIVATE cppast
	PRIVATE fmt::fmt
	PRIVATE $<IF:$<CONFIG:Debug>,${PCRE_LIBRARY_DEBUG},${PCRE_LIBRARY_RELEASE}>
	PRIVATE llvm_demangle
)

install(TARGETS xt_inst_gen
	RUNTIME DESTINATION bin
)

if(WIN32)
	required_find_program(PCRECPP_DLL_DEBUG pcrecppd.dll)
	required_find_program(PCRECPP_DLL_RELEASE pcrecpp.dll)
	required_find_program(PCRE_DLL_DEBUG pcred.dll)
	required_find_program(PCRE_DLL_RELEASE pcre.dll)

	install(FILES
		$<TARGET_FILE:fmt::fmt>
		$<IF:$<CONFIG:Debug>,${PCRECPP_DLL_DEBUG},${PCRECPP_DLL_RELEASE}>
		$<IF:$<CONFIG:Debug>,${PCRE_DLL_DEBUG},${PCRE_DLL_RELEASE}>
		DESTINATION bin
	)

	install(FILES 
		../../build_rules/xt_inst_gen.targets
		../../build_rules/Xt_Build_Instances.cs
		../../build_rules/cuda_workaround.targets
		DESTINATION etc
	)
	
	install(FILES
		../../export_template-config.cmake
		DESTINATION cmake
	)
endif()
