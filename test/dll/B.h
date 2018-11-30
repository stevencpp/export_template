#pragma once

#include <memory>

#if defined(DLL_EXPORT)
#define DLL_SPEC __declspec(dllexport)
#elif defined(DLL_IMPORT)
#define DLL_SPEC __declspec(dllimport)
#else
#define DLL_SPEC
#endif

#pragma warning (push)
#pragma warning (disable:5030)
namespace B
{
	template<typename T>
	struct [[export_template]] Interface
	{
		virtual void foo() = 0;
		virtual ~Interface() {};

		static DLL_SPEC auto create(const char * path) -> std::shared_ptr<Interface>;
	};

} // namespace B
#pragma warning (pop)

#undef DLL_SPEC