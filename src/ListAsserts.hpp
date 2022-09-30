#pragma once
#include "Types.hpp"

/*
ATTRIBUTION: Using recursive inheritance + variadic templates for arbitrarily
	long compile-time lists:
	https://eli.thegreenplace.net/2014/variadic-templates-in-c/

Attempts to make compile-time type checks for arbitrary list of types in
a concise manner. The first type needs to be an assertion struct. When
instantiated, its constructor runs any relevant static_assert statements.
For example:
	ListAssert<AssertPrimitive, double, int, bool, float, char> a;
	ListAssert<AssertPowerOfTwoSize, int, double> b;
	ListAssert<AssertPowerOfTwoSize, IVec3> c;  // Should fail.

NOTE: Still not happy with this. Creates a useless 1-byte
struct and prints obtuse error messages if something goes wrong.
*/

//-----------------------------------------------
// List Assert
//-----------------------------------------------
template <template<typename> class AssertStruct, class... Types> 
struct ListAssert{
	// No fields
};

template <template<typename> class AssertStruct, class Type, class... Types>
struct ListAssert<AssertStruct, Type, Types...> : 
	ListAssert<AssertStruct, Types...> {
	constexpr ListAssert(){
		/*
		Attempts to create a struct of the peeled-off type here. 
		If any of the static_assert statements in its constructor fail, 
		we get an error.
		*/

		AssertStruct<Type> attempted_construction;
	}
};


//-----------------------------------------------
// Assertion Structures
//-----------------------------------------------
template <class T>
struct AssertPrimitive{
	/*
	For structures that can be blindly copy-pasted in chunks rather than
	needing to invoke copy-constructors for every element.
	*/

	constexpr AssertPrimitive(){
		static_assert(std::is_trivial<T>::value);
		static_assert(std::is_default_constructible<T>::value);
		static_assert(std::is_trivially_copyable<T>::value);
		static_assert(std::is_pod<T>::value);
	}
};

template <class T>
struct AssertUniquePrimitive{
	/*
	Primitives that must also have no floating point components or
	padding. For cases where I want to be able to check for equality over
	arrays of the given type using a bytewise check like memcmp.
	*/

	constexpr AssertUniquePrimitive(){
		AssertPrimitive<T> primitive_test;
		static_assert(std::has_unique_object_representations<T>::value);
	}
};

template <class T>
struct AssertPowerOfTwoSize{
	/*
	Asserts that the type is a power of two.
	*/

	constexpr AssertPowerOfTwoSize(){
		constexpr bool is_power_of_2 = 
			(sizeof(T) != 0) && 
			(sizeof(T) & (sizeof(T) - 1)) == 0;
		static_assert(is_power_of_2);
	}
};


//-----------------------------------------------
// Example Usage
//-----------------------------------------------
inline void assertSimpleStructs(){
	/*
	Hacky example of how to do these asserts without creating a
	useless 1-byte object.
	*/

	ListAssert<AssertPrimitive, double, int, bool, float, char> a;
	ListAssert<AssertUniquePrimitive, int, char, bool> b;
	ListAssert<AssertPowerOfTwoSize, int, double> c;
}

