#pragma once

#pragma warning (push)
#pragma warning (disable:5030)
namespace A {
	template < typename T >
	[[export_template]] bool addWithCuda(T *c, const T *a, const T *b, int size);
} // namespace A
#pragma warning (pop)