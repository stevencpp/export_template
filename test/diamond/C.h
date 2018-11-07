#pragma once

#pragma warning (push)
#pragma warning (disable:5030)
namespace C
{
	template<typename T>
	struct [[export_template]] C
	{
		static void foo();
	};
	
} // namespace C
#pragma warning (pop)