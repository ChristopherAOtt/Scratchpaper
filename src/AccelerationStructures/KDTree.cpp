#include "KDTree.hpp"

//-----------------------------------------------------------------------------
// VoxelKDTree
//-----------------------------------------------------------------------------
void VoxelKDTree::debuggingPrintTreeContents(const VoxelKDTree::TreeData* tree){
	assert(tree->is_packed_tree);

	const char* TYPE_TEXT[] = {
		"AXIS_X", "AXIS_Y", "AXIS_Z", "LEAF        "
	};

	printf("<Tree Contents>\n");
	for(int i = 0; i < tree->node_count; ++i){
		VoxelKDTree::PackedData pack = tree->geometry_nodes_ptr[i].pack;
		int node_type = pack & MASK_NODE_TYPE_BITS;
		
		printf("\t[%06d]: ", i);
		debuggingPrintPackedData(pack);
		printf("|%s", TYPE_TEXT[node_type]);
		if(node_type != VALUE_LEAF_NODE){
			int offset = (pack & MASK_PLANE_OFFSET_BITS) >> SHIFT_PLANE_OFFSET;
			printf(" %05d", offset);
		}
		printf("|%i\n", tree->descendant_nodes_ptr[i].left_child_index);
	}
	printf("</Tree Contents>\n");
}


void VoxelKDTree::debuggingPrintTreeStats(const VoxelKDTree::TreeData* tree){
	const char* bool_to_str[2] = {"False", "True"};
	constexpr int NUM_TREE_NODE_TYPES = 3;

	printf("<VoxelKDTree>\n");
	printf("\tTreeData struct size: %li\n", sizeof(TreeData));
	printf("\tBounds:"); printPODStruct(tree->bounds); printf("\n");
	printf("\tNode Count/Capacity (%li/%li) %f%%\n",
		tree->node_count,
		tree->node_capacity,
		((float)tree->node_count / (float)tree->node_capacity) * 100);
	printf("\tisPacked: %s\n", bool_to_str[tree->is_packed_tree]);
	printf("\thasPropertyNodes: %s\n", bool_to_str[tree->has_property_nodes]);
	
	struct NodeInfo{
		bool has_node_type;
		int size_per_node;
		const char* type_name;
	};

	Int64 memory_footprint = sizeof(TreeData);
	Int64 node_array_footprints[NUM_TREE_NODE_TYPES] = {};
	NodeInfo type_info[NUM_TREE_NODE_TYPES] = {
		{true,                     sizeof(GeometryNode),   "GeometryNode"},
		{tree->is_packed_tree,     sizeof(DescendantNode), "DescendantNode"},
		{tree->has_property_nodes, sizeof(PropertyNode),   "PropertyNode"},
	};
	for(int i = 0; i < NUM_TREE_NODE_TYPES; ++i){
		auto [has_item, item_size, item_name] = type_info[i];
		Int64 array_size = has_item * tree->node_capacity * item_size;
		Int64 num_bytes_utilized = has_item * tree->node_count * item_size;
		memory_footprint += array_size;
		node_array_footprints[i] += array_size;
		printf("\tBytes for '%s' array: %li\n", item_name, array_size);
		printf("\t\tNum utilized bytes: %li\n", num_bytes_utilized);
	}
	printf("\tTotal memory footprint: %li\n", memory_footprint);

	printf("</VoxelKDTree>\n");
}

void VoxelKDTree::debuggingPrintPackedData(VoxelKDTree::PackedData data){
	char as_text[VoxelKDTree::NUM_PACKED_DATA_BITS + 7] = {};
	char bool_to_digit[2] = {'0', '1'};
	int write_index = 0;
	constexpr Bytes2 HIGH_MASK = 0b1000'0000'0000'0000;

	for(int i = 0; i < VoxelKDTree::NUM_PACKED_DATA_BITS; ++i){
		if(i % 4 == 0){
			as_text[write_index] = '\'';
			++write_index;
		}

		int digit_index = ((data << i) & HIGH_MASK) >> 
			(VoxelKDTree::NUM_PACKED_DATA_BITS - 1);
		as_text[write_index] = bool_to_digit[digit_index];
		++write_index;
	}
	printf("<Pack %s'>", &as_text[0]);
}

Int64 VoxelKDTree::writeToBuffer(const TreeData* tree){
	/*
	
	*/
}

//-----------------------------------------------------------------------------
// Tree Builder Namespace
//-----------------------------------------------------------------------------
//-----------------------------------------------
// Voxel Lookup
//-----------------------------------------------
VoxelKDTree::TreeBuilder::VoxelLookup::VoxelLookup(const ChunkTable* table){
	m_table_ptr = table;
	m_num_cached_chunks = 0;
}

bool isInBounds(IVec3 vec){
	for(Int32 i = 0; i < 3; ++i){
		if(vec[i] < 0 || vec[i] >= CHUNK_LEN){
			return false;
		}
	}
	return true;
}

