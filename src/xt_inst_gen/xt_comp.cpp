#include "xt.h"
#include "xt_util.h"
#include "xt_inst_file.h"

#include <fstream>
#include <string>
#include <filesystem>
#include <optional>

#include <fmt/core.h>
#include <fmt/ostream.h>

#include <cppast/visitor.hpp>
#include <cppast/libclang_parser.hpp>
#include <cppast/cpp_function_template.hpp>
#include <cppast/cpp_member_function.hpp>
#include <cppast/cpp_class_template.hpp>
#include <type_safe/reference.hpp>

namespace xt {

namespace fs = std::filesystem;

// add the namespace(s),class(es) and the function name
std::string get_fully_qualified_name_without_templates(const cppast::cpp_entity & e) {
	type_safe::optional_ref<const cppast::cpp_entity> current { e };
	std::vector<std::string> containers;
	while (current && current.value().kind() != cppast::cpp_file::kind()) {
		if(!cppast::is_template(current.value().kind()))
			containers.emplace_back(current.value().name());
		current = current.value().parent();
	}
	std::reverse(containers.begin(), containers.end());
	bool first = true;
	std::string ret;
	for (const auto &container : containers) {
		if (!first) ret += "::";
		first = false;
		ret += container;
	}
	return ret;
}

std::string get_return_type(const cppast::cpp_function_base & func) {
	// todo: the return type is not fully namespace qualified
	if (func.kind() == cppast::cpp_function::kind()) {
		auto &f = static_cast<const cppast::cpp_function&>(func);
		return cppast::to_string(f.return_type());
	} else if (func.kind() == cppast::cpp_member_function::kind()) {
		auto &f = static_cast<const cppast::cpp_member_function&>(func);
		return cppast::to_string(f.return_type());
	}
	throw std::runtime_error("unsupported function type");
}

std::string get_signature(const cppast::cpp_function_base & func) {
	// add the full signature (parameter types, and function cv/ref qualifiers)
	// todo: the parameter types are not fully namespace qualified
	return func.signature();
}

enum class xt_file_type
{
	// the interface in a header and the implementation in a .cpp_xt / .cu_xt file
	xt_split,
	// both the interface and the implementation in a header file
	xt_consolidated
};

int find_exported_templates(
	std::string file_path,
	const std::vector<std::string_view> & pp_defs,
	const std::vector<std::string_view> & include_dirs,
	const std::function<void(const std::string &, std::string_view)> & symbol_cb)
{
	//bug?: this doesn't work with backslashes !
	std::replace(file_path.begin(), file_path.end(), '\\', '/');

	std::optional< cppast::libclang_compile_config > config_opt;
	try {
		config_opt.emplace();
	} catch (std::invalid_argument e) {
		fmt::print("failed to construct cppast::libclang_compile_config because: {}\n", e.what());
		return 1;
	}
	auto & config = config_opt.value();

	//config.fast_preprocessing(true);
	config.fast_preprocessing(false);

	cppast::compile_flags flags;
	flags.set(cppast::compile_flag::ms_compatibility);
	flags.set(cppast::compile_flag::ms_extensions);
	config.set_flags(cppast::cpp_standard::cpp_17, flags);

	for (std::string_view include_dir : include_dirs) {
		// trailing slashes cause the quotes not to work
		while (!include_dir.empty() && include_dir.back() == '\\')
			include_dir.remove_suffix(1);
		config.add_include_dir(std::string { include_dir });
	}

	for (std::string_view macro : pp_defs) {
		config.define_macro(std::string { macro }, "");
	}

	cppast::libclang_parser parser;
	cppast::cpp_entity_index idx;

	auto parse = [&](const std::string &path) -> std::unique_ptr<cppast::cpp_file>
	{
		const char *err_fmt = "failed to parse file because: {}\n";
		try {
			auto ret = parser.parse(idx, path, config);
			if (parser.error() || !ret) {
				fmt::print(err_fmt, parser.error() ? "unspecified parser error" : "parser did not return a file");
				return {};
			}
			return ret;
		} catch (cppast::libclang_error err) {
			fmt::print(err_fmt, err.what());
			return {};
		}
	};

	auto file_ptr = parse(file_path);
	if(!file_ptr) {
		return 1;
	}

	const auto& file = *file_ptr;

	cppast::visit(file, [&](const cppast::cpp_entity& e, cppast::visitor_info info) {
		if (info.event == cppast::visitor_info::container_entity_exit)
			return true; // already handled

		// TODO: If an exported class/function is a member of an exported class
		// then we shouldn't emit a separate entry for that.

		if (e.kind() == cppast::cpp_function_template::kind() &&
			cppast::has_attribute(e, "export_template"))
		{
			auto& func_template = static_cast<const cppast::cpp_function_template&>(e);
			auto& func = func_template.function();

		#if 0
			std::string full_name =
				get_return_type(func) + " " +
				get_fully_qualified_name(func_template) +
				get_signature(func);
		#else
			std::string full_name = get_fully_qualified_name_without_templates(func);
		#endif

			fmt::print("function template: {}\n", full_name);

			char * flags = cppast::has_attribute(e, "dllexport") ? "fd" : "f";
			symbol_cb(full_name, flags);
		}

		if (e.kind() == cppast::cpp_class_template::kind() &&
			cppast::has_attribute(e, "export_template"))
		{
			auto& class_template = static_cast<const cppast::cpp_class_template&>(e);
			auto& class_ = class_template.class_();

			std::string full_name = get_fully_qualified_name_without_templates(class_);

			fmt::print("class template: {}\n", full_name);

			char * flags = cppast::has_attribute(e, "dllexport") ? "cd" : "c";
			symbol_cb(full_name, flags);
		}

		return true;
	});

	fmt::print("finished parsing\n");
	return 0;
}

int print_comp_usage() {
	fmt::print("usage: {} comp xt_file_path xt_inst_file_path "
		"preprocessor_definitions include_directories\n", tool_name);
	return 1;
}

int do_comp(int argc, const char *argv[]) {
	if (argc < 5) {
		return print_comp_usage();
	}

	auto header_path = argv[1];
	auto xt_inst_file_path = argv[2];

#if 0
	fmt::print("xt_inst_gen for '{}' in '{}'\n", header_path, xt_inst_file_path);
	fmt::print("preprocessor definitions: {}\n", argv[3]);
	fmt::print("include directories: {}\n", argv[4]);
#endif

	std::vector<std::string_view> pp_defs = split_string_to_views(argv[3], ';');
	std::vector<std::string_view> include_dirs = split_string_to_views(argv[4], ';');

	// first ensure that the path to the .xti exists
	if (std::error_code err;
		!fs::create_directories( fs::path { xt_inst_file_path }.remove_filename(), err )
	&& err) {
		fmt::print("failed to create the path to the .xti file because: {}\n", err.message());
		return 1;
	}

	try {
		// open the file for reading/appending and create it if it doesn't already exist
		xt_inst_file_reader_writer xti_rw { xt_inst_file_path };
		// read the old .xti file if it exists, then overwrite it with new data
		xt_inst_file xti_old = xti_rw.read();
		xt_inst_file xti;

		xti.append_version();

		// note: the linker needs to have a list of exported symbols
		// to match the unresolved symbols against, and ideally
		// that information could be embedded in the implementation object files
		// but without relying on a custom compiler/linker
		// we write the exported symbols to .xti files generated from the interface,
		// let the build system pass the paths to all the .xti files to the link tool
		// then if the link tool appends the template instantiations to the .xti files,
		// the implemenation files that include the .xti should get rebuilt next time

		// todo: now we parse the interface header, but we don't really know 
		// if definitions are actually provided in the impelementation files.
		// might be better to parse the implementation file and only emit the defined symbols ?
		int ret = find_exported_templates(header_path, pp_defs, include_dirs, 
			[&](const std::string & symbol, std::string_view flags) {
				xti.append_exported_symbol(symbol, flags);
			});

		if (ret) return ret;

		// todo: this keeps the old instantiated symbols, but if the signature changed for any of them ?
		for (string_view_ref symbol : xti_old.get_instantiated_symbols()) {
			xti.append_instantiated_symbol(symbol.get());
		}

		if (xti_old.contents != xti.contents) {
			fmt::print("writing new contents to {}\n", xt_inst_file_path);
			xti_rw.write(xti);
		} else {
			fmt::print("nothing changed in {}\n", xt_inst_file_path);
			// we should still update the modification time for the file
			// to make sure the build system knows that the file is up to date
			fs::last_write_time(xt_inst_file_path, fs::file_time_type::clock::now());
		}
		return 0;
	} catch (std::runtime_error e) {
		fmt::print("caught exception: {}\n", e.what());
		return 1;
	}
}

} // namespace xt