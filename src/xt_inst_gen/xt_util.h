#pragma once

//interface includes
#include <vector>
#include <string>
#include <string_view>
#include <optional>

// implementation includes
#include <fstream>
#include <filesystem>

namespace xt
{
	template< typename Iter >
	std::string_view string_view_from_range(Iter a, Iter b) {
		return { &*a, (std::size_t)(b - a) };
	}

	// returns list of non-empty components delimited by 'delim'
	static std::vector<std::string_view> split_string_to_views(std::string_view str, const char delim) {
		std::vector<std::string_view> ret;
		ret.reserve(std::count(str.begin(), str.end(), delim) + 1);
		auto it_from = str.begin();
		while (it_from != str.end()) {
			auto it_to = std::find(it_from, str.end(), delim);
			if(it_from != it_to)
				ret.emplace_back(string_view_from_range(it_from, it_to));
			if (it_to == str.end()) break;
			it_from = it_to + 1;
		}
		return ret;
	}

	static std::vector<std::string> split_string(std::string_view str, const char delim) {
		std::vector<std::string> ret;
		ret.reserve(std::count(str.begin(), str.end(), delim) + 1);
		auto it_from = str.begin();
		while (it_from != str.end()) {
			auto it_to = std::find(it_from, str.end(), delim);
			if (it_from != it_to)
				ret.emplace_back(it_from, it_to);
			if (it_to == str.end()) break;
			it_from = it_to + 1;
		}
		return ret;
	}

	template <typename E, typename F, typename... Args>
	auto try_catch(F func, Args&&... args)
		-> std::pair< decltype(func()), std::optional<E> >
	{
		try {
			return { func() , std::optional<E>{} };
		} catch (E e) {
			return { decltype(func()) { std::forward<Args>(args)... }, std::move(e) };
		}
	}

	static std::string get_file_contents(std::fstream & f)
	{
		std::string contents;
		f.seekg(0, std::ios::end);
		contents.resize(f.tellg());
		f.seekg(0, std::ios::beg);
		f.read(&contents[0], contents.size());
		return contents;
	}

	static std::optional<std::string> get_file_contents(std::string_view file_name)
	{
		std::fstream in(file_name, std::ios::in | std::ios::binary);
		if (!in) return {};
		return get_file_contents(in);
	}

	static std::string in_quotes(std::string_view s) {
		std::string ret;
		ret.reserve(s.size() + 3); // quotes + null terminator
		ret = '\"';
		//ret = " \"";
		ret += s;
		ret += '\"';
		return ret;
	}

	// return true if a starts with b
	static bool string_starts_with(const std::string_view & a, const std::string_view & b) noexcept {
		if (a.size() < b.size())
			return false;
		return std::string_view { &a[0], b.size() } == b;
	}

	template<typename T, typename Func>
	auto tuple_map(T && t, Func&& f) {
		// TODO: ...
		return std::tuple { f(std::get<0>(t)), f(std::get<1>(t)) };
	}

	template<typename T, typename Func>
	void tuple_apply(T && t, Func&& f) {
		// TODO: ...
		f(std::get<0>(t));
		f(std::get<1>(t));
	};

	template<typename... Ranges>
	struct zip_ranges
	{
		std::tuple<Ranges& ...> ranges;

		zip_ranges(Ranges&... ranges) : ranges { ranges... }  {

		}

		struct iterator
		{
			std::tuple<typename Ranges::iterator...> itrs;

			using iterator_category = std::common_type_t< typename Ranges::iterator::iterator_category ... >;
			using value_type = std::tuple<typename Ranges::value_type...>;
			using difference_type = std::common_type_t< typename Ranges::iterator::difference_type ... >;
			using pointer = value_type * ;
			using reference = value_type & ;

			bool operator != (const iterator & iter) {
				return itrs != iter.itrs;
			}

			iterator operator ++ () {
				tuple_apply(itrs, [](auto &itr) { ++itr; });
				return *this;
			}

			value_type operator * () {
				return tuple_map(itrs, [](auto &itr) { return *itr; });
			}
		};

		iterator begin() {
			return { tuple_map(ranges, [](auto& r) { return std::begin(r); }) };
		}

		iterator end() {
			return { tuple_map(ranges, [](auto& r) { return std::end(r); }) };
		}

	};
}