Voxel VoxelKDTree::TreeBuilder::VoxelLookup::voxelAtCoord(
	IVec3 global_voxel_coord){
	/*
	Returns the voxel at the given global coordinate
	THREADING: Unsafe reference to the chunk table
	*/

	// Update cached chunk if we didn't have it yet.
	IVec3 chunk_coord = chunkCoordFromVoxelCoord(global_voxel_coord);
	IVec3 local_voxel_coord = localVoxelCoordFromGlobal(global_voxel_coord);
	assert(isInBounds(local_voxel_coord));

	// See if we can find a cached chunk
	Int32 read_index = -1;
	for(Int32 i = 0; i < m_num_cached_chunks; ++i){
		if(chunk_coord == m_cached_chunk_coords[i]){
			read_index = i;
			break;
		}
	}

	// If the chunk wasn't cached, find it. If we still have empty slots, fill
	// them. If not, kick the one with the least recent use.
	if(read_index == -1){
		Int32 index_to_replace = m_num_cached_chunks;
		if(m_num_cached_chunks == CACHE_CAPACTIY){
			Uint8 most_misses = m_num_misses_since_last_refresh[0];
			index_to_replace = 0;
			for(Int32 i = 1; i < CACHE_CAPACTIY; ++i){
				if(m_num_misses_since_last_refresh[i] > most_misses){
					most_misses = m_num_misses_since_last_refresh[i];
					index_to_replace = i;
				}
				++m_num_misses_since_last_refresh[i];
			}
		}else{
			++m_num_cached_chunks;
		}

		// Swap the first chunk with the deleted one so the new chunk is
		// first in the list
		m_cached_chunk_coords[index_to_replace] = m_cached_chunk_coords[0];
		m_cached_chunks[index_to_replace] = m_cached_chunks[0];
		m_num_misses_since_last_refresh[index_to_replace] = 
			m_num_misses_since_last_refresh[0];

		// Write the new chunk.
		m_cached_chunk_coords[0] = chunk_coord;
		m_num_misses_since_last_refresh[0] = 0;
		m_cached_chunks[0] = *m_table_ptr->getChunkPtr(chunk_coord);
		read_index = 0;
	}
	
	RawVoxelChunk& target_chunk = m_cached_chunks[read_index];
	return target_chunk.data[linearChunkIndex(local_voxel_coord)];
}

//-----------------------------------------------
// Tree building code
//-----------------------------------------------
VoxelKDTree::TreeBuilder::AxisSummary 
VoxelKDTree::TreeBuilder::generateSummary(const ChunkTable* table, 
	IVec3 volume_origin_offset, ICuboid volume, Axis axis){
	/*
	Given a volume in tree space and an axis to move along, return 
	an axis summary.

	NOTE: This is one of the few areas in the code where the coordinates
	can vary between tree space and world space. Be careful.
	*/

	constexpr IVec2 ITERATION_PLANES[] = {
		{AXIS_Y, AXIS_Z},
		{AXIS_X, AXIS_Z},
		{AXIS_X, AXIS_Y},
	};
	const IVec2 offsets = ITERATION_PLANES[axis];
	
	VoxelLookup lookup(table);

	// Precalculate for easier reading
	int origin_u    = volume.origin[offsets[0]];
	int origin_v    = volume.origin[offsets[1]];
	int origin_axis = volume.origin[axis];
	int final_u     = origin_u + volume.extent[offsets[0]];
	int final_v     = origin_v + volume.extent[offsets[1]];
	int axis_extent = volume.extent[axis];
	IVec3 tree_coord;  // Coordinate in "tree space" relative to origin
	IVec3 world_coord;  // Coordinate in world space, used for queries.
	IVec3 local_to_world_offset = volume_origin_offset;

	Int64 total_solids = 0;
	Int64 plane_area = volume.extent[offsets[0]] * volume.extent[offsets[1]];
	//Int32 plane_perimeter = 2*(final_u - origin_u) + 2*(final_v - origin_v);
	Int64 plane_perimeter = 
		2 * (volume.extent[offsets[0]]) + 2 * (volume.extent[offsets[1]]);
	std::vector<Int64> num_solid_in_plane;
	std::vector<VoxelType> type_if_homogenous;
	num_solid_in_plane.reserve(axis_extent);
	type_if_homogenous.reserve(axis_extent);

	for(int a = 0; a < axis_extent; ++a){
		Int64 num_plane_solids = 0;
		VoxelType first_voxel_type;
		bool is_currently_homogenous = true;
		for(int v = origin_v; v < final_v; ++v){
			for(int u = origin_u; u < final_u; ++u){
				// Generate an actual voxel coordinate
				tree_coord[axis] = origin_axis + a;
				tree_coord[offsets[0]] = u;
				tree_coord[offsets[1]] = v;
				world_coord = tree_coord + local_to_world_offset;

				VoxelType curr_type = lookup.voxelAtCoord(world_coord).type;
				bool is_solid = !isAir(curr_type);
				num_plane_solids += is_solid;

				// Type homogeneity info
				if(v == origin_v && u == origin_u){
					// This is the first voxel
					first_voxel_type = curr_type;
				}
				is_currently_homogenous &= (curr_type == first_voxel_type);
			}
		}

		total_solids += num_plane_solids;
		num_solid_in_plane.push_back(num_plane_solids);

		// Push back the type if they were all the same, and
		// EMPTY if they weren't.
		VoxelType plane_type = VoxelType::EMPTY;
		if(is_currently_homogenous){
			plane_type = first_voxel_type;
		}
		type_if_homogenous.push_back(plane_type);
	}
	assert(type_if_homogenous.size() == num_solid_in_plane.size());

	Int16 offset_from_orign = volume.origin[axis];
	AxisSummary output_summary = {
		.axis=axis,
		.offset_from_origin=offset_from_orign, 
		.plane_perimeter=plane_perimeter,
		.plane_area=plane_area,
		.total_solids=total_solids, 
		.num_solid_in_plane=num_solid_in_plane, 
		.type_if_homogenous=type_if_homogenous
	};
	return output_summary;
}

