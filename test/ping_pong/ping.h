#pragma once

#pragma warning (push)
#pragma warning (disable:5030)
namespace ping
{

	template<int N, int I = 1>
	[[export_template]] void foo();

} // namespace ping
#pragma warning (pop)