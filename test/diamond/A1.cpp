#include "A1.h"
#include <A1.xti>

#include <stdio.h>
#include <iostream>

namespace A
{
	
	template <typename T>
	void bar(const T & t) {
		printf("%s\n", __FUNCSIG__);
		std::cout << t << std::endl;
	}
	
} // namespace A