void 
VoxelKDTree::TreeBuilder::debuggingPrintAxisSummary(
	const AxisSummary& summary){
	/*

	*/
	char axis_strings[] = {'X', 'Y', 'Z'};
	Int64 num_planes = summary.num_solid_in_plane.size();
	
	printf("<AxisSummary>\n");
	printf("\tAxis: %c\n", axis_strings[summary.axis]);
	printf("\tOffsetFromOrigin: %i\n", summary.offset_from_origin);
	printf("\tPlaneArea: %li\n", summary.plane_area);
	printf("\tTotalSolids: %li\n", summary.total_solids);
	printf("\tNumPlanes: %li\n", num_planes);
	printf("\tNumSolidInPlane: {");
	for(Int64 value : summary.num_solid_in_plane){
		printf("%li, ", value);
	}
	printf("}\n");
	printf("\tTypeIfHomogenous: {");
	for(VoxelType type : summary.type_if_homogenous){
		printf("%i, ", (int) type);
	}
	printf("}\n");
	printf("</AxisSummary>\n");
}

Axis 
VoxelKDTree::TreeBuilder::axisPickerStupid(const SplitReferenceData& ref){
	/*
	Pick which axis to split along. Just cycle around based on the depth.
	*/

	Axis axis = (Axis)((ref.node_depth + 3) % NUM_3D_AXES);
	assert(axis >= 0 && axis < NUM_3D_AXES);
	return axis;
}

Axis 
VoxelKDTree::TreeBuilder::axisPickerLongest(const SplitReferenceData& ref){
	/*
	Pick the longest axis to split. If axes are equal in length, side
	with the lowest valued one (X < Y < Z).
	*/

	const IVec3& extent = ref.node_cuboid.extent;
	int largest_axis_index = 0;
	for(int i = 1; i < 3; ++i){
		if(extent[i] > extent[largest_axis_index]){
			largest_axis_index = i;
		}	
	}

	Axis axis = (Axis) largest_axis_index;
	assert(axis >= 0 && axis < NUM_3D_AXES);
	return axis;
}

VoxelKDTree::TreeBuilder::SplitResult 
VoxelKDTree::TreeBuilder::generateSplit(const SplitReferenceData& ref){
	/*
	Given reference data to a tree under construction (including a target
	node), find the optimal split axis and return the result.

	CONVENTION: All info is in tree space unless stated otherwise.
		Tree Space: Coordinate relative to origin of tree.
		Node Space: Coordinate relative to origin of node.
		World Space: Coordinate relative to origin of world.

	TODO: Add ability to check all axes and pick best option
	TODO: Take types into consideration when splitting. 
		For now they're all treated the same.
	*/

	const TreeData* tree_ptr = ref.tree_data;
	//const GeometryNode& node = tree_ptr->geometry_nodes_ptr[ref.node_to_split];
	auto& mandatory_leaf_volume = ref.settings->mandatory_leaf_volume;
	AxisSummary all_summaries[3] = {};
	SplitRecommendation all_recs[3] = {};

	// STEP 1: Survey the axis and decide where to split
	Axis axis = axisPickerLongest(ref);
	all_summaries[axis] = generateSummary(
		ref.chunk_table, tree_ptr->bounds.origin, ref.node_cuboid, axis);
	//all_recs[axis] = offsetPickerStupid(ref, all_summaries[axis]);
	all_recs[axis] = offsetPickerLongestRunBias(ref, all_summaries[axis]);
	Axis best_axis = axis;
	if(ref.settings->optimization_level == OPTIMIZE_EXHAUSTIVE){
		float best_split_score = all_recs[axis].split_score;
		for(int i = 0; i < NUM_3D_AXES; ++i){
			if(i != axis){
				all_summaries[i] = generateSummary(ref.chunk_table, 
					tree_ptr->bounds.origin, ref.node_cuboid, (Axis) i);
				all_recs[i] = offsetPickerLongestRunBias(ref, 
					all_summaries[i]);
			}

			if(all_recs[i].split_score > best_split_score){
				best_split_score = all_recs[i].split_score;
				best_axis = (Axis) i;
			}
		}
	}
	AxisSummary& summary = all_summaries[best_axis];
	SplitRecommendation rec = all_recs[best_axis];
	CuboidSplit split_cuboids = splitCuboid(ref.node_cuboid, rec.plane);

	// STEP 2: Get information about the two children.
	bool is_split_result_leaf[NUM_CHILDREN_PER_SPLIT];
	GeometryNode child_nodes[2] = {VALUE_UNSET_NODE, VALUE_UNSET_NODE};
	auto candidate_leaf_states = leafStates(summary, rec.plane);
	for(int i = 0; i < NUM_CHILDREN_PER_SPLIT; ++i){
		PackedData possible_leaf = candidate_leaf_states[i];
		Int16 local_offset = rec.plane.offset - summary.offset_from_origin;
		Int64 child_volume = summary.plane_area * local_offset;
		
		// Decide based on a number of factors whether to go with the leaf
		// data or leave the node unset.
		assert(!(ref.node_depth > ref.settings->max_depth));
		bool is_empty = possible_leaf & FLAG_LEAF_IS_EMPTY;
		bool is_mixed = possible_leaf & FLAG_LEAF_IS_MIXED_TYPE;
		bool should_make_leaf = false;
		should_make_leaf |= is_empty;
		should_make_leaf |= (!is_empty && !is_mixed);  // Homogenous solid
		should_make_leaf |= (possible_leaf == VALUE_SOLID_MIXED_LEAF);
		should_make_leaf |= (child_volume <= mandatory_leaf_volume);
		should_make_leaf |= (ref.node_depth == ref.settings->max_depth - 1);

		// Update tracking variables of the result
		is_split_result_leaf[i] = should_make_leaf;
		if(should_make_leaf){
			child_nodes[i].pack = possible_leaf;
		}
	}

	// STEP 3: Perform all the nasty bit packing and formatting
	// before returning the result.
	SplitResult result;
	result.split = rec.plane;
	for(int i = 0; i < NUM_CHILDREN_PER_SPLIT; ++i){
		result.cuboids[i] = split_cuboids.results[i];
		result.is_leaf_node[i] = is_split_result_leaf[i];
		result.children[i] = child_nodes[i];	
	}

	if(ref.settings->should_calculate_non_leaf_properties){
		assert(false);  // TODO: Implement
	}else{
		result.is_properties_defined[0] = false;
		result.is_properties_defined[1] = false;
	}

	return result;
}

