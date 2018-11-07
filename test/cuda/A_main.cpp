#include "A.h"
#include "B.h"

#include <stdio.h>

template<typename T, typename F>
bool test_cuda(F addWithCuda)
{
	const int arraySize = 5;
	const T a[arraySize] = { 1, 2, 3, 4, 5 };
	const T b[arraySize] = { 10, 20, 30, 40, 50 };
	T c[arraySize] = { 0 };

	// Add vectors in parallel.
	if (!addWithCuda(c, a, b, arraySize)) {
		fprintf(stderr, "addWithCuda failed!");
		return false;
	}

	printf("[1,2,3,4,5] + [10,20,30,40,50] = [%d,%d,%d,%d,%d]\n",
		(int)c[0], (int)c[1], (int)c[2], (int)c[3], (int)c[4]);

	return true;
}

int main()
{
	if (!test_cuda<short>(A::addWithCuda<short>))
		return 1;
	if (!test_cuda<int>(B::addWithCuda<int>))
		return 1;
}