#include "Geometry.hpp"

//------------------------------------------------------------------------------
// Geometric Structs
//------------------------------------------------------------------------------
//--------------------------------------
// Float structs
// NO CURRENT IMPLEMENTATION
//--------------------------------------
//--------------------------------------
// Integer structs
//--------------------------------------
ICuboid ICuboid::operator*(int scale) const{
	return {
		.origin=origin*scale,
		.extent=extent*scale,
	};
}

bool ICuboid::operator==(const ICuboid& other) const{
	return origin == other.origin && extent == other.extent;
}

void printPODStruct(const ICuboid& cuboid){
	printf("<Cuboid|Origin:");
	printPODStruct(cuboid.origin);
	printf("|Extent:");
	printPODStruct(cuboid.extent);
	printf(">");
}

CuboidSplit splitAlongAxis(ICuboid cuboid, Axis axis, Int32 offset){
	/*
	Given a cuboid, a splitting axis, and an offset from the origin, return
	the cuboids on the near and far of the split respectively. 
	NOTE: If the offset falls before the start position, there is no near
		cuboid, only a far. Likewise, if the offset falls after the end, there
		is no far cuboid, only a near.

	*/

	CuboidSplit output = {{false, false}, {cuboid, cuboid}};

	Int32 pos_start = cuboid.origin[axis];
	Int32 extent = cuboid.extent[axis];  assert(extent >= 0);
	Int32 pos_end = pos_start + extent;

	if(offset <= pos_start){
		output.is_valid[1] = true;
	}else if(offset >= pos_end){
		output.is_valid[0] = true;
	}else{
		// Split falls between the start and end of the cuboid

		output.is_valid[0] = true;
		output.cuboid[0].origin[axis] = pos_start;
		output.cuboid[0].extent[axis] = offset - pos_start;

		output.is_valid[1] = true;
		output.cuboid[1].origin[axis] = offset;
		output.cuboid[1].extent[axis] = pos_end - offset;	
	}

	return output;
}

Int64 surfaceArea(ICuboid cuboid){
	Int64 total = 0;
	
	total += (cuboid.origin[0] * cuboid.origin[1]);  // 1x XY face
	total += (cuboid.origin[1] * cuboid.origin[2]);  // 1x YZ face
	total += (cuboid.origin[2] * cuboid.origin[1]);  // 1x ZX face

	return total * 2;
}

Int64 volume(ICuboid cuboid){
	return cuboid.extent[0] * cuboid.extent[1] * cuboid.extent[2];
}

//------------------------------------------------------------------------------
// Other structs
//------------------------------------------------------------------------------
//--------------------------------------
// Ray
//--------------------------------------
Ray Ray::normal() const{
	Ray result = {origin, dir.normal()};
	return result;
}

void printPODStruct(const Ray& ray){
	printf("<Ray");
	printf("|Origin: "); printPODStruct(ray.origin);
	printf("|Direction: "); printPODStruct(ray.dir);
	printf(">");
}

//--------------------------------------
// Basis
//--------------------------------------
Basis Basis::orthogonalized() const{
	/*
	Returns an orthogonalized basis using a simplified version of the 
	Gram-Schmidt process.
	WARNING: Untested.
	*/

	// Handle X
	FVec3 x_norm = v0.normal();
	FVec3 x_new = x_norm;

	// Handle Y
	FVec3 y_norm = v1.normal();
	FVec3 y_new = y_norm - 
		(y_norm.dot(x_new)) * x_new;

	// Handle Z
	FVec3 z_norm = v2.normal();
	FVec3 z_new = z_norm - 
		(z_norm.dot(x_new)) * x_new - 
		(z_norm.dot(y_new)) * y_new;

	return {x_new, y_new, z_new};
}

Basis Basis::rotatedByVector(FVec3 axis, float amount) const{
	/*
	IMPLEMENT
	*/
	assert(false);
	return {
		{},
		{},
		{}
	};
}

FVec3& Basis::operator[](const int index){
	FVec3* content_arr = &this->v0;
	return content_arr[index];
}

FVec3 Basis::operator[](const int index) const{
	const FVec3* content_arr = &this->v0;
	return content_arr[index];
}

Basis rotateElementwise(Basis basis, float horizontal, float vertical){
	/*
	Rotates the basis about the Z axis first (horizontal), then X (vertical).
	NOTE: {V0, V1, V2} == {X, Y, Z}
	*/

	Basis output_basis = basis;

	// Rotate Horizontal
	if(horizontal != 0){
		output_basis.v0 = output_basis.v0.rotation(output_basis.v2, horizontal);
		output_basis.v1 = output_basis.v1.rotation(output_basis.v2, horizontal);
	}

	// Rotate Vertical
	if(vertical != 0){
		output_basis.v2 = output_basis.v2.rotation(output_basis.v0, vertical);
		output_basis.v1 = output_basis.v1.rotation(output_basis.v0, vertical);
	}

	return output_basis;
}


void printPODStruct(const Basis& basis){
	printf("<Basis>\n");
	for(int i = 0; i < 3; ++i){
		printf("\t");
		printPODStruct(basis[i]);
		printf("\n");
	}
	printf("</Basis>");
}


//--------------------------------------------------------------------------------------------------
// Misc functions
//--------------------------------------------------------------------------------------------------
std::vector<FVec3> Geometry::fibonacciSphere(int num_points){
	/*
	ATTRIBUTION: Math for calculating points on a Fibonacci Sphere
		Sebastian League
	
	Returns an array of unit vectors pointing to the surface of a sphere.
	*/

	constexpr float ANGLE_INCREMENT = TAU * GOLDEN_RATIO;

	std::vector<FVec3> output_dirs;
	output_dirs.reserve(num_points);
	for(int i = 0; i < num_points; ++i){
		float t = static_cast<float>(i) / num_points;
		float angle1 = acos(1 - 2 * t);
		float angle2 = ANGLE_INCREMENT * i;

		float x = sin(angle1) * cos(angle2);
		float y = sin(angle1) * sin(angle2);
		float z = cos(angle1);

		FVec3 displacement = {x, y, z};
		output_dirs.push_back(displacement.normal());
	}

	return output_dirs;
}