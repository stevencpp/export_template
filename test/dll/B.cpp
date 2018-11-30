#define DLL_EXPORT
#include "B.h"
#include <B.xti>

#include "C.h"

#include <stdio.h>

namespace B {

	template<typename T>
	struct Impl : public Interface<T>
	{
		void foo() override {
			printf("%s\n", __FUNCSIG__);
			C::C<T>::foo();
			C::C<const T&>::bar<T[3]>();
		}
	};

	template<typename T>
	auto Interface<T>::create(const char * path) -> std::shared_ptr<Interface> {
		// shared_ptr has a type erased deleter so this should be safe to use across dll boundaries
		// https://stackoverflow.com/questions/345003/is-it-safe-to-use-stl-tr1-shared-ptrs-between-modules-exes-and-dlls
		return std::make_shared< Impl<T> >();
	};

	//todo: normally A should not be linked with B
	//but until I figure out how to do that with CMake
	//we can use this dummy export symbol here to make sure B.lib exists
	//to allow A to otherwise link and show unresolved symbols
	// before instatiations are injected into B
	__declspec(dllexport) void dummy_func() {}

} // namespace B