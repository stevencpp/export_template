#include "C.h"
#include <C.xti>

#include "D.h"

#include <stdio.h>

namespace C
{
	
	template <typename T>
	void C<T>::foo() {
		printf("%s\n", __FUNCSIG__);
		D::D<T>::foo<int*>();
	}
	
} // namespace C