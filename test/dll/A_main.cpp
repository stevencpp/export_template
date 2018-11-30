#include "B.h"
#include "C.h"

#define DLL_IMPORT
#include "D.h"

int main()
{
	auto impl_ptr = B::Interface<float>::create("B.dll");
	impl_ptr->foo();
	C::C<int>::foo();
	D::D1<float>::foo();
	//D::D1<int>::bar<float>(); // doesn't work yet
	D::D1<char>::D3::foo();
	D::D1<double>::D3::bar<int>();
	D::D1<short>::D4<int>::foo();
	D::D2::foo<int>();
}