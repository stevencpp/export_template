#pragma once

#pragma warning (push)
#pragma warning (disable:5030)
namespace pong
{

	template<int N, int I = 1>
	[[export_template]] void foo();

} // namespace pong
#pragma warning (pop)