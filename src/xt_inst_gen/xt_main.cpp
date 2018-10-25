#include "xt.h"

#include <fmt/core.h>

using namespace xt;

int print_usage()
{
	fmt::print("usage: {} filt/comp/link ...\n", tool_name);
	return 0;
}

int main(int argc, const char *argv[])
{
	if (argc < 2) {
		return print_usage();
	}

	std::string cmd = argv[1];
	if (cmd == "comp") {
		return do_comp(argc - 1, &argv[1]);
	} else if (cmd == "link") {
		return do_link(argc - 1, &argv[1]);
	} else if (cmd == "filt") {
		return do_filt(argc - 1, &argv[1]);
	} else {
		return print_usage();
	}
	
	return 1; // should be unreachable
}