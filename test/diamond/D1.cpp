#include "D1.h"
#include <D1.xti>

#include <stdio.h>

namespace D1
{
	template <typename T>
	template <typename U>
	template <typename V>
	auto D1<T>::D11<U>::foo(D11 *) -> D11 * {
		printf("%s\n", __FUNCSIG__);
		return this;
	}
} // namespace D1