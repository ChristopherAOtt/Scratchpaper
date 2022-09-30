#pragma once

#include "Primitives.hpp"
#include "Constants.hpp"
#include "Widgets.hpp"
#include "KDTree.hpp"
#include "Colors.hpp"
#include "Geometry.hpp"
#include "WorldState.hpp"
#include "Settings.hpp"

#include <vector>
#include <string>
#include <unordered_map>
#include <stack>

namespace Debug{
	struct DebugData{
		/*
		A struct passed to functions to collect data for
		debugging purposes
		*/

		std::string chronological_messages;
		std::unordered_map<std::string, std::vector<Widget>> widget_groups;
	};

	Widget widgetFromCuboid(ICuboid cuboid, FVec3 color);
	Widget widgetFromPoint(FVec3 point, FVec3 color);
	Widget widgetFromVoxel(IVec3 coord, FVec3 color);
	Widget widgetLineBetweenVoxels(IVec3 start, IVec3 end, FVec3 color);
	Widget widgetFromLine(FVec3 start, FVec3 end, FVec3 color);
	Widget widgetFromRay(Ray ray, FVec3 color, float t_value=10);

	DebugData visualizeTree(const VoxelKDTree::TreeData* tree);	

	std::vector<Widget> portalWidgets(const Portal& portal);

	void print(const Settings& settings, bool should_indent=false);
}