VoxelKDTree::TreeBuilder::SplitRecommendation 
VoxelKDTree::TreeBuilder::axisPickerBlindCenter(
	const SplitReferenceData& ref, const AxisSummary& summary){
	/*
	Offset picker that picks based on a blind 50/50 split. 
	*/

	Int16 node_space_offset = summary.num_solid_in_plane.size() / 2;
	if(node_space_offset == 0){
		node_space_offset = 1;
	}

	Int16 tree_space_offset = summary.offset_from_origin + node_space_offset;
	assert(tree_space_offset != INVALID_PLANE_OFFSET);
	return {
		.plane={summary.axis, tree_space_offset},
		.split_score=0
	};
}

VoxelKDTree::TreeBuilder::SplitRecommendation 
VoxelKDTree::TreeBuilder::offsetPickerLongestRunBias(
	const SplitReferenceData& ref, const AxisSummary& summary){
	/*
	Offset picker that splits nodes at the edge of the longest same-density
	run along the axis. This is intended to split at the edges of homogenous
	volumes and make large leaves. If no good run could be found it splits
	down the middle to try reducing the node size as much as possible.
	
	TODO: Right now this only considers counts. Consider materials as well.
	*/

	constexpr float BAD_SPLIT_RATIO = 0.1;
	constexpr int BAD_MIN_RUN_LEN = 2;
	auto& solids_vec = summary.num_solid_in_plane;
	int num_planes = solids_vec.size();
	//assert(num_planes > 1);
	if(num_planes < 2){
		return {
			.plane={summary.axis, 0},
			.split_score=-LARGE_FLOAT
		};
	}
	
	// Find the longest continuous run
	int best_run_len = 0;
	int best_run_start_index = 0;
	int curr_run_start_index = 0;
	int curr_run_len = 0;
	int curr_run_solids_count = solids_vec[0];
	for(int i = 0; i < num_planes; ++i){
		if(solids_vec[i] == curr_run_solids_count){
			// Run continues
			++curr_run_len;
		}else{
			// Run terminated
			if(curr_run_len > best_run_len){
				best_run_start_index = curr_run_start_index;
				best_run_len = curr_run_len;
			}

			curr_run_len = 1;
			curr_run_start_index = i;
			curr_run_solids_count = solids_vec[i];
		}
	}

	// Force one last check in case the best run was at the end
	if(curr_run_len > best_run_len){
		best_run_start_index = curr_run_start_index;
		best_run_len = curr_run_len;
	}

	// If the best run started at 0, flip the split to the end of the run.
	int best_split_pos = best_run_start_index;
	if(best_run_start_index == 0){
		best_split_pos = best_run_len;
	}

	if(false){
		// This happens if we picked an axis where everything is
		// consistent all the way down. IE: a Z-oriented box around
		// a featureless pillar.
		if(best_run_len == num_planes){
			// Split in half to avoid pathalogical linear time.
			best_split_pos = num_planes / 2;
			
			// This only happens if the entire volume is a leaf. 
			// Leaves shouldn't be split.
			assert(curr_run_solids_count != summary.plane_area);
		}

		// If the split is too unequal, or there is no good run, put the split 
		// down the middle to avoid turning the tree into a linked-list.
		float split_ratio = 
			(float)std::abs(num_planes / 2 - best_split_pos) / 
			(float)num_planes;
		if(best_run_len < BAD_MIN_RUN_LEN || split_ratio < BAD_SPLIT_RATIO){
			best_split_pos = num_planes / 2;
		}
	}

	// Finalize the offset
	Int16 node_space_offset = std::min(
		std::max(best_split_pos, 1), num_planes - 1);
	Int16 tree_space_offset = summary.offset_from_origin + node_space_offset;
	assert(tree_space_offset != INVALID_PLANE_OFFSET);
	
	// Calculate the score
	float volume = (best_split_pos * summary.plane_area);
	float surface_area = best_split_pos * summary.plane_perimeter;
	float split_score = (volume * surface_area) * best_run_len;
	if(best_run_len == num_planes){
		split_score = -split_score;
	}

	return {
		.plane={summary.axis, tree_space_offset},
		.split_score=split_score
	};
}

