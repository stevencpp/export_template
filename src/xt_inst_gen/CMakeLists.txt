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
target_link_libraries(xt_inst_gen PRIVATE cppast)