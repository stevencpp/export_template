#pragma once

#include <memory>

#if defined(B_DLL_EXPORT)
#define B_DLL_SPEC __declspec(dllexport)
#elif defined(B_DLL_IMPORT)
#define B_DLL_SPEC __declspec(dllimport)
#else
#define B_DLL_SPEC
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

		static B_DLL_SPEC auto create(const char * path) -> std::shared_ptr<Interface>;
	};

} // namespace B
#pragma warning (pop)

#undef B_DLL_SPEC