VoxelKDTree::TreeBuilder::CuboidSplit 
VoxelKDTree::TreeBuilder::splitCuboid(ICuboid cuboid, SplitPlane split){
	/*
	Splits the cuboid along the given plane. A more restrictive version
	of the method in Geometry.hpp.
	*/

	int pos_start = cuboid.origin[split.axis];
	int extent = cuboid.extent[split.axis];
	int pos_end = pos_start + extent;
	assert(split.offset > pos_start && split.offset < pos_end);

	// Results[0].origin stays the same, but kept here for clarity
	CuboidSplit output = {{cuboid, cuboid}};
	output.results[0].origin[split.axis] = pos_start;
	output.results[0].extent[split.axis] = split.offset - pos_start;
	output.results[1].origin[split.axis] = split.offset;
	output.results[1].extent[split.axis] = pos_end - split.offset;

	return output;
}

std::array<VoxelKDTree::PackedData, 2>
VoxelKDTree::TreeBuilder::leafStates(const AxisSummary& summary, 
	SplitPlane split){
	/*
	Given an iterator over a volume, return what leaf state it
	would be if we assume that it's a leaf.

	WARNING: Currently assumes that the palette index is just the 
		numerical value of the type.
	*/

	Int16 num_planes = summary.num_solid_in_plane.size();
	Int16 local_offset = split.offset - summary.offset_from_origin;
	assert(num_planes > 0);
	assert(num_planes <= MAX_VALID_PLANE_OFFSET);
	assert(local_offset <= MAX_VALID_PLANE_OFFSET);
	assert(local_offset >= MIN_VALID_PLANE_OFFSET);

	constexpr int INDEX_COUNT_AIR = 0;
	constexpr int INDEX_COUNT_SOLID = 1;

	// Get summary for the children on each half of the axis summary.
	Uint64 counts[NUM_CHILDREN_PER_SPLIT][2] = {{0, 0}, {0, 0}};
	bool is_homogenous[NUM_CHILDREN_PER_SPLIT] = {
		summary.num_solid_in_plane[0] == summary.plane_area, 
		summary.num_solid_in_plane[local_offset] == summary.plane_area,
	};
	VoxelType types[NUM_CHILDREN_PER_SPLIT] = {
		summary.type_if_homogenous[0], 
		summary.type_if_homogenous[local_offset]
	};
	for(Int16 i = 0; i < num_planes; ++i){
		bool child_index = i >= local_offset;
		
		Int32 num_air = summary.plane_area - summary.num_solid_in_plane[i];
		counts[child_index][INDEX_COUNT_SOLID] += summary.num_solid_in_plane[i];
		counts[child_index][INDEX_COUNT_AIR] += num_air;
		
		bool types_match = types[child_index] == summary.type_if_homogenous[i];
		is_homogenous[child_index] &= types_match;
	}

	// Now generate a packed leaf node for each child.
	std::array<PackedData, NUM_CHILDREN_PER_SPLIT> partition_results;
	Int16 num_child_planes[] = {
		local_offset, 
		num_planes - local_offset
	};
	for(int i = 0; i < NUM_CHILDREN_PER_SPLIT; ++i){
		Uint64 air_count = counts[i][INDEX_COUNT_AIR];
		Uint64 solid_count = counts[i][INDEX_COUNT_SOLID];
		Uint64 child_volume = summary.plane_area * num_child_planes[i];
		Int64 counted_blocks = air_count + solid_count;
		Int64 difference = counted_blocks - child_volume;
		assert(difference == 0);

		PackedData state = VALUE_UNSET_NODE | VALUE_LEAF_NODE;
		if(air_count == child_volume){  // Status: Completely empty
			state = VALUE_EMPTY_LEAF;
		}else if(solid_count == child_volume){  // Status: Completely solid	
			state &= ~FLAG_LEAF_IS_EMPTY;
			if(is_homogenous[i]){
				// WARNING: Assumes palette id is the type's numeric value!
				Bytes2 palette_id_bits = 
					(types[i] << SHIFT_LEAF_PALETTE_INDEX);
				state |= palette_id_bits;

				state &= ~FLAG_LEAF_IS_MIXED_TYPE;  // 100% solid, same type.
			}else{
				state = VALUE_SOLID_MIXED_LEAF;  // 100% solid, mixed type.
			}
		}else{  // Status: Mixed solid/empty.
			state &= ~FLAG_LEAF_IS_EMPTY;
			state |= FLAG_LEAF_IS_MIXED_TYPE;
			
			float fill_percent = ((float) solid_count / child_volume) * 100;
			Bytes2 percent_full = std::max(1.0f, fill_percent);
			assert(percent_full > 0 && percent_full <= 100);
			state |= percent_full << SHIFT_LEAF_PERCENT_FULL;
		}

		partition_results[i] = state;
	}

	return partition_results;
}

