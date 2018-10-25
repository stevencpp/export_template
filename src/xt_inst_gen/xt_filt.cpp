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

struct CompareString
{
	using is_transparent = void;
	using S = const std::string &;
	using SV = std::string_view;
	bool operator()(S a, S b) const { return a < b; }
	bool operator()(SV a, S b) const { return a < b; }
	bool operator()(S a, SV b) const { return a < b; }
};

std::set<std::string, CompareString> get_cached_headers(std::string_view header_cache_file_path) {
	std::set<std::string, CompareString> headers;
	std::ifstream f { header_cache_file_path };
	if (!f) return {};
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

	std::ofstream f { header_cache_file_path };
	if (!f) {
		fmt::print("failed to open header cache file '{}'\n", header_cache_file_path);
		return 1;
	}

	header_checker checker;

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