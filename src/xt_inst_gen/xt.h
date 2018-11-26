#pragma once

//..fmt/core.h(215): warning : 'result_of<fmt::v5::internal::custom_formatter<char, fmt::v5::basic_format_context<std::back_insert_iterator<fmt::v5::internal::basic_buffer<char> >, char> > (int)>'
// is deprecated: warning STL4014: std::result_of and std::result_of_t are deprecated in C++17.
// They are superseded by std::invoke_result and std::invoke_result_t.
// You can define _SILENCE_CXX17_RESULT_OF_DEPRECATION_WARNING or _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
// to acknowledge that you have received this warning. [-Wdeprecated-declarations]
#define _SILENCE_CXX17_RESULT_OF_DEPRECATION_WARNING

namespace xt
{
	constexpr const char * tool_name = "xt_inst_gen";

	int do_comp(int argc, const char *argv[]);
	int do_link(int argc, const char *argv[]);
	int do_filt(int argc, const char *argv[]);
}
