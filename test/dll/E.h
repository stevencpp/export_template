#pragma once

#if defined(E_DLL_EXPORT)
#define E_DLL_SPEC __declspec(dllexport)
#elif defined(E_DLL_IMPORT)
#define E_DLL_SPEC __declspec(dllimport)
#else
#define E_DLL_SPEC
#endif

#pragma warning (push)
#pragma warning (disable:5030)
namespace E
{
	template <typename T>
	[[export_template]] E_DLL_SPEC void foo();
} // namespace E
#pragma warning (pop)

#undef E_DLL_SPEC