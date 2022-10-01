#pragma once

#include <cstring>
#include <cassert>
#include <stdio.h>
#include <math.h>
#include <initializer_list>
#include <type_traits>

#include "Types.hpp"
#include "Constants.hpp"
#include "ListAsserts.hpp"


//--------------------------------------------------------------------------------------------------
// Helper Functions
//--------------------------------------------------------------------------------------------------
inline Bytes8 tripletHash(int x, int y, int z){
	/*
	Key layout (bytes). X,Y,Z are ints, . is empty
	XXXXZZZZ
	xor with
	..YYYY..

	WARNING: Bad hash function. Not tested. Done purely to get something working.
	TODO: Test and optimize.
	*/

	Bytes8 output = x;
	output = output << (BITS_PER_BYTE * 4); // Slide X coord into the left half
	output |= z; // Place Z coord in the right half

	// Slide Y halfway over so it overlaps X and Z, then XOR it
	Bytes8 temp = y;
	output ^= (temp << (BITS_PER_BYTE * 2));

	return output;
}

inline float degreesToRadians(float angle){
	return (angle * PI) / 180;
}

inline float radiansToDegrees(float angle){
	return (angle * 180) / PI;
}

inline bool matchesWithinTolerance(float a, float b){
	/*
	WARNING: Testing float equality with an absolute epsilon is unsafe.
	TODO: Better float equality testing
	*/

	return abs(a - b) < ARBITRARY_EPSILON;
}

template <typename T>
inline T max(T a, T b){
	if(a > b){
		return a;
	}else{
		return b;
	}
}

template <typename T>
inline T min(T a, T b){
	if(a < b){
		return a;
	}else{
		return b;
	}
}

template Int32 max<Int32>(Int32 a, Int32 b);
template Int64 max<Int64>(Int64 a, Int64 b);
template float max<float>(float a, float b);

template Int32 min<Int32>(Int32 a, Int32 b);
template Int64 min<Int64>(Int64 a, Int64 b);
template float min<float>(float a, float b);


//--------------------------------------------------------------------------------------------------
// Vector Structures
//--------------------------------------------------------------------------------------------------
struct FVec2;
struct FVec3;
struct IVec2;
struct IVec3;

//--------------------------------------
// FVec2
//--------------------------------------
struct FVec2{
	float x, y;

	bool isZero() const;
	float length() const;
	FVec2 normal() const;
	FVec2 operator*(float scalar) const;
	float& operator[](const int index);
	float operator[](const int index) const;
};
FVec2 toFloatVector(const IVec2& vec);
void printPODStruct(const FVec2& vec);
bool matchesWithinTolerance(const FVec2& v1, const FVec2& v2);

//--------------------------------------
// IVec2
//--------------------------------------
struct IVec2{
	int x, y;

	IVec2 operator+(const IVec2& other) const;
	IVec2 operator-(const IVec2& other) const;
	IVec2 operator+=(const IVec2& other);
	IVec2 operator-=(const IVec2& other);
	IVec2 operator-() const;
	IVec2 operator*(int scalar) const;
	IVec2 operator*(float scalar) const;
	bool operator==(const IVec2& other) const;
	int& operator[](const int index);
	int operator[](const int index) const;
};
void printPODStruct(const IVec2& vec);

//--------------------------------------
// FVec3
//--------------------------------------
struct FVec3{
	/*
	// NOTE: I'm going to switch to this structure once I can figure out how
	// to keep the resulting object as
		1) POD type
		2) Initializable using FVec3 vec = {x, y, z}; notation
		3) Accessed using indexing and dot notation
	union{
		struct{
			float x, y, z;
		}; 
		struct{
			float r, g, b;
		};
		float components[3];
	};
	*/
	float x, y, z;

	bool isZero() const;
	float length() const;
	float dot(const FVec3& other) const;
	FVec3 cross(const FVec3& other) const;
	FVec3 normal() const;
	FVec3 rotation(const FVec3 axis, float degrees) const;
	FVec3 reflection(const FVec3 reflector) const;

