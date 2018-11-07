#include "B.h"
#include <B.xti>

#include "D.h"

#include <stdio.h>

namespace B
{
	
	template <typename T>
	void B::foo() {
		printf("%s\n", __FUNCSIG__);
		D::D<T>::foo<int>();
	}
	
} // namespace B