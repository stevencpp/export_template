#pragma once

namespace xt
{
	constexpr const char * tool_name = "xt_inst_gen";

	int do_comp(int argc, const char *argv[]);
	int do_link(int argc, const char *argv[]);
	int do_filt(int argc, const char *argv[]);
}
