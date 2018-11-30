#include "xt.h"
#include "xt_util.h"
#include "xt_inst_file.h"

#include <vector>
#include <fstream>
#include <filesystem>
#include <functional>
#include <map>
#include <set>
#include <cctype>

#include <fmt/core.h>
#include <fmt/ostream.h>

//warning C4251: 'pcrecpp::RE::pattern_': class 'std::basic_string<char,std::char_traits<char>,std::allocator<char>>'
//needs to have dll-interface to be used by clients of class 'pcrecpp::RE'
#pragma warning(disable:4251)
#include <pcrecpp.h>

#include "llvm\Demangle\MicrosoftDemangle.h"

namespace xt {

namespace fs = std::filesystem;

struct symbol
{
	std::string name; // full demangled name of the symbol
	std::string mangled; // mangled name
};

struct symbol_to_inst
{
	std::string name; // demangled name of the symbol
	std::string to_match; // fully qualified function/class name without templates
	std::string to_inst; // code generated for explicit instantiation

	bool operator ==(const symbol_to_inst & p) const {
		return name == p.name && to_match == p.to_match && to_inst == p.to_inst;
	}
	void print() {
		fmt::print("[\n\t{}\n\t{}\n\t{}\n]", name, to_match, to_inst);
	}
};

// copied from MicrosoftDemangleNodes.cpp
// Writes a space if the last token does not end with a punctuation.
static void outputSpaceIfNecessary(OutputStream &OS) {
	if (OS.empty())
		return;

	char C = OS.back();
	if (std::isalnum(C) || C == '>')
		OS << " ";
}

struct unresolved_symbol_parser
{
	struct buf {
		char * p = nullptr;
		std::size_t N = 0;
		bool init_stream(OutputStream &s) {
			return initializeOutputStream(p, &N, s, 1024);
		}
		void update(OutputStream &s) {
			p = s.getBuffer();
			N = std::max(N, s.getBufferCapacity());
		}
		~buf() {
			if (p) {
				std::free(p);
			}
		}
	};

	buf to_match_buf, to_inst_buf;

