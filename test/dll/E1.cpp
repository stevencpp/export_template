#include "E1.h"
#include <E1.xti>

#include <stdio.h>

namespace E1
{
	
	template <typename T>
	void foo() {
		printf("%s\n", __FUNCSIG__);
	}
	
} // namespace E1