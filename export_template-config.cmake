cmake_minimum_required(VERSION 3.12)

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

required_find_program(XT_CLANG_BINARY clang++ 
	PATHS "C:/Program Files/LLVM/bin" DOC "path to the clang++ binary")
get_filename_component(XT_CLANG_PATH ${XT_CLANG_BINARY} DIRECTORY)

required_find_program(XT_INSTGEN_BINARY xt_inst_gen 
	HINTS ${CMAKE_INSTALL_PREFIX}/../export_template/bin DOC "path to the xt_inst_gen tool")
get_filename_component(XT_INSTGEN_PATH ${XT_INSTGEN_BINARY} DIRECTORY)

required_find_path(XT_TARGETS_PATH xt_inst_gen.targets
	HINTS ${XT_INSTGEN_PATH}/../etc DOC "path containing xt_inst_gen.targets and its dependencies")
	
set(XT_INST_SUFFIX ".xti" CACHE STRING 
	"extension use for the generated template instatiation files")

set(XT_INST_FILE_PATH "$(SolutionDir)intermediate/src/$(ProjectName)" CACHE STRING
	"path where the generated files will be placed (uses VS macros)")

function(target_export_template target)
	set_target_properties(${target} PROPERTIES 
		VS_GLOBAL_Xt_ClangPath ${XT_CLANG_PATH}
		VS_GLOBAL_Xt_InstGen_Path ${XT_INSTGEN_BINARY}
		VS_GLOBAL_Xt_TargetsPath ${XT_TARGETS_PATH}/
		VS_GLOBAL_Xt_InstSuffix ${XT_INST_SUFFIX}
		VS_GLOBAL_Xt_InstFilePath ${XT_INST_FILE_PATH}/
	)

	target_link_libraries(${target}
		${XT_TARGETS_PATH}/xt_inst_gen.targets
	)
endfunction()