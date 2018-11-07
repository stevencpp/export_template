#include "pong.h"
#include <pong.xti>

#include "ping.h"

#include <stdio.h>

namespace pong
{

	template <int N, int I>
	void foo() {
		printf("%s\n", __FUNCSIG__);
		if constexpr (I < N) {
			ping::foo<N, I + 1>();
		}
	}

} // namespace pong