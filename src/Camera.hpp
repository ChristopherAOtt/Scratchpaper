#pragma once

#include "Geometry.hpp"


//--------------------------------------
// Camera
//--------------------------------------
struct Camera{
	FVec3 pos;
	Basis basis;

	float fov;  // 90
	float aspect_ratio;  // Width / Height

	float near_plane;
	float far_plane;
};

Camera initDefaultCamera();
FMat4 perspectiveMatrix(Camera camera, IVec2 screen_dims);


//--------------------------------------------------------------------------------------------------
// Assertions
// TODO: Fix the awful formatting here. Figure out proper template usage.
//--------------------------------------------------------------------------------------------------
static_assert(std::is_pod<Camera>::value,        "Camera must be POD");
static_assert(std::is_trivially_copyable<Camera>::value,
	"Camera must be trivially copyable");
