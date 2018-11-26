#include "A.h"
#include "B.h"
#include "C.h"
#include "D.h"

//#include <string_view>

int main()
{
	//std::vector<std::string_view> v = { "asdf" }; // doesn't work yet
	std::vector<int> v = { 1, 2, 3, 4 };
	A::foo(v);
	B::B::foo<bool>();
	
	int x = 2;
	C::C<char> c { x };
	c.foo();
	c(x);
	if(c > x) x = c << x;
	if (std::move(c) < x) c = x;

	D::D<double>::foo<double>();
}