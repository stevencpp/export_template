//..fmt/core.h(215): warning : 'result_of<fmt::v5::internal::custom_formatter<char, fmt::v5::basic_format_context<std::back_insert_iterator<fmt::v5::internal::basic_buffer<char> >, char> > (int)>'
// is deprecated: warning STL4014: std::result_of and std::result_of_t are deprecated in C++17.
// They are superseded by std::invoke_result and std::invoke_result_t.
// You can define _SILENCE_CXX17_RESULT_OF_DEPRECATION_WARNING or _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
// to acknowledge that you have received this warning. [-Wdeprecated-declarations]
#define _SILENCE_CXX17_RESULT_OF_DEPRECATION_WARNING

#include "xt.h"
#include "xt_util.h"
#include "xt_inst_file.h"

#include <vector>
#include <fstream>
#include <filesystem>
#include <functional>
#include <map>
#include <set>

#include <fmt/core.h>
#include <fmt/ostream.h>

//warning C4251: 'pcrecpp::RE::pattern_': class 'std::basic_string<char,std::char_traits<char>,std::allocator<char>>'
//needs to have dll-interface to be used by clients of class 'pcrecpp::RE'
#pragma warning(disable:4251)
#include <pcrecpp.h>


namespace xt {

namespace fs = std::filesystem;

struct unresolved_name_matcher
{
	pcrecpp::RE re;

	std::string get_pattern() {
		// e.g Aa1::B_3s<f::g<int, float>::h<char, i<int,int>::j>::val>::f::g<int, float>::h __cdecl asdf(int, int);
		// should match: type, name, arg1, arg2
		//R""(\w+(?:<(?:.|(?R))*>)?(?:::\w+(?:<(?:.|(?R))*>)?)*)"";
		std::string tpo = R""((?:<(?:.|(?R))*>)?)""; // optional recursive template pattern
		std::string word_tpo = "\\w+" + tpo;
		// word template? ( :: word template? )*
		std::string pattern = word_tpo + "(?:" "::" + word_tpo + ")*";
		// two match groups for the type and name
		return "__cdecl (" + pattern + ")";
	}

	unresolved_name_matcher() : re { get_pattern() } {

	}

	bool get_name(const std::string& symbol, std::string & name) {
		if (!re.PartialMatch(symbol, &name)) {
			fmt::print("failed to parse symbol: {}\n", symbol);
			return false;
		}
		return true;
	}

	struct name_symbol_pair { std::string name, symbol; };

