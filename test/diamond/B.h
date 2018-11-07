#pragma once

#pragma warning (push)
#pragma warning (disable:5030)
namespace B
{
	struct B
	{
		template<typename T>
		[[export_template]] static void foo();
	};
	
} // namespace B
#pragma warning (pop)