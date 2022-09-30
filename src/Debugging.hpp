#pragma once

//-------------------------------------------------------------------------------------------------
// TODO: Try to get this or an equivalent method working. It'd be cleaner than the current method.
// ATTRIBUTION: https://stackoverflow.com/a/62523297
//		User: https://stackoverflow.com/users/11086268/clyne
//-------------------------------------------------------------------------------------------------
/*
"If the types and order of the object's members are known, and if all members have the same 
	access specifier (e.g. public), and if the object lacks a vtable, you can rebuild the 
	object's structure within a helper class and use the size of that." -clyne
*/
template<typename... Ts>
struct struct_traits {
	static constexpr std::size_t size() {
		return sizeof(struct_builder<Ts...>);
	}

	private:
		template<typename First, typename... Rest>
		struct struct_builder : struct_builder<Rest...> {
			First v;
		};

		template<typename First>
		struct struct_builder<First> {
			First v;
		};

	// Below: addition to work with structures declared with alignas().
	public:
		template<std::size_t alignas_>
		static constexpr std::size_t size() {
			return sizeof(aligned_struct_builder<alignas_, Ts...>);
		}

	private:
		template<std::size_t alignas_, typename First, typename... Rest>
		struct alignas(alignas_) aligned_struct_builder : aligned_struct_builder<alignas_, Rest...> {
			First v;
		};
		template<std::size_t alignas_, typename First>
		struct alignas(alignas_) aligned_struct_builder<alignas_, First> {
			First v;
		};
};