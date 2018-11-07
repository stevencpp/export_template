#pragma once

#pragma warning (push)
#pragma warning (disable:5030)
namespace D
{
	
	template<typename T>
	struct D
	{
		template<typename U>
		[[export_template]] static void foo();
	};

} // namespace D
#pragma warning (pop)