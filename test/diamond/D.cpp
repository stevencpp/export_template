#include "D.h"
#include <D.xti>

#include "D1.h"

#include <stdio.h>

namespace D
{
	void dummy(const char & a = '2') {}
	
	template <typename T>
	template <typename U>
	void D<T>::foo() {
		printf("%s\n", __FUNCSIG__);
		D1::D1<T>::D11<U>::foo<decltype(dummy)>();
	}
	
} // namespace D