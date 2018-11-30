#include "B.h"
// to make it possible to build A without B
// B's .xti should only be included if it was generated
#if __has_include(<../B/B.xti>)
#include <../B/B.xti>
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef STRICT
#define STRICT
#endif
#include <windows.h>

#include <stdexcept>
#include <string>

namespace B {

	template<typename F>
	F load_lib_function(const char * lib_path, const char * function_symbol) {
		HMODULE hmod = LoadLibraryExA(lib_path, NULL, 0);
		if (!hmod) {
			int err = GetLastError();
			throw std::runtime_error(
				std::string("failed to load ") + lib_path + ": error " + std::to_string(err)
			);
		}

		auto function = GetProcAddress(hmod, function_symbol);
		if (!function) {
			int err = GetLastError();
			throw std::runtime_error(
				std::string("unable to resolve ") + function_symbol + " in " + lib_path + ": error " + std::to_string(err)
			);
		}

		return reinterpret_cast<F>(function);
	}

	template<typename T>
	auto Interface<T>::create(const char * path) -> std::shared_ptr<Interface> {
		auto function = load_lib_function< decltype(&create) >(path, __FUNCDNAME__);
		return function(path);
	}

} // namespace B