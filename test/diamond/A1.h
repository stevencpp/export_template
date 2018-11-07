#pragma once

#pragma warning (push)
#pragma warning (disable:5030)
namespace A
{
	
	template<typename T>
	[[export_template]] void bar(const T &);
	
} // namespace A
#pragma warning (pop)