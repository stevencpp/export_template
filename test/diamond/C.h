#pragma once

#pragma warning (push)
#pragma warning (disable:5030)
namespace C
{
	template<typename T>
	struct [[export_template]] C
	{
		static void foo();
		
		template<typename U>
		explicit C(U);
		
		template<typename U>
		operator U () const;
		
		template<typename U>
		void operator()(U) &;
		
		template<typename U>
		C& operator =(U);
		
		template<typename U>
		bool operator <(U) &&;

		template<typename U>
		C& operator <<(U);
		
		virtual bool operator >(int);
		
		~C();
	};
	
} // namespace C
#pragma warning (pop)