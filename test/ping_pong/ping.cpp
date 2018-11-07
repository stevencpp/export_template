#include "ping.h"
#include <ping.xti>

#include "pong.h"

#include <stdio.h>

namespace ping
{

	template <int N, int I>
	void foo() {
		printf("%s\n", __FUNCSIG__);
		if constexpr (I < N) {
			pong::foo<N, I + 1>();
		}
	}

} // namespace ping