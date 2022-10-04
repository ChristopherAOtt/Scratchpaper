#pragma once

#include "ListAsserts.hpp"
#include "Primitives.hpp"

#include <vector>

//--------------------------------------------------------------------------------------------------
// Geometric Structs
//--------------------------------------------------------------------------------------------------
//--------------------------------------
// Float structs
//--------------------------------------
struct FSphere{
	FVec3 origin;
	float radius;
};

struct FCuboid{
	FVec3 origin;
	FVec3 extent;
};

struct FCylinder{
	FVec3 origin;
	FVec3 dir;  // Length is the height
	float radius;
};

struct FCone{
	FVec3 origin;
	FVec3 dir;  // Length is the height
	float apex_angle;  // Angle between the sides and height, from the apex.
};

struct FRect{
	FVec3 origin;
	FVec3 dir;
	float width;
	float height;
};

struct FLine{
	FVec3 origin;
	FVec3 destination;
};

struct FPlane{
	FVec4 parameters;
};

//--------------------------------------
// Integer structs (grid-aligned)
//--------------------------------------
struct ICuboid{
	IVec3 origin;
	IVec3 extent;

	ICuboid operator*(int scale) const;
	bool operator==(const ICuboid& other) const;
};
void printPODStruct(const ICuboid& cuboid);

struct CuboidSplit{
	bool is_valid[2];
	ICuboid cuboid[2];
};

CuboidSplit splitAlongAxis(ICuboid cuboid, Axis axis, Int32 offset);
Int64 surfaceArea(ICuboid cuboid);
Int64 volume(ICuboid cuboid);

//--------------------------------------
// Misc other structs
//--------------------------------------
struct Triangle{
	FVec3 vertices[3];
};

//--------------------------------------
// Generalized shape
//--------------------------------------
enum GeometryType{
	GEOMETRY_INVALID,

	GEOMETRY_SPHERE,
	GEOMETRY_CUBOID,
	GEOMETRY_CYLINDER,
	GEOMETRY_CONE,
	GEOMETRY_RECT,
	GEOMETRY_LINE,
	GEOMETRY_PLANE
};

struct GeometricPrimitive{
	GeometryType type;
	
	union{
		FSphere sphere;
		FCuboid cuboid;
		ICuboid cuboid_int;
		FCylinder cylinder;
		FCone cone;
		FRect rect;
		FLine line;
		FPlane plane;
	};
};

//--------------------------------------
// Basis
//--------------------------------------
struct Basis{
	FVec3 v0, v1, v2;

	Basis orthogonalized() const;
	Basis rotatedByVector(FVec3 axis, float degrees) const;
	FVec3& operator[](const int index);
	FVec3 operator[](const int index) const;
};

Basis rotateElementwise(Basis basis, float horizontal, float vertical);
void printPODStruct(const Basis& basis);

//--------------------------------------
// Ray
//--------------------------------------
struct Ray{
	public:
		FVec3 origin;
		FVec3 dir;

	public:
		Ray normal() const;
};
void printPODStruct(const Ray& ray);

//--------------------------------------------------------------------------------------------------
// Janky constants definitions
// NOTE: Since this file is dependent on Constants.hpp, we currently need 
// to put the values using these structs here instead. 
// TODO: Figure out a better way to do this. 
//--------------------------------------------------------------------------------------------------
constexpr FVec3 WORLD_UP = {0, 0, 1};
constexpr Basis DEFAULT_BASIS = {
	{1, 0, 0},
	{0, 1, 0},
	{0, 0, 1},
};

constexpr FVec3 NORMALS_BY_FACE_INDEX[] = {
	/*
	NOTE: Ray normals face AWAY from the face. For example, the positive Y
	axis normal is {0, -1, 0}. The negative Y is {0, 1, 0} These are indexed using
	the equation "normal_index = axis * 2 + is_negative;"
	*/

	{-1, 0, 0}, {1, 0, 0},  // +X/-X
	{0, -1, 0}, {0, 1, 0},  // +Y/-Y
	{0, 0, -1}, {0, 0, 1},  // +Z/-Z
};


//--------------------------------------------------------------------------------------------------
// Misc functions. Put in a namespace for organizational reasons.
//--------------------------------------------------------------------------------------------------
namespace Geometry{
	std::vector<FVec3> fibonacciSphere(int num_points);	
};


//--------------------------------------------------------------------------------------------------
// Assertions
//--------------------------------------------------------------------------------------------------
constexpr inline void geometryAsserts(){
	// These need to be trivially-copyable POD types
	ListAssert<AssertPrimitive,
		FSphere, FCuboid, FCylinder, FCone, FRect, FLine, FPlane, 
		GeometricPrimitive, Ray, Basis> a;

	// These have the additional restriction of needing unique representattions.
	ListAssert<AssertUniquePrimitive, ICuboid> b;
}
