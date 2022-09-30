#pragma once

#include "Geometry.hpp"
#include "Linefont.hpp"

#include <unordered_map>

//-----------------------------------------------
// Definitions
//-----------------------------------------------
enum WidgetType{
	WIDGET_INVALID = 0,

	WIDGET_MARKER,
	WIDGET_LINE,
	WIDGET_CUBOID,
	WIDGET_SPHERE,
	WIDGET_PLANE,
	WIDGET_TEXT,
};

struct WidgetVertex{
	FVec3 pos;
	FVec2 uv;
	FVec3 col;
};


//-----------------------------------------------
// Widget definitions
//-----------------------------------------------
struct WidgetMarker{
	/*
	A marker at a 3d point in space
	*/
	WidgetVertex vertex;
};

struct WidgetLine{
	/*
	Line between two points. Can have different colored
	endpoints. 
	*/
	WidgetVertex v1;
	WidgetVertex v2;
};

struct WidgetCuboid{
	/*
	Creates a cuboid with the lowest XYZ coordinates as the
	origin and side lengths of "extent".
	*/

	FVec3 origin, extent;
	FVec3 color;
};

struct WidgetSphere{
	FVec3 origin;
	float radius;
	FVec3 color;
};

struct WidgetText{
	FVec3 origin;
	PODString string;
};

struct WidgetPlane{
	FVec3 origin;
	FVec3 normal;
	FVec2 axis_bounds;  // Distance to go from the orign in U, V space
};


//-----------------------------------------------
// Main Widget Structs
//-----------------------------------------------
struct Widget{
	WidgetType type;
	union{
		WidgetMarker marker;
		WidgetLine   line;
		WidgetCuboid cuboid;
		WidgetSphere sphere;
		WidgetText   text;
		WidgetPlane  plane;
	};
};
