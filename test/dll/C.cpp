#include "C.h"
#include <C.xti>

#include <stdio.h>

namespace C
{
	
	template <typename T>
	void C<T>::foo() {
		printf("%s\n", __FUNCSIG__);
	}
	
	template<typename T>
	template<typename U>
	void C<T>::bar() {
		printf("%s\n", __FUNCSIG__);
	}
	
} // namespace C