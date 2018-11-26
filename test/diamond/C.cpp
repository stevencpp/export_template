#include "C.h"
#include <C.xti>

#include "D.h"

#include <stdio.h>

namespace C
{
	template <typename T>
	void C<T>::foo() {
		printf("%s\n", __FUNCSIG__);
		D::D<T>::foo<int*>();
	}
	
	template <typename T>
	template <typename U>
	C<T>::C(U) {
		printf("%s\n", __FUNCSIG__);
	}
	
	template <typename T>
	template <typename U>
	C<T>::operator U () const {
		printf("%s\n", __FUNCSIG__);
		return U {};
	}
	
	template <typename T>
	template <typename U>
	void C<T>::operator()(U) & {
		printf("%s\n", __FUNCSIG__);
	}
	
	template <typename T>
	template <typename U>
	auto C<T>::operator =(U) -> C& {
		printf("%s\n", __FUNCSIG__);
		return *this;
	}
	
	template <typename T>
	template <typename U>
	bool C<T>::operator <(U) && {
		printf("%s\n", __FUNCSIG__);
		return true;
	}

	template <typename T>
	template <typename U>
	auto C<T>::operator <<(U) -> C& {
		printf("%s\n", __FUNCSIG__);
		return *this;
	}
	
	template <typename T>
	bool C<T>::operator >(int) {
		printf("%s\n", __FUNCSIG__);
		return true;
	}
	
	template <typename T>
	C<T>::~C() {
		printf("%s\n", __FUNCSIG__);
	}
	
} // namespace C