#pragma once

#pragma warning (push)
#pragma warning (disable:5030)
namespace D1
{
	template<typename T>
	struct [[export_template]] D1
	{
		template<typename U>
		class D11
		{
			template<typename V>
			[[export_template]] D11 * foo(D11 * p = nullptr);
		public:
			template<typename V>
			static void foo() {
				D11 d11;
				d11.foo<V>(&d11);
			}
		};
	};
	
} // namespace D1
#pragma warning (pop)