	std::optional<symbol_to_inst> get_inst(const symbol & symbol) {
		llvm::ms_demangle::Demangler D;
		StringView Name { symbol.mangled.c_str() };
		bool is_dll_imported = Name.consumeFront("__imp_");
		llvm::ms_demangle::SymbolNode *AST = D.parse(Name);
		if (D.Error) {
			fmt::print("ERROR: failed to demangle symbol '{}' ({})\n", symbol.mangled, symbol.name);
			return {};
		}
		if (AST->kind() != llvm::ms_demangle::NodeKind::FunctionSymbol) {
			return {}; // not an error
		}
		auto fsn = static_cast<llvm::ms_demangle::FunctionSymbolNode *>(AST);

		int nr_comps = (int)fsn->Name->Components->Count;
		auto for_each_identifier = [&](auto func) {
			for (int i = 0; i < nr_comps; ++i) {
				auto comp = fsn->Name->Components->Nodes[i];
				auto identifier = static_cast<llvm::ms_demangle::IdentifierNode *>(comp);
				func(i, identifier);
			}
		};

		int last_template_idx = -1;
		for_each_identifier([&](int idx, llvm::ms_demangle::IdentifierNode * identifier) {
			if (identifier->TemplateParams)
				last_template_idx = idx;
		});
		if (last_template_idx < 0)
			return {}; // not an error, just not a template

		auto OutputFlags = llvm::ms_demangle::OutputFlags::OF_Default;
		OutputStream to_match_stream, to_inst_stream;
		if (!to_match_buf.init_stream(to_match_stream) || 
			!to_inst_buf.init_stream(to_inst_stream)) {
			fmt::print("ERROR: failed to initialize output streams\n");
			return {};
		}

		auto output_without_template = [&](auto id, OutputStream &s) {
			auto * template_params = id->TemplateParams;
			id->TemplateParams = nullptr;
			id->output(s, OutputFlags);
			id->TemplateParams = template_params;
		};

		for_each_identifier([&](int idx, llvm::ms_demangle::IdentifierNode * identifier) {
			auto * id_to_match = identifier;
			if (identifier->kind() == llvm::ms_demangle::NodeKind::StructorIdentifier) {
				auto structor = static_cast<llvm::ms_demangle::StructorIdentifierNode *>(identifier);
				id_to_match = structor->Class;
			}
			output_without_template(id_to_match, to_match_stream);
			if (idx < nr_comps - 1)
				to_match_stream << "::";
		});
		to_match_stream += '\0';

		auto add_ids_to_inst_up_to_idx = [&](int idx_limit_inclusive, bool trailing_colons = false) {
			for_each_identifier([&](int idx, llvm::ms_demangle::IdentifierNode * identifier) {
				if (idx <= idx_limit_inclusive) {
					identifier->output(to_inst_stream, OutputFlags);
					if (trailing_colons || idx < idx_limit_inclusive)
						to_inst_stream << "::";
				}
			});
		};

		if (last_template_idx == nr_comps - 1) { // if it's a function template
			if (is_dll_imported)
				to_inst_stream << "__declspec(dllexport) ";

			bool add_return_type = true;
			bool add_name = true;

			auto func_id = fsn->Name->getUnqualifiedIdentifier();
			if (func_id->kind() == llvm::ms_demangle::NodeKind::ConversionOperatorIdentifier) {
				add_return_type = false;
				add_name = false;
				auto conv = static_cast<llvm::ms_demangle::ConversionOperatorIdentifierNode *>(func_id);
				add_ids_to_inst_up_to_idx(nr_comps - 2, true);
				to_inst_stream << "operator ";
				conv->TargetType->output(to_inst_stream, OutputFlags);
			}

			if (add_return_type) {
				if (fsn->Signature->ReturnType) {
					fsn->Signature->ReturnType->outputPre(to_inst_stream, OutputFlags);
					to_inst_stream << " ";
				}
				outputSpaceIfNecessary(to_inst_stream);
			}

			if (func_id->kind() == llvm::ms_demangle::NodeKind::IntrinsicFunctionIdentifier) {
				auto intrinsic = static_cast<llvm::ms_demangle::IntrinsicFunctionIdentifierNode *>(func_id);
				if (intrinsic->Operator == llvm::ms_demangle::IntrinsicFunctionKind::LessThan) {
					add_name = false;
					add_ids_to_inst_up_to_idx(nr_comps - 2, true);
					to_inst_stream << "operator< <";
					intrinsic->TemplateParams->output(to_inst_stream, OutputFlags);
					to_inst_stream << ">";
				}
			} else if (func_id->kind() == llvm::ms_demangle::NodeKind::StructorIdentifier) {
				add_name = false;
				add_ids_to_inst_up_to_idx(nr_comps - 2, true);
				auto structor = static_cast<llvm::ms_demangle::StructorIdentifierNode *>(func_id);
				output_without_template(structor->Class, to_inst_stream);
			}

			if(add_name)
				fsn->Name->output(to_inst_stream, OutputFlags);

			fsn->Signature->outputPost(to_inst_stream, OutputFlags);
		} else { // if it might be exported from a class template
			add_ids_to_inst_up_to_idx(last_template_idx);
		}
		to_inst_stream += '\0';

		to_match_buf.update(to_match_stream);
		to_inst_buf.update(to_inst_stream);

		return symbol_to_inst { symbol.name, to_match_stream.getBuffer(), to_inst_stream.getBuffer() };
	}

