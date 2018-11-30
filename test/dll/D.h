#pragma once

#if defined(D_DLL_EXPORT)
#define D_DLL_SPEC __declspec(dllexport)
#elif defined(D_DLL_IMPORT)
#define D_DLL_SPEC __declspec(dllimport)
#else
#define D_DLL_SPEC
#endif

#pragma warning (push)
#pragma warning (disable:5030)
namespace D
{
	template<typename T>
	struct D_DLL_SPEC [[export_template]] D1
	// note: the order matters here since the reverse,
	// [[export_template]] D_DLL_SPEC, does not compile in clang
	{
		static void foo();
		
		// The following is a particularly tricky case where
		// the mangled symbol name of bar is not prefixed with __imp_
		// so there's no way to tell from the unresolved symbol for bar
	    // that it's a dllexport.
		template <typename U>
		static void bar();
		
		struct D3
		{
			D_DLL_SPEC static void foo();
			template<typename U>
			D_DLL_SPEC static void bar();
		};
		
		template<typename U>
		struct D_DLL_SPEC D4
		{
			static void foo();
		};
	};
	
	struct D2
	{
		template<typename T>
		[[export_template]] D_DLL_SPEC static void foo();
	};

} // namespace D
#pragma warning (pop)

#undef D_DLL_SPEC