VoxelKDTree::TreeData* 
VoxelKDTree::TreeBuilder::buildTree(const ChunkTable& table, 
	VoxelKDTree::BuildSettings settings){
	/*
	Given access to a chunk table and a settings object, build a 
	tree with the given configuration.
	*/

	struct ConstructionBlock{
		int node_depth;
		NodeIndex unfinished_node;
		NodeIndex parent_index;
		ICuboid bounds;
	};

	constexpr Int32 ARBITRARY_INITIAL_NODE_CAPACITY = 20;
	constexpr Int32 ARBITRARY_MAX_TREE_DEPTH = 100;
	constexpr Int32 MAX_POSSIBLE_PREALLOCATION_DEPTH = 30;

	// How much to multiply the capacity by when reallocating
	constexpr float ARBITRARY_CAPACITY_MULTIPLIER = 2.0;

	TreeData tree;
	tree.bounds = settings.bounds;  // World space. Nodes are in "tree space"
	tree.node_capacity = 0;
	tree.node_count = 1;  // Account for the root node.
	tree.curr_max_depth = 0;
	tree.is_packed_tree = settings.should_pack_nodes;
	tree.has_property_nodes = settings.should_calculate_non_leaf_properties;
	tree.geometry_nodes_ptr   = NULL;
	tree.property_nodes_ptr   = NULL;
	tree.descendant_nodes_ptr = NULL;
	//assert(!tree.is_packed_tree); // TODO: Remove once unpacked version is done

	assert(settings.max_depth <= ARBITRARY_MAX_TREE_DEPTH);
	if(settings.should_preallocate_max_node_space){
		assert(settings.max_depth <= MAX_POSSIBLE_PREALLOCATION_DEPTH);
	}

	// Node indices are 32bit values. No matter the depth, we can't get
	// more than the max Int32 value.
	Int64 max_possible_nodes = MAX_INT32_VALUE;
	if(settings.max_depth < MAX_POSSIBLE_PREALLOCATION_DEPTH){
		max_possible_nodes = (1 << (settings.max_depth + 1)) - 1;
	}

	if(settings.should_preallocate_max_node_space){
		assert(settings.max_depth > 0);
		assert(settings.max_depth <= MAX_POSSIBLE_PREALLOCATION_DEPTH);
		resizeToCapacity(tree, max_possible_nodes);
	}else{
		resizeToCapacity(tree, ARBITRARY_INITIAL_NODE_CAPACITY);
	}
	tree.geometry_nodes_ptr[0] = {VALUE_UNSET_NODE};  // Placeholder root

	Int64 total_volume = volume(tree.bounds);
	Int64 processed_volume = 0;
	Int32 num_iterations = 0;

	std::stack<ConstructionBlock> stack;
	ICuboid tree_space_bounds = {{0, 0, 0}, {tree.bounds.extent}};
	stack.push({0, 0, INVALID_NODE_INDEX, tree_space_bounds});
	while(stack.size() > 0){
		auto [curr_depth, curr_index, parent_index, curr_bounds] = stack.top();
		stack.pop();

		float ratio = (float) processed_volume / (float) total_volume;
		printf("BuildTree: %f%% Processed|Iteration %i|Depth:%i"
			"|CurrIndex:%i|ParentIndex:%i\n",
			ratio * 100, ++num_iterations, curr_depth, 
			curr_index, parent_index);

		// STEP 1: Perform split and init current node.
		SplitReferenceData split_reference = {
			.settings=&settings,
			.chunk_table=&table,
			.tree_data=&tree,
			.node_to_split=curr_index,
			.node_cuboid=curr_bounds,
			.node_depth=curr_depth
		};
		SplitResult split_result = generateSplit(split_reference);
		PackedData curr_data = VALUE_UNSET_NODE;
		curr_data |= NODE_TYPES_ARR[split_result.split.axis];
		curr_data |= (split_result.split.offset << SHIFT_PLANE_OFFSET);
		tree.geometry_nodes_ptr[curr_index] = {.pack=curr_data};
		tree.curr_max_depth = max(tree.curr_max_depth, curr_depth);

		// Update descendants array if this is a packed tree
		Int32 child_base_index;
		if(settings.should_pack_nodes){
			child_base_index = tree.node_count;
			tree.descendant_nodes_ptr[curr_index].left_child_index 
				= child_base_index;
		}else{
			child_base_index = 2 * curr_index + 1;
		}

		// See if we need to resize
		if(tree.node_count + 2 > tree.node_capacity){
			Int32 new_size = min(
				(Int32) (tree.node_capacity * ARBITRARY_CAPACITY_MULTIPLIER),
				(Int32) max_possible_nodes);
			bool is_successful = resizeToCapacity(tree, new_size);
			if(!is_successful){
				printf("ERROR: Attempt to resize to capacity %i failed!\n",
					new_size);
				return NULL;
			}
		}

		// STEP 2: Deal with the children
		for(int i = 0; i < 2; ++i){
			ICuboid node_bounds = split_result.cuboids[i];
			GeometryNode node = split_result.children[i];
			bool is_leaf_node = split_result.is_leaf_node[i];
			processed_volume += (volume(node_bounds) * is_leaf_node);

			Int32 child_index = child_base_index + i;
			if(is_leaf_node){
				//node_counts[INDEX_LEAF_AIR_COUNT]
				tree.geometry_nodes_ptr[child_index] = node;
			}else{
				// This node will need to be split. 
				// Pack a ConstructionBlock and stick it on the stack.
				ConstructionBlock new_block = {
					.node_depth=curr_depth+1,
					.unfinished_node=child_index,
					.parent_index=curr_index,
					.bounds=node_bounds,
				};
				stack.push(new_block);
			}

			if(split_result.is_properties_defined[i]){
				// TODO: Implement
				assert(false);
			}
		}
		tree.node_count += 2;
	}
	debuggingPrintTreeStats(&tree);
	
	// Return a pointer to the struct instead of the struct itself.
	TreeData* tree_data_ptr = (TreeData*) malloc(sizeof(TreeData));
	if(tree_data_ptr != NULL){
		*tree_data_ptr = tree;
	}
	printf("\a\n");
	return tree_data_ptr;
}

