#include "xt.h"
#include "xt_util.h"

#include <filesystem>
#include <set>
#include <functional>

#include <fmt/core.h>
#include <fmt/ostream.h>

namespace xt
{

namespace fs = std::filesystem;

struct header_checker
{
	std::string needle = "[[export_template]]";
	std::boyer_moore_searcher< decltype(needle.begin()) > searcher { needle.begin(), needle.end() };

	bool is_xt_header(std::string_view header_path) {
		// todo: check if it's faster not to read the whole file
		// (my guess is that it'll often be slower)
		auto contents_opt = get_file_contents(header_path);
		if (!contents_opt) return false;
		auto &contents = contents_opt.value();
		return std::search(contents.begin(), contents.end(), searcher) != contents.end();
	}
};

auto get_cached_headers(std::string_view header_cache_file_path) {
	std::set<std::string, std::less<>> headers;
	std::ifstream f { header_cache_file_path };
	if (!f) return headers;
	std::string line;
	while (std::getline(f, line)) {
		headers.emplace(line);
	}
	return headers;
}

int print_filt_usage() {
	fmt::print("usage: {} filt headers header_cache_file_path\n", tool_name);
	return 1;
}

int do_filt(int argc, const char *argv[])
{
	if (argc < 3) {
		return print_filt_usage();
	}
	
	auto header_list = argv[1];
	auto header_cache_file_path = argv[2];

	std::error_code err;
	auto cache_last_modified = fs::last_write_time(header_cache_file_path, err);
	auto cached_headers = get_cached_headers(header_cache_file_path);

	// todo: if the path to the header cache does not exist, create directories ?
	std::ofstream f { header_cache_file_path };
	if (!f) {
		fmt::print("failed to open header cache file '{}'\n", header_cache_file_path);
		return 1;
	}

	header_checker checker;

	// todo: The following logic doesn't work for headers in the input list that 
	// existed but did not appear in the input list last time the cache file was written,
	// (so header.last_modified < cache.last_modified).
	// To support projects that include headers outside of the project file's path
	// currently the input list contains headers from ClInclude,ClCompile,CudaCompile,
	// so if you first create the header, run this tool, then include it in the project
	// then running the tool again won't recognize the new header.
	// A possible solution would be to save the input list to a file, then check it next time.

	for (auto header : split_string_to_views(header_list, ';')) {
		// if the header changed then we need to check again
		// otherwise it should still be an xt header if it was previously found to be
		// note: if the cache file does not exist, cache_last_modified is time::min()
		if (fs::last_write_time(header, err) > cache_last_modified ?
			checker.is_xt_header(header) :
			cached_headers.find(header) != cached_headers.end())
		{
			fmt::print(f, "{}\n", header);
		}
	}

	return 0;
}

} // namespace xt