	std::list<name_symbol_pair> get_list_from_symbols(const std::vector<std::string> & unresolved_symbols) {
		std::list<name_symbol_pair> ret;
		std::string name;
		for (const std::string& unresolved_symbol : unresolved_symbols) {
			if (get_name(unresolved_symbol, name))
				ret.push_back({ name, unresolved_symbol });
		}
		return ret;
	}
};

// remove b from a if b is in a, as long as b is at the beginning of a or preceded by a space
void string_search_erase_whole(std::string &a, const std::string_view & b) {
	auto it = std::search(a.begin(), a.end(), b.begin(), b.end());
	if (it != a.end() && (it == a.begin() || *(it-1) == ' ')) {
		a.erase(it, it + b.size());
	}
}

std::optional< std::vector<std::string> > get_unresolved_symbols(std::string_view log_file, std::string_view project_name)
{
	std::vector<std::string> unresolved_symbols;

	/*
	LINK : warning LNK4075: ignoring '/INCREMENTAL' due to '/FORCE' specification
	 Creating library C:\src\...\ModuleTest.lib and object C:\src\...\ModuleTest.exp
CUDATest2.lib(kernel.obj) : error LNK2019: unresolved external symbol "bool __cdecl split::addWithCuda<short>(short *,short const *,short const *,int)" (??$addWithCuda@F@split@@YA_NPEAFPEBF1H@Z) referenced in function "int __cdecl test_cuda(void)" (?test_cuda@@YAHXZ)
C:\src\...\ModuleTest.exe : warning LNK4088: image being generated due to /FORCE option; image may not run
  ModuleTest.vcxproj -> C:\src\...\ModuleTest.exe
	*/

	// find the start of this library's linker output
	// todo: since we're doing a backward search from the end,
	// maybe we don't need to read the whole file ?
	auto log_contents_opt = get_file_contents(log_file);
	if (!log_contents_opt) {
		fmt::print("failed to get the contents of the build log file '{}'\n", log_file);
		return {};
	}
	auto & log_contents = log_contents_opt.value();

	// linking might need to be done multiple times during the build
	// (either for the same project, iteratively, or for DLL dependencies)
	// our custom target runs before every link and prints this
	// to make it easy to find the unresolved symbols from only the last link
	std::string needle = str_cat("linking project ", project_name);
	// do a reverse search here with rbegin/rend
	auto rev_it = std::search(log_contents.rbegin(), log_contents.rend(),
		std::boyer_moore_searcher(needle.rbegin(), needle.rend()));
	if (rev_it == log_contents.rend()) {
		fmt::print("failed to parse linker output: could not find '{}'\n", needle);
		return {};
	}
	auto it = rev_it.base();

	// find the unresolved symbols
	// note: in Debug the error is LNK2019, in Release the error is LNK2001
	needle = ": unresolved external symbol \"";
	std::boyer_moore_searcher searcher(needle.begin(), needle.end());
	while ((it = std::search(it, log_contents.end(), searcher)) != log_contents.end())
	{
		it += needle.size();
		auto it_end = std::find(it, log_contents.end(), '\"');
		if (it_end == log_contents.end()) {
			fmt::print("failed to match quotation marks\n");
			return {};
		}
		std::string &symbol = unresolved_symbols.emplace_back(it, it_end);
		fmt::print("found unresolved symbol: {}\n", symbol);
		it = it_end;
	}
	return unresolved_symbols;
}

int print_link_usage() {
	fmt::print("usage: {} link project_name log_file inst_files project_list_file\n", tool_name);
	return 1;
}

int do_link(int argc, const char *argv[]) {
	if (argc < 5) {
		return print_link_usage();
	}

	auto project_name = argv[1];
	auto log_file = argv[2];
	auto inst_files = argv[3];
	auto project_list_file = argv[4];
	fmt::print("project: {}\nlog: {}\ninst files: {}\nproj list file: {}\n", 
		project_name, log_file, inst_files, project_list_file);
	
	auto unresolved_symbols_opt = get_unresolved_symbols(log_file, project_name);
	if (!unresolved_symbols_opt) {
		return 1; // failed to parse the log
	}

	auto & unresolved_symbols = unresolved_symbols_opt.value();
	if (unresolved_symbols.empty()) {
		fmt::print("could not find any unresolved symbols in the log");
		return 1;
	}
	// ^ this tool should only be called when there were link errors
	// but if those link errors were not due to unresolved symbols that the tool can fix
	// the build should fail

	unresolved_name_matcher matcher;

	// find the function name for each symbol
	auto remaining_unresolved_names = matcher.get_list_from_symbols(unresolved_symbols);

	std::set<std::string> projects_to_build;

	//find the files which have definitions for each symbol
	std::vector<std::string_view> inst_files_vec = split_string_to_views(inst_files, ';');
	for (std::string_view inst_file_path : inst_files_vec) {
		try {
			xt_inst_file_reader_writer inst_file_rw { inst_file_path };
			xt_inst_file inst_file = inst_file_rw.read();

			for (auto exported_symbol : inst_file.get_exported_symbols()) {
				bool got_one = false;
				remaining_unresolved_names.remove_if([&](auto & p) {
					// todo: if it's a class template, you can have multiple unresolved members of it
					// but the class template should only be added once
					if (string_starts_with(p.name, exported_symbol.get())) {
						// the explicit instantiation should not contain these
						// keywords that MSVC shows as part of the symbol:
						string_search_erase_whole(p.symbol, "public: ");
						string_search_erase_whole(p.symbol, "static ");
						 
						// normally if the symbol is unresolved then it shouldn't be instantiated in the .xti file
						// but it can be missing if the build system didn't rebuild the implementation file
						// todo: what if the symbol instantiation is there but rendered slightly differently ?
						if (inst_file.get_instantiated_symbols().contains(p.symbol)) {
							fmt::print("ERROR: symbol '{}' already instantiated in '{}'\n", p.symbol, inst_file_path);
							return true;
						}

						fmt::print("adding explicit instantiation for '{}' to '{}'\n", p.symbol, inst_file_path);
						inst_file.append_instantiated_symbol(p.symbol);
						got_one = true;
						return true;
					}
					return false;
				});
				if (got_one) {
					// write all of the new instantiations to the .xti file
					// this should trigger a recompile for all of the implementation files
					inst_file_rw.append_diff(inst_file);

					projects_to_build.emplace(inst_file.get_impl_project());
				}
			}
		} catch (std::runtime_error e) {
			fmt::print("caught exception: {}\n", e.what());
			return 1;
		}
	}

	std::ofstream f_proj { project_list_file };
	if (!f_proj) {
		fmt::print("failed to open {}\n", project_list_file);
		return 1;
	}
	for (auto & project : projects_to_build) {
		fmt::print(f_proj, "{}\n", project);
	}

	// prevent an infinite loop in case we can't fix any remaining unresolved symbols
	if (projects_to_build.empty()) {
		fmt::print("nothing to build");
		return 1;
	}

	return 0;
}

} // namespace xt