//-----------------------------------------------------------------------------
// VoxelKDTree
//-----------------------------------------------------------------------------
bool VoxelKDTree::resizeToCapacity(VoxelKDTree::TreeData& tree, 
	Int32 capacity){
	/*
	Resize the internal array to have at least the given capacity
	Returns False if something went wrong, and True otherwise.
	*/

	struct PointerProperties{
		void** ptr;
		int element_size;
		bool should_reallocate;
	};

	PointerProperties tree_pointer_properties[] = {
		{(void**)&tree.geometry_nodes_ptr,   sizeof(GeometryNode),
			true},
		{(void**)&tree.property_nodes_ptr,   sizeof(PropertyNode),
			tree.has_property_nodes},
		{(void**)&tree.descendant_nodes_ptr, sizeof(DescendantNode),
			tree.is_packed_tree},
	};

	if(tree.node_capacity < capacity){
		for(PointerProperties& prop : tree_pointer_properties){
			auto [ptr, element_size, should_reallocate] = prop;

			if(should_reallocate){
				void* result = realloc(*ptr, element_size * capacity);
				if(result == NULL){
					printf("Reallocation Failed!\n");
					return false;
				}else{
					*ptr = result;
				}
			}
		}

		tree.node_capacity = capacity;	
	}

	return true;
}

void VoxelKDTree::freeTreeData(TreeData*& data){
	if(data == NULL){
		return;
	}
	
	free(data->geometry_nodes_ptr);
	free(data->property_nodes_ptr);
	free(data->descendant_nodes_ptr);
	free(data);
	data = NULL;
}

bool isLeaf(VoxelKDTree::PackedData data){
	return (data & VoxelKDTree::MASK_NODE_TYPE_BITS) == 
		VoxelKDTree::VALUE_LEAF_NODE;
}

