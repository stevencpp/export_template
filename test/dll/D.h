#pragma once

#if defined(DLL_EXPORT)
#define DLL_SPEC __declspec(dllexport)
#elif defined(DLL_IMPORT)
#define DLL_SPEC __declspec(dllimport)
#else
#define DLL_SPEC
#endif

#pragma warning (push)
#pragma warning (disable:5030)
namespace D
{
	template<typename T>
	struct [[export_template]] DLL_SPEC D1
	{
		static void foo();
		
		template <typename U>
		static void bar();
		
		struct D3
		{
			DLL_SPEC static void foo();
			template<typename U>
			DLL_SPEC static void bar();
		};
		
		template<typename U>
		struct DLL_SPEC D4
		{
			static void foo();
		};
	};
	
	struct D2
	{
		template<typename T>
		[[export_template]] DLL_SPEC static void foo();
	};

} // namespace D
#pragma warning (pop)

#undef DLL_SPEC