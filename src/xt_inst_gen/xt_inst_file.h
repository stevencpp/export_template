#pragma once

// interface includes
#include <fstream>
#include <string_view>

// implementation includes
#include "xt_util.h"
#include <fmt/core.h>
#include <fmt/format.h>

namespace xt
{

// structure represanting a substring
// that remains valid even if the referenced string is moved elsewhere
struct string_range
{
	std::size_t ofs, len;

	string_range(std::size_t ofs, std::size_t len) :
		ofs(ofs), len(len) {}

	string_range(const std::string & str_ref, std::string_view view)
	{
		set_view(str_ref, view);
	}

	// note: the return value is invalidated if the referenced string
	// is moved or reallocated due to size changes 
	std::string_view get(const std::string & str_ref) {
		return { &str_ref[ofs], len };
	}

	std::string get_str(const std::string & str_ref) {
		return str_ref.substr(ofs, len);
	}

	void set_view(std::size_t ofs, std::size_t len) {
		this->ofs = ofs;
		this->len = len;
	}

	void set_view(const std::string & str_ref, std::string_view view) {
		if (view.empty()) {
			ofs = len = 0;
		} else {
			// assert( ! str_ref.empty() );
			ofs = &view[0] - &str_ref[0];
			len = view.size();
		}
	}
};

// structure represanting a substring
// that is no longer valid if the referenced string is moved
// but remains valid if data is appended to the referenced string
struct string_view_ref : public string_range
{
	std::string & str_ref;
	
	// note: the return value is invalidated if the referenced string
	// is moved or reallocated due to size changes
	std::string_view get() {
		return string_range::get(str_ref);
	}

	std::string get_str() {
		return string_range::get_str(str_ref);
	}
};

template<typename Iter, typename OutputType = string_view_ref>
struct string_view_deref_range {
	std::string & _str_ref;
	Iter _begin, _end;

	struct iterator {
		std::string & _str_ref;
		Iter _itr;

		using iterator_category = typename Iter::iterator_category;
		using value_type = OutputType;
		using difference_type = typename Iter::difference_type;
		using pointer = value_type *;
		using reference = value_type &;

		bool operator != (const iterator & iter) {
			return _itr != iter._itr;
		}

		iterator operator ++ () {
			++_itr;
			return *this;
		}

		value_type operator * () {
			return { *_itr, _str_ref };
		}
	};

	iterator begin() {
		return { _str_ref, _begin };
	}

	iterator end() {
		return { _str_ref, _end };
	}

	bool contains(std::string_view s) {
		for (auto ref : *this) {
			if (ref.get() == s) {
				return true;
			}
		}
		return false;
	}
};

struct xt_inst_file
{
	std::string contents;
	// views into the contents:

	string_range version { 0, 0 };

	struct exp_symbol { string_range flags, name; };

	struct exp_symbol_ref : public exp_symbol {
		std::string & str_ref;
		std::string_view get_flags() { return flags.get(str_ref); }
		bool has_flag(const char c) { return get_flags().find(c) != std::string_view::npos;	}
		bool has_flags(std::string_view cs) {
			for (const char c : cs) if (!has_flag(c)) return false;
			return true;
		}
		std::string_view get_name() { return name.get(str_ref); }
	};

	std::vector<exp_symbol> exported_symbols;
	std::vector<string_range> instantiated_symbols;

	// return a range of string_view_refs
	auto get_exported_symbols() {
		return string_view_deref_range<decltype(exported_symbols)::iterator, exp_symbol_ref> { 
			contents, exported_symbols.begin(), exported_symbols.end()
		};
	}

	// return a range of string_view_refs
	auto get_instantiated_symbols() {
		return string_view_deref_range<decltype(instantiated_symbols)::iterator> {
			contents, instantiated_symbols.begin(), instantiated_symbols.end()
		};
	}

	string_range append_string(std::string_view str) {
		std::size_t pos = contents.size();
		contents += str;
		return { pos, str.size() };
	}