VoxelKDTree::TreeData* VoxelKDTree::loadTreeFromFile(std::string filepath, 
	bool is_packed){
	/*
	TODO: Init this on a bytebuffer instead of loading from a filepath
	TODO: Calculate the max depth when loading.
	*/

	constexpr Int32 ARBITRARY_TREE_SIZE_LIMIT = 1 << 25;
	struct TreeHeader{
		Uint8 version[3];
		Uint8 padding[1];
		Uint32 tree_id;
		Uint32 node_count;
		ICuboid bounds;
	};
	
	FileIO::ByteBuffer buffer(filepath);
	buffer.setCursorIndex(0);

	TreeHeader header;
	buffer.copyFromCursor(sizeof(TreeHeader), &header);
	header.tree_id = ntohl(header.tree_id);
	header.node_count = FileIO::flipEndian((Uint32)header.node_count);
	header.bounds = {
		FileIO::flipEndian(header.bounds.origin),
		FileIO::flipEndian(header.bounds.extent)
	};
	
	Int32 node_count = header.node_count;
	Int32 node_capacity = node_count;  // TODO: Extra for resizes
	
	TreeData* tree_ptr;
	GeometryNode* g_nodes_ptr;
	DescendantNode* d_nodes_ptr;
	if(node_count > ARBITRARY_TREE_SIZE_LIMIT){
		printf("ERROR: Node count was %i, above max limit of %i\n", 
			node_count, ARBITRARY_TREE_SIZE_LIMIT);
		return NULL;
	}else if(!is_packed){
		// TODO: Remove this once unpacking is implemented
		printf("ERROR: Loaded trees must currently be packed!\n");
		return NULL;
	}else{
		// Allocate memory and fill tree struct
		tree_ptr = (TreeData*) malloc(sizeof(TreeData));
		g_nodes_ptr = (GeometryNode*) malloc(node_capacity * 
			sizeof(GeometryNode));
		d_nodes_ptr = (DescendantNode*) malloc(node_capacity * 
			sizeof(DescendantNode));
		tree_ptr->geometry_nodes_ptr = g_nodes_ptr;
		tree_ptr->descendant_nodes_ptr = d_nodes_ptr;
		tree_ptr->property_nodes_ptr = NULL;

		tree_ptr->bounds = header.bounds;
		tree_ptr->node_capacity = node_capacity;
		tree_ptr->node_count = node_count;
		tree_ptr->curr_max_depth = 0;
		tree_ptr->is_packed_tree = is_packed;
		tree_ptr->has_property_nodes = false;
	}

	struct Node{
		PackedData data;
		Int32 stream_index{0};
		Int32 num_prior_leaves{0};

		Int32 target_index{0};  // Calculated after creation
	};

	constexpr int LEFT_MEMBER_INDEX = 0;
	constexpr int PAIR_FULL = 2;
	struct StackPair{
		/*
		Helps pair up left/right children when deserializing a 
		stream of nodes that doesn't necessarily put them next
		to each other.
		*/
		
		int num_filled{0};
		Node members[2] = {};
		int output_index{0};

		int addMember(Node node){
			members[num_filled] = node;
			return ++num_filled;
		}
	};

	printf("Loading tree with %i nodes\n", node_count);
	Uint64 node_arr_start_index = sizeof(TreeHeader);
	buffer.setCursorIndex(node_arr_start_index);
	
	Int32 max_depth = 0;
	Int32 num_prior_leaves = 0;
	std::stack<StackPair> stack;
	stack.push(StackPair());
	for(Int32 n = 0; n < node_count; ++n){
		if(stack.size() == 0){
			freeTreeData(tree_ptr);
			return NULL;
		}

		Int64 node_byte_index = node_arr_start_index + n * sizeof(PackedData);
		PackedData node_data = ntohs(buffer.readBytes2(node_byte_index));

		// Add this node to the first available position in the top pair, then 
		// see if we need to push another pair or not.
		StackPair& top_pair = stack.top();
		int active_index = top_pair.num_filled;
		top_pair.addMember({node_data, n, num_prior_leaves});
		if(active_index == LEFT_MEMBER_INDEX){
			// At this point we have all the information needed to fully
			// calculate the output position of the pair.
			// NOTE: Shifting down by 1 is OK because root never has a sibling.
			Node& lhs = top_pair.members[LEFT_MEMBER_INDEX];
			top_pair.output_index = 
				(lhs.stream_index - lhs.num_prior_leaves) * 2 - 1;
		}

		// Update stack if this was an internal node.
		if(isLeaf(node_data)){
			++num_prior_leaves;
		}else{
			stack.push(StackPair());

			// Also track the max depth	
			if(stack.size() > max_depth){
				max_depth = stack.size();
			}
		}

		// Finally, pop off all the filled pairs until we hit one
		// with open spaces
		while(stack.size() > 0 && stack.top().num_filled == PAIR_FULL){
			StackPair filled_pair = stack.top();
			stack.pop();
			
			// If the active member of the new stack top isn't a leaf, then 
			// they're an internal node that needs to know where to point.
			// Update their target index to point to the just removed pair's 
			// output index.
			StackPair& top_pair = stack.top();
			active_index = top_pair.num_filled - 1;
			assert(active_index >= 0);  // Never push a pair over an empty pair
			Node& active_member = top_pair.members[active_index];
			if(!isLeaf(active_member.data)){
				active_member.target_index = filled_pair.output_index;
			}

			// Write the data to their respective arrays
			assert(filled_pair.output_index < node_count);
			for(int i = 0; i < 2; ++i){
				int index = filled_pair.output_index + i;
				g_nodes_ptr[index] = {filled_pair.members[i].data};
				d_nodes_ptr[index] = {filled_pair.members[i].target_index};
			}
		}
	}
	// Direct assign the root. It's never supposed to have a sibling.
	assert(stack.size() == 1);
	g_nodes_ptr[0] = {stack.top().members[0].data};
	d_nodes_ptr[0] = {stack.top().members[0].target_index};
	tree_ptr->curr_max_depth = max_depth;
	
	if(!is_packed && false){
		// Traverse the array once to figure out how much memory to allocate, 
		// then do a second pass using the 2n+1+i rule to place the nodes.
		// Finally, remove the descendant nodes list.
		
		// TODO: 1st Traversal

		// TODO: 2nd Traversal

		free(d_nodes_ptr);
	}

	// Print the nodes
	// debuggingPrintTreeContents(tree_ptr);
	return tree_ptr;
}


//-----------------------------------------------------------------------------
// Mesh KDTree
//-----------------------------------------------------------------------------
/*
struct TriangleMesh{
	struct Triangle{
		FVec3 positions[3];
		FVec3 normals[3];
		FVec2 uvs[3];
	};

	struct NamedRange{
		std::string name;
		Range32 range;
	};

	std::vector<Triangle> triangles;
	std::vector<NamedRange> material_ranges;
	std::vector<NamedRange> group_ranges;
};

*/

MeshKDTree::TreeData MeshKDTree::buildTree(const TriangleMesh& mesh){
	/*
	TODO: Actually implement splitting
	*/

	TreeData tree_data;

	tree_data.intersect_data.reserve(mesh.triangles.size());
	tree_data.surface_data.reserve(mesh.triangles.size());
	for(TriangleMesh::Triangle triangle : mesh.triangles){
		Triangle tri_intersect;
		TriangleSurfaceData tri_surface;
		for(int i = 0; i < 3; ++i){
			tri_intersect.vertices[i] = triangle.positions[i];
			tri_surface.uvs[i] =        triangle.uvs[i];
			tri_surface.normals[i] =    triangle.normals[i];
		}

		tree_data.intersect_data.push_back(tri_intersect);
		tree_data.surface_data.push_back(tri_surface);
	}

	return tree_data;
}