	FVec3 operator+(const FVec3& other) const;
	FVec3 operator-(const FVec3& other) const;
	FVec3 operator+=(const FVec3& other);
	FVec3 operator-=(const FVec3& other);
	FVec3 operator-() const;
	FVec3 operator*(float scalar) const;
	FVec3 operator/(float scalar) const;
	float& operator[](const int index);
	float operator[](const int index) const;
};
FVec3 operator*(float scalar, const FVec3 vec);
FVec3 operator/(float scalar, const FVec3 vec);
FVec3 projection(FVec3 source, FVec3 target);
FVec3 toFloatVector(IVec3 vec);
FVec3 floatVector(int x, int y, int z);
FVec3 hadamard(FVec3 a, FVec3 b);
void printPODStruct(const FVec3& vec);
bool matchesWithinTolerance(const FVec3& v1, const FVec3& v2);

//--------------------------------------
// IVec3
//--------------------------------------
struct IVec3{
	int x, y, z;

	IVec3 operator+(const IVec3& other) const;
	IVec3 operator-(const IVec3& other) const;
	IVec3 operator+=(const IVec3& other);
	IVec3 operator-=(const IVec3& other);
	IVec3 operator-() const;
	IVec3 operator*(int scalar) const;
	bool operator==(const IVec3& other) const;
	bool operator!=(const IVec3& other) const;
	int& operator[](const int index);
	int operator[](const int index) const;
};
void printPODStruct(const IVec3& vec);

//--------------------------------------------------------------------------------------------------
// Misc Math Structures
//--------------------------------------------------------------------------------------------------
//--------------------------------------
// FVec4
//--------------------------------------
struct FVec4{
	float x, y, z, w;

	float& operator[](const int index);
	float operator[](const int index) const;
};
void printPODStruct(const FVec4& vec);

//--------------------------------------
// FMat4
// This matrix used column-major addressing
//--------------------------------------
struct FMat4{
	FVec4 r0, r1, r2, r3;

	float determinant() const;
	FMat4 transposition() const;
	FMat4 inverse() const;
	FMat4 operator*(const FMat4 other) const;
	FMat4 operator*(float scalar) const;
	FVec4 operator*(const FVec4 vector) const;
	FVec4& operator[](const int index);
	FVec4 operator[](const int index) const;
};
bool matchesWithinTolerance(FMat4 a, FMat4 b, float tolerance);
void printPODStruct(const FMat4& mat);

//--------------------------------------
// Quaternion
// Reference: 
// 	https://www.cprogramming.com/tutorial/3d/quaternions.html
//	https://www.cs.utexas.edu/~fussell/courses/cs384g-spring2016/lectures/
//		7a-Quaternions_Orientation.pdf
//--------------------------------------
struct Quat{
	float w, i, j, k;

	float magnitude() const;
	Quat normal() const;
	Quat operator*(Quat other) const;
	Quat operator+(Quat other) const;
	FMat4 asRotationMatrix() const;
};
Quat initDefaultQuaternion();
Quat quatFromAxisAngle(FVec3 axis, float angle);


//--------------------------------------------------------------------------------------------------
// Common-use Types
//--------------------------------------------------------------------------------------------------
//--------------------------------------
// String
//--------------------------------------
struct PODString{
	static constexpr int MAX_LEN = 64 - sizeof(int);
	
	char text[MAX_LEN];
	
	// NOTE: This is a janky way of ensuring the string is always
	// null terminated. If this hits zero we're out of characters.
	int num_chars_remaining;

	// Functions
	static PODString init(const char* text);
	int len() const;
	bool operator==(const PODString& other) const;
	bool operator==(const char* other) const;
};

void printPODStruct(const PODString& str);

//--------------------------------------
// Variant
//--------------------------------------
struct PODVariant{
	enum DataType : char{
		DATATYPE_INVALID = 0,

		DATATYPE_BOOL   = 1,
		DATATYPE_INT32  = 2,
		DATATYPE_FLOAT  = 3,
		DATATYPE_STRING = 4,
		DATATYPE_IVEC2  = 5,
		DATATYPE_FVEC2  = 6,
		DATATYPE_IVEC3  = 7,
		DATATYPE_FVEC3  = 8,	
	};

	static constexpr int SIZE_BY_DATATYPE[] = {
		0,

		sizeof(bool),
		sizeof(Int32),
		sizeof(float),
		sizeof(PODString),
		sizeof(IVec2),
		sizeof(FVec2),
		sizeof(IVec3),
		sizeof(FVec3),
	};

	// Member variables
	DataType type;
	union{
		bool      val_bool;
		Int32     val_int;
		float     val_float;
		PODString val_string;
		IVec2     val_ivec2;
		FVec2     val_fvec2;
		IVec3     val_ivec3;
		FVec3     val_fvec3;
	};

	static PODVariant init(DataType type, void* val);
	static PODVariant init();
	static PODVariant init(bool      val);
	static PODVariant init(Int32     val);
	static PODVariant init(float     val);
	static PODVariant init(PODString val);
	static PODVariant init(IVec2 vec);
	static PODVariant init(FVec2 vec);
	static PODVariant init(IVec3 vec);
	static PODVariant init(FVec3 vec);

	bool operator==(const PODVariant& other);
	void operator=(bool        val);
	void operator=(Int32       val);
	void operator=(float       val);
	void operator=(PODString   val);
	void operator=(const char* val);
	void operator=(IVec2 vec);
	void operator=(FVec2 vec);
	void operator=(IVec3 vec);
	void operator=(FVec3 vec);
};

void printPODStruct(const PODVariant& var);

//--------------------------------------
// Dict
// 
// This is a hacky placeholder that doesn't even use hashing. At the moment
// I'm just trying to get the interface in place. Currently just linear scan.
// TODO: Implement a proper dict.
//--------------------------------------
struct KVPair{
	PODString  key;
	PODVariant val;
};

constexpr Int32 MAX_POD_DICT_KV_PAIRS = 16;
struct PODDict{
	struct Iter{
		/*
		Janky iterator to mimic std::unordered_map's interface
		Invalidated if the dict updates
		*/

		int curr_index{0};
		const PODDict* target_dict;

		void operator++(){
			++curr_index;
		}

		KVPair operator*() const{
			return target_dict->table[curr_index];
		}

		bool operator==(const Iter& other){
			return curr_index == other.curr_index && 
				target_dict == other.target_dict;
		}

		bool operator!=(const Iter& other){
			return !(*this == other);
		}

		KVPair pair() const{
			return target_dict->table[curr_index];
		}

		PODString first() const{
			return target_dict->table[curr_index].key;	
		}

		PODVariant second() const{
			return target_dict->table[curr_index].val;	
		}
	};

	// Data
	Int32 num_entries;
	KVPair table[MAX_POD_DICT_KV_PAIRS];

	// Functions
	static PODDict init();
	Int32 keyIndex(PODString str) const;
	Int32 keyIndex(const char* key_text) const;
	Int32 len() const;
	PODVariant getValue(PODString str) const;
	bool update(KVPair pair);
	bool update(const char* key_text, PODVariant value);
	bool update(const char* key_text, bool value);
	bool update(const char* key_text, float value);
	bool update(const char* key_text, Int32 value);
	bool update(const char* key_text, const char* value_text);
	bool update(const char* key_text, IVec2 value);
	bool update(const char* key_text, FVec2 value);
	bool update(const char* key_text, IVec3 value);
	bool update(const char* key_text, FVec3 value);

	Iter begin() const;
	Iter end() const;
};

void printPODStruct(const PODDict& dict);

//--------------------------------------
// CellAddress
//--------------------------------------
struct CellAddress{
	/*
	2D example of the cell addressing pattern. Assume top-down floorplan view (XY plane) with
	+Y being north and +X being east. The letter is just a variable and the number is the
	LOD level.

	+Y                +Y
	┏━━┳━━┳━━┳━━┓     ┏━━━━━┳━━━━━┓
	┃A1┃B1┃C1┃D1┃     ┃     ┃     ┃
	┣━━╋━━╋━━╋━━┫     ┃     ┃     ┃
	┃E1┃F1┃G1┃H1┃     ┃E2   ┃G2   ┃ 
	┣━━╋━━╋━━╋━━┫     ┣━━┳━━╋━━┳━━┫
	┃I1┃J1┃K1┃L1┃     ┃I1┃J1┃K1┃L1┃
	┗━━┻━━┻━━┻━━┛+X   ┗━━┻━━┻━━┻━━┛+X

	Each basic cell is addressed by its chunk position. Each composite cell (lower LOD) is
	addressed using the chunk in the southwest corner. In the 3D case this would be the 
	(west, south, lower) corner, but I'm not drawing a labelled text cube lol. The number to 
	the side is just the lod_power.
	*/

	IVec3 corner_addr;  // Address of the chunk in the lower left rear corner of the cell.
	Int32 lod_power;  // Power of this LOD cell. A power of 3 means an 8x8x8 cell.

	bool operator==(const CellAddress& other) const;
};
void printPODStruct(const CellAddress& addr);

//--------------------------------------
// Resource Primitives
//--------------------------------------
enum ResourceType{
	RESOURCE_INVALID = 0,

	RESOURCE_FONT,
	RESOURCE_TEXTURE,
	RESOURCE_RIGID_TRIANGLE_MESH,
	RESOURCE_RIGGED_SKELETAL_MESH,
};

constexpr const char* RESOURCE_STRINGS[] = {
	"RESOURCE_INVALID",
	"RESOURCE_FONT",
	"RESOURCE_TEXTURE",
	"RESOURCE_RIGID_TRIANGLE_MESH",
	"RESOURCE_RIGGED_SKELETAL_MESH",
};

typedef Uint32 ResourceId;
constexpr ResourceId INVALID_RESOURCE_ID = 0;

struct ResourceHandle{
	ResourceType type;
	ResourceId id;

	bool operator==(const ResourceHandle& other) const;
};
void printPODStruct(const ResourceHandle& handle);

//--------------------------------------
// Entity Primitives
//--------------------------------------
typedef Uint32 EntityTypeIndex;
typedef Uint32 EntityId;
constexpr EntityTypeIndex INVALID_ENTITY_TYPE_INDEX = 0;
constexpr EntityId INVALID_ENTITY_ID = 0;
struct EntityHandle{
	/*
	// NOTE: IDs are by type. Two entities with the same
	*/

	EntityTypeIndex type_index;
	EntityId id;

	bool operator==(const EntityHandle& other) const;
};
static_assert(sizeof(EntityHandle) == 8);
void printPODStruct(const EntityHandle& handle);

//--------------------------------------
// PODHasher
//--------------------------------------
struct PODHasher{
	Bytes8 operator()(const IVec3& pos) const;
	Bytes8 operator()(PODString str) const;
	Bytes8 operator()(const CellAddress& addr) const;
	Bytes8 operator()(const ResourceHandle& handle) const;
	Bytes8 operator()(const EntityHandle& handle) const;
};


//--------------------------------------------------------------------------------------------------
// Other structs
//--------------------------------------------------------------------------------------------------
struct Range32{
	Int32 origin;
	Int32 extent;
};

struct Range64{
	Int64 origin;
	Int64 extent;
};

//--------------------------------------------------------------------------------------------------
// Hash namespace
//--------------------------------------------------------------------------------------------------
namespace Hash{
	typedef Bytes8 CoordHash;

	constexpr int LARGE_PRIME_1 = 198491317;
	constexpr int LARGE_PRIME_2 = 6542989;
	
	constexpr Bytes4 BIT_NOISE_s32_1 = 0xB5297A4D;
	constexpr Bytes4 BIT_NOISE_s32_2 = 0x68E31DA4;
	constexpr Bytes4 BIT_NOISE_s32_3 = 0x1B56C4E9;
	constexpr Bytes8 BIT_NOISE_s64_1 = 0xB5297A4DB5297A4D;
	constexpr Bytes8 BIT_NOISE_s64_2 = 0x68E31DA468E31DA4;
	constexpr Bytes8 BIT_NOISE_s64_3 = 0x1B56C4E91B56C4E9;

	CoordHash modifiedSquirrelNoise(IVec3 coord, Bytes8 seed);
};


//--------------------------------------------------------------------------------------------------
// Assertions
//--------------------------------------------------------------------------------------------------
constexpr inline void asserts(){
	// These need to be trivially-copyable POD types
	ListAssert<AssertPrimitive,
		FVec2, IVec2, FVec3, IVec3, FVec3, FMat4, 
		Quat, PODString, PODVariant, PODDict> a;

	// These have the additional restriction of needing unique representattions.
	ListAssert<AssertUniquePrimitive,
		IVec2, IVec3, CellAddress, Range32, Range64> b;
}