	static constexpr std::string_view
		version_prefix = "// ver ",
		export_prefix = "// exp ",
		instantiation_prefix = "template ";

	static constexpr std::string_view
		current_version = "1";

	void append_version() {
		contents += version_prefix;
		contents += current_version;
		contents += '\n';
	}

	void append_exported_symbol(std::string_view symbol, std::string_view flags) {
		contents += export_prefix;
		auto flags_range = append_string(flags);
		contents += ' ';
		auto name_range = append_string(symbol);
		contents += '\n';
		exported_symbols.emplace_back(exp_symbol { flags_range, name_range });
	}

	void append_instantiated_symbol(std::string_view symbol) {
		contents += instantiation_prefix;
		auto symbol_range = append_string(symbol);
		contents += ";\n";
		instantiated_symbols.emplace_back(symbol_range);
	}
};

struct xt_inst_file_reader_writer
{
	std::string file_name;
	std::fstream f;
	xt_inst_file_reader_writer(std::string_view file_name) : 
		file_name(file_name), 
		f { file_name, std::ios::in | std::ios::out | std::ios::app | std::ios::binary }
	{
		// note: with just in|out|bin it fails if the file does not already exist
		// but with in|out|app|bin a new file is created in that case
		if (!f) {
			throw std::runtime_error(fmt::format("cannot open file {}", file_name));
		}
		f.seekg(0, std::ios::beg);
	}

	xt_inst_file read() {
		xt_inst_file ret;
		ret.contents = get_file_contents(f);

		const std::size_t nr_lines = std::count(ret.contents.begin(), ret.contents.end(), '\n');
		ret.exported_symbols.reserve(nr_lines);
		ret.instantiated_symbols.reserve(nr_lines);

		std::string_view version;

		auto it = ret.contents.begin();
		while (it != ret.contents.end()) {
			auto it_end = std::find(it, ret.contents.end(), '\n');
			std::string_view line = string_view_from_range(it, it_end);
			if (!line.empty() && line.back() == '\r')
				line.remove_suffix(1);

			auto check_and_remove = [&](std::string_view prefix) {
				if (string_starts_with(line, prefix)) {
					line.remove_prefix(prefix.size());
					return true;
				}
				return false;
			};

			if (check_and_remove(xt_inst_file::version_prefix)) {
				version = line;
			} else if (check_and_remove(xt_inst_file::export_prefix)) {
				std::string_view flags = line.substr(0, line.find(' '));
				std::string_view name = line.substr(flags.size() + 1);
				ret.exported_symbols.emplace_back(xt_inst_file::exp_symbol {
					string_range { ret.contents, flags },
					string_range { ret.contents, name }
				});
			} else if (check_and_remove(xt_inst_file::instantiation_prefix)) {
				line.remove_suffix(1); // for the ; at the end
				ret.instantiated_symbols.emplace_back(ret.contents, line);
			} else {
				// just ignore it
			}
			if (it_end == ret.contents.end()) break;
			it = it_end + 1;
		}

		if (!ret.contents.empty() && version != xt_inst_file::current_version) {
			fmt::print("note: version mismatch, ignoring file contents\n");
			ret = {};
		}
		
		return ret;
	}

	// overwrite the file with the given xti
	void write(const xt_inst_file & xti) {
		// it'd be nice to be able to use _chsize / ftruncate / SetFileSize,
		// that way we could avoid reopening the file,
		// but those require a file descriptor and there's no way to get
		// the fd of an fstream (afaik)
		if (f.tellp() != 0) {
			f.close();
			f.open(file_name, std::ios::out | std::ios::binary);
			if (!f) {
				throw std::runtime_error(fmt::format("cannot reopen file {}", file_name));
			}
		}
		f << xti.contents;
	}

	// write out additional lines appended to xti after it was read
	void append_diff(const xt_inst_file & xti) {
		auto pos = f.tellg();
		f.seekp(pos);
		f.write(&xti.contents[pos], xti.contents.size() - pos);
	}
};

} // namespace xt