	std::vector<symbol_to_inst> get_symbols_to_inst(const std::vector<symbol> & unresolved_symbols) {
		std::vector<symbol_to_inst> ret;
		for (const symbol& unresolved_symbol : unresolved_symbols) {
			auto s_opt = get_inst(unresolved_symbol);
			if (!s_opt)
				continue;
			symbol_to_inst & s = s_opt.value();
			// if it's a class template you can have multiple unresolved members of it
			// but the class template instantiation should only be added once
			if (ret.end() != std::find_if(ret.begin(), ret.end(), [&](auto &p) { return p.to_inst == s.to_inst; }))
				continue;
			ret.emplace_back(std::move(s));
		}
		return ret;
	}
};

int test_link()
{
	unresolved_symbol_parser parser;

	std::vector<symbol> inputs;
	std::vector<symbol_to_inst> expected_results;
	auto add = [&](const char * name, const char *mangled, const char *to_match, const char *to_inst) {
		inputs.push_back({ name, mangled });
		expected_results.push_back({ name, to_match, to_inst });
	};

	add("private: class D1::D1<char>::D11<int *> * __cdecl D1::D1<char>::D11<int *>::foo<&void __cdecl D::dummy(char const &) > (class D1::D1<char>::D11<int *> *)",
		"", "D1::D1::D11::foo", "class D1::D1<char>::D11<int *> * __cdecl D1::D1<char>::D11<int *>::foo<&void __cdecl D::dummy(char const &) > (class D1::D1<char>::D11<int *> *)");
	add("public: static void __cdecl B::B::foo<bool>(void)",
		"", "B::B::foo", "void __cdecl B::B::foo<bool>(void)");
	add("void __cdecl Aa1::B_3s<f::g<int, float>::h<char, i<int,int>::j>::val>::f::g<int, float>::h(void)",
		"", "Aa1::B_3s::f::g::h", "struct Aa1::B_3s<f::g<int, float>::h<char, i<int,int>::j>::val>::f::g<int, float>");
	add("public: static void __cdecl C::C<char>::foo(void)",
		"", "C::C::foo", "struct C::C<char>");
	add("public: static void __cdecl D::D<char &>::d_foo<int *>(void)",
		"", "D::D::d_foo", "void __cdecl D::D<char &>::d_foo<int *>(void)");
	add("public: __cdecl C::C<char>::C<char><int>(int)", // remove both last templates
		"", "C::C::C", "__cdecl C::C<char>::C(int)");
	add("public: __cdecl C::C<char>::operator<int> int(void)",
		"", "C::C::operator", "__cdecl C::C<char>::operator int(void);");
	add("public: void __cdecl C::C<char>::operator()<int>(int)",
		"", "C::C::operator()", "void __cdecl C::C<char>::operator()<int>(int)");
	add("public: bool __cdecl C::C<char>::operator<<int>(int)",
		"", "C::C::operator<", "__cdecl C::C<char>::operator< <int>(int)"); // needs space after operator
	add("public: struct C::C<char> & __cdecl C::C<char>::operator=<int>(int)",
		"", "C::C::operator=", "struct C::C<char> & __cdecl C::C<char>::operator=<int>(int)");

	auto results = parser.get_symbols_to_inst(inputs);

	if (results != expected_results) {
		for (std::size_t i = 0; i < std::min(results.size(), expected_results.size()); ++i) {
			auto &r = results[i], &r_exp = expected_results[i];
			if (!(r == r_exp)) {
				fmt::print("expected "); r.print();
				fmt::print(" but got "); r.print();
				fmt::print("\n");
				parser.get_inst(inputs[i]);
			}
		}
		fmt::print("failed");
		return 1;
	}

	return 0;
}

std::optional< std::vector<symbol> > get_unresolved_symbols(std::string_view log_file)
{
	std::vector<symbol> unresolved_symbols;

	/*
	 Creating library C:\src\...\ModuleTest.lib and object C:\src\...\ModuleTest.exp
CUDATest2.lib(kernel.obj) : error LNK2019: unresolved external symbol "bool __cdecl split::addWithCuda<short>(short *,short const *,short const *,int)" (??$addWithCuda@F@split@@YA_NPEAFPEBF1H@Z) referenced in function "int __cdecl test_cuda(void)" (?test_cuda@@YAHXZ)
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

	// Linking might need to be done multiple times during the build, for example
	// if project A depends on but does not link with project B, a DLL, then
	// B will build before A so when B fails to link it starts the custom linker
	// loop, which needs to resolve all current unresolved symbols. 
	// Assuming every transitively dependent project uses the build customization,
	// if another link had failed prior to linking B then it would've already triggered
	// the linker loop so the first iteration of B's linker loop only needs to
	// look for the last link in the log. To make it easy to find this in the log, 
	// a custom message is printed by the Xt_PreLink target before every link.
	// Resolving the symbols for A can result in unresolved symbols injected into B
	// so both A and B may need to be built during an iteration of A's linker loop.
	// During A's iterations B's linker loop is disabled so e.g A's second iteration
	// may need to find the unresolved symbols from both A and B's linker output.
	// For this the linker loop prints a message before starting to build projects,
	// the same message printed by Xt_PreLink, but the latter is disabled during
	// iterations.

	std::string needle = "export_template: building projects";
	// do a reverse search here with rbegin/rend
	auto rev_it = std::search(log_contents.rbegin(), log_contents.rend(),
		std::boyer_moore_searcher(needle.rbegin(), needle.rend()));
	if (rev_it == log_contents.rend()) {
		fmt::print("failed to parse linker output: could not find '{}'\n", needle);
		return {};
	}
	auto it = rev_it.base();

	// find the unresolved symbols (both demangled and mangled versions)
	// note: in Debug the error is LNK2019, in Release the error is LNK2001
	needle = ": unresolved external symbol \"";
	std::boyer_moore_searcher searcher(needle.begin(), needle.end());
	pcrecpp::RE symbol_pattern { R""(([^"]*)" \(([^)]*))"" };
	while ((it = std::search(it, log_contents.end(), searcher)) != log_contents.end())
	{
		it += needle.size();
		symbol s;
		char *log_ptr = &log_contents[0] + (it - log_contents.begin());
		if(!symbol_pattern.PartialMatch(log_ptr, &s.name, &s.mangled)) {
			fmt::print("failed to parse symbol name / mangled name from the log\n");
			return {};
		}

		fmt::print("found unresolved symbol: {}\n", s.name);
		unresolved_symbols.emplace_back(std::move(s));
	}
	return unresolved_symbols;
}

int print_link_usage() {
	fmt::print("usage: {} link log_file project_list_file inst_files\n", tool_name);
	return 1;
}

int do_link(int argc, const char *argv[]) {
	// todo: use a proper test framework
	if (argc == 2 && strcmp(argv[1], "test") == 0)
		return test_link();

	if (argc < 4) {
		return print_link_usage();
	}

	auto log_file = argv[1];
	auto project_list_file = argv[2];
	auto inst_files = argv[3];
#if 0
	fmt::print("project: {}\nlog: {}\ninst files: {}\nproj list file: {}\n", 
		project_name, log_file, inst_files, project_list_file);
#endif
	
	auto unresolved_symbols_opt = get_unresolved_symbols(log_file);
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

	unresolved_symbol_parser parser;

	// find the function name for each symbol
	auto remaining_unresolved_names = parser.get_symbols_to_inst(unresolved_symbols);

	std::set<std::string> projects_to_build;

	//find the files which have definitions for each symbol
	std::vector<std::string_view> inst_files_vec = split_string_to_views(inst_files, ';');
	for (std::string_view inst_file_path : inst_files_vec) {
		try {
			xt_inst_file_reader_writer inst_file_rw { inst_file_path };
			xt_inst_file inst_file = inst_file_rw.read();

			for (auto exported_symbol : inst_file.get_exported_symbols()) {
				bool got_one = false;
				remove_if_and_erase(remaining_unresolved_names, [&](auto & p) {
					if (string_starts_with(p.to_match, exported_symbol.get())) {
						// normally if the symbol is unresolved then it shouldn't be instantiated in the .xti file
						// but it can be missing if the build system didn't rebuild the implementation file
						// todo: what if the symbol instantiation is there but rendered slightly differently ?
						if (inst_file.get_instantiated_symbols().contains(p.to_inst)) {
							fmt::print("ERROR: symbol '{}' already instantiated in '{}'\n", p.to_inst, inst_file_path);
							return true;
						}

						fmt::print("adding explicit instantiation '{}' for '{}' to '{}'\n", p.to_inst, p.name, inst_file_path);
						inst_file.append_instantiated_symbol(p.to_inst);
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
		fmt::print("nothing to build\n");
		return 1;
	}

	return 0;
}

} // namespace xt