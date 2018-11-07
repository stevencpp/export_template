#pragma once

#pragma warning (push)
#pragma warning (disable:5030)
namespace B {
	template < typename T >
	[[export_template]] bool addWithCuda(T *c, const T *a, const T *b, int size);
} // namespace B
#pragma warning (pop)