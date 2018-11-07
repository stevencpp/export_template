#pragma once

#include <vector>

#pragma warning (push)
#pragma warning (disable:5030)
namespace A
{
	
	template<typename T>
	[[export_template]] void foo(const std::vector<T> &);
	
} // namespace A
#pragma warning (pop)