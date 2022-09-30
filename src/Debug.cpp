#include "Debug.hpp"


Widget Debug::widgetFromCuboid(ICuboid cuboid, FVec3 color){
	Widget widget = {
		.type=WIDGET_CUBOID,
		.cuboid={
			.origin=toFloatVector(cuboid.origin), 
			.extent=toFloatVector(cuboid.extent),
			.color=color
		}
	};
	return widget;
}

Widget Debug::widgetFromPoint(FVec3 point, FVec3 color){
	Widget widget = {
		.type=WIDGET_MARKER,
		.marker={point, {0,0}, color}
	};
	return widget;
}

Widget Debug::widgetFromVoxel(IVec3 coord, FVec3 color){
	Widget widget = {
		.type=WIDGET_CUBOID,
		.cuboid={
			.origin=toFloatVector(coord), 
			.extent={1, 1, 1},
			.color=color
		}
	};
	return widget;
}


Widget Debug::widgetLineBetweenVoxels(IVec3 start, IVec3 end, FVec3 color){
	FVec3 centering = {0.5, 0.5, 0.5};
	Widget widget = {
		.type=WIDGET_LINE,
		.line={
			.v1={toFloatVector(start) + centering, {0,0}, color}, 
			.v2={toFloatVector(end) + centering  , {0,0}, color},
		}
	};
	return widget;
}

Widget Debug::widgetFromLine(FVec3 start, FVec3 end, FVec3 color){
	Widget widget = {
		.type=WIDGET_LINE,
		.line={
			.v1={start, {0,0}, color}, 
			.v2={end, {0,0}, color},
		}
	};
	return widget;
}

Widget Debug::widgetFromRay(Ray ray, FVec3 color, float t_value){
	const FVec2 FAKE_UV = {0, 0};
	Widget new_ray_widget = {
		.type=WIDGET_LINE,
		.line={
			.v1={ray.origin, FAKE_UV, color}, 
			.v2={ray.origin + ray.dir * t_value, FAKE_UV, color}
		}
	};

	return new_ray_widget;
}


Debug::DebugData Debug::visualizeTree(const VoxelKDTree::TreeData* tree){
	/*
	Return a debug object with widgets for various tree properties.

	TODO: Implement plane visualization
	*/

	Debug::DebugData debug;

	std::vector<Widget> leaf_widgets;  // Leaf volumes
	std::vector<Widget> plane_widgets;  // Plane volumes
	
	IVec3 local_to_world_offset = tree->bounds.origin;
	ICuboid local_bounds = {{0,0,0}, tree->bounds.extent};

	struct TraversalData{
		VoxelKDTree::NodeIndex node_index;
		int node_depth;
		ICuboid node_bounds;
	};

	std::stack<TraversalData> stack;
	stack.push({0, 0, local_bounds});
	while(stack.size() > 0){
		auto& [node_index, node_depth, node_bounds] = stack.top();
		stack.pop();
		
		VoxelKDTree::PackedData data = tree->geometry_nodes_ptr[node_index].pack;
		Bytes2 node_type = data & VoxelKDTree::MASK_NODE_TYPE_BITS;
		if(node_type == VoxelKDTree::VALUE_LEAF_NODE){
			FVec3 leaf_color;
			bool is_empty = data & VoxelKDTree::FLAG_LEAF_IS_EMPTY;
			bool is_mixed = data & VoxelKDTree::FLAG_LEAF_IS_MIXED_TYPE;
			if(is_empty){
				leaf_color = COLOR_GREEN_MILD;
			}else{
				if(is_mixed){
					leaf_color = COLOR_RED_MILD;
				}else{
					leaf_color = COLOR_BLUE_VIVID;
				}
			}
			node_bounds.origin += local_to_world_offset;
			leaf_widgets.push_back(widgetFromCuboid(node_bounds, leaf_color));
		}else{
			Axis plane_axis = (Axis) node_type;
			Uint16 plane_offset = (data & VoxelKDTree::MASK_PLANE_OFFSET_BITS) 
				>> VoxelKDTree::SHIFT_PLANE_OFFSET;

			CuboidSplit split = splitAlongAxis(node_bounds, plane_axis, 
				plane_offset);
			bool is_valid_split = split.is_valid[0] && split.is_valid[1];
			assert(is_valid_split);
			
			Int32 left_child_index = 2 * node_index + 1;
			if(tree->is_packed_tree){
				left_child_index = tree->descendant_nodes_ptr[node_index].left_child_index;
			}

			for(int i = 0; i < 2; ++i){
				ICuboid& next_cuboid = split.cuboid[i];
				TraversalData next_traversal_node = {
					left_child_index + i,
					node_depth + 1,
					next_cuboid,
				};
				stack.push(next_traversal_node);
			}
		}
	}

	printf("%li leaf widgets produced.\n", leaf_widgets.size());
	debug.widget_groups["TreeLeaves"] = leaf_widgets;
	return debug;
}

std::vector<Widget> Debug::portalWidgets(const Portal& portal){
	std::vector<Widget> portal_widgets;
	portal_widgets.reserve(3);

	constexpr FVec3 portal_colors[] = {
		{0.2, 0.2, 1.0}, {1.0, 0.2, 0.2}
	};
	for(int i = 0; i < 2; ++i){
		Widget sphere_widget = {
			.type=WidgetType::WIDGET_SPHERE,
			.sphere={
				.origin=portal.locations[i],
				.radius=portal.radius,
				.color=portal_colors[i]
			}
		};

		portal_widgets.push_back(sphere_widget);
	}

	FVec2 dummy_uv = {0,0};
	Widget connection = {
		.type=WidgetType::WIDGET_LINE,
		.line={
			.v1={
				portal.locations[0],
				dummy_uv,
				portal_colors[0]
			},
			.v2={
				portal.locations[1],
				dummy_uv,
				portal_colors[1]
			}
		}
	};
	portal_widgets.push_back(connection);
	return portal_widgets;
}

void Debug::print(const Settings& settings, bool should_indent){
	struct Indenter{
		int current_spacing = 0;

		void space(){
			for(int i = 0; i < current_spacing; ++i){
				printf("\t");
			}
		}
	};

	struct StackNode{
		std::string name;
		int depth;
	};

	printf("<Settings>\n");

	Indenter ind;
	std::stack<StackNode> stack;
	stack.push({"ENGINE", 0});
	while(stack.size() > 0){
		// Pop the next item
		auto [name, depth] = stack.top();
		ind.current_spacing = depth * should_indent;
		stack.pop(); 

		// Print the namespace contents
		auto namespace_data = settings.namespaceData(name);
		ind.space(); printf("---------------------------------\n");
		ind.space(); printf("Namespace: '%s'\n", name.c_str());
		for(auto [key, variant] : namespace_data.dict){
			ind.space(); printf("[%s]: ", key.c_str()); printPODStruct(variant); printf("\n");
		}

		// Traverse to child namespaces
		for(std::string str : namespace_data.contained_namespaces){
			stack.push({str, depth + 1});
		}
		ind.space(); printf("---------------------------------\n");
	}

	printf("</Settings>\n");
}
