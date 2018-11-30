#include "E.h"
#include <E.xti>

#include "E1.h"

#include <stdio.h>

namespace E {

	__declspec(dllexport) void exported_function() {
		E1::foo<bool>();
	}

	template <typename T>
	void foo() {
		printf("%s\n", __FUNCSIG__);
		exported_function();
	}

} // namespace E