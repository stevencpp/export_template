#include "A.h"
#include <A.xti>

#include "A1.h"

#include <stdio.h>

namespace A
{
	
	template <typename T>
	void foo(const std::vector<T> & v) {
		printf("%s\n", __FUNCSIG__);
		if(!v.empty()) bar(v.front());
	}
	
} // namespace A