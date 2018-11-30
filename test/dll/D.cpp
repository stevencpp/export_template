#define DLL_EXPORT
#include "D.h"
#include <D.xti>

#include <stdio.h>

namespace D {

	template<typename T>
	void D1<T>::foo() {
		printf("%s\n", __FUNCSIG__);
	}
	
	template<typename T>
	template<typename U>
	void D1<T>::bar() {
		printf("%s\n", __FUNCSIG__);
	}
	
	template<typename T>
	void D1<T>::D3::foo() {
		printf("%s\n", __FUNCSIG__);
	}
	
	template<typename T>
	template<typename U>
	void D1<T>::D3::bar() {
		printf("%s\n", __FUNCSIG__);
	}
	
	template<typename T>
	template<typename U>
	void D1<T>::D4<U>::foo() {
		printf("%s\n", __FUNCSIG__);
	}
	
	template<typename T>
	void D2::foo() {
		printf("%s\n", __FUNCSIG__);
	}
	
	__declspec(dllexport) void dummy_func() {}

} // namespace D