#include "RayTracing.hpp"


//-------------------------------------------------------------------------------------------------
// Raytracing-specific helper functions
//-------------------------------------------------------------------------------------------------
int smallestIndexBranchless(float x, float y, float z){
	/*
	Returns the index of the smallest float without branching. Assumes an array of floats
	with x at 0, y at 1, and z at 2.

	TODO: I don't know if this is actually worth it. Need to profile this vs smallestIndex().
	*/

	int is_x_below_y = x < y;
	int x_answer = (x > z) * 2;
	int y_answer = (y > z) + 1;
	int smallest = (is_x_below_y * x_answer) + (!is_x_below_y * y_answer);
	
	return smallest;
}

int smallestIndexBranchless(FVec3 vec){
	return smallestIndexBranchless(vec.x, vec.y, vec.z);
}

int smallestIndex(float x, float y, float z){
	if(x < y){
		// x < y
		if(x < z){
			return 0;
		}else{
			return 2;
		}
	}else{
		// y < x
		if(y < z){
			return 1;
		}else{
			return 2;
		}
	}
}

int largestIndex(FVec3 vec){
	auto [x, y, z] = vec;
	if(x > y){
		// x > y
		if(x > z){
			return 0;
		}else{
			return 2;
		}
	}else{
		// y > x
		if(y > z){
			return 1;
		}else{
			return 2;
		}
	}
}

std::vector<Rendering::ImageTile> Rendering::tiles(
	Camera camera, Rendering::ImageConfig config){
	/*
	Returns a vector of tiles for use in tiled rendering.
	*/

	std::vector<ImageTile> tiles;

	assert(config.num_pixels[0] > 0 && config.num_pixels[1] > 0);
	assert(config.tile_dims[0] > 0 && config.tile_dims[1] > 0);

	// Calculate tile dimensions for both the regular and end tiles
	IVec2 num_tiles;
	IVec2 full_tile_dims = config.tile_dims;
	IVec2 uneven_tile_dims;
	for(Int32 i = 0; i < 2; ++i){
		num_tiles[i] = (Int32) ceil((float)config.num_pixels[i] / config.tile_dims[i]);
		
		Int32 full_tiles_pixel_coverage = config.tile_dims[i] * (num_tiles[i] - 1);
		uneven_tile_dims[i] = config.num_pixels[i] - full_tiles_pixel_coverage;
	}

	// 
	Int32 num_total_tiles = num_tiles[0] * num_tiles[1];
	tiles.reserve(num_total_tiles);

	// Construct the tile structs
	bool is_end_tile[2];
	for(Int32 x = 0; x < num_tiles.x; ++x){
		is_end_tile[0] = (x == num_tiles.x - 1);
		for(Int32 y = 0; y < num_tiles.y; ++y){
			is_end_tile[1] = (y == num_tiles.y - 1);
		
			ImageTile new_tile;
			Range32* tile_ranges[] = {&new_tile.range_x, &new_tile.range_y};
			IVec2 dimensions = {x, y};
			for(Int32 dim = 0; dim < 2; ++dim){
				Int32 tile_start = dimensions[dim] * full_tile_dims[dim];
				Int32 tile_extent = is_end_tile[dim] ? uneven_tile_dims[dim] : full_tile_dims[dim];
				*tile_ranges[dim] = {tile_start, tile_extent};
			}
			tiles.push_back(new_tile);
		}
	}

	return tiles;
}

Rendering::CameraRayGenerator::CameraRayGenerator(){
	
}

Rendering::CameraRayGenerator::CameraRayGenerator(Camera camera, IVec2 image_dims){
	/*
	Precalculate all terms that won't change for each ray
	*/

	// Convert to coordinate system where +X is right, +Y is down, and -Z is the view direction.
	FVec3 image_plane_normal = camera.basis.v1.normal();
	FVec3 image_x = image_plane_normal.cross(camera.basis.v2).normal();
	FVec3 image_y = image_x.cross(image_plane_normal).normal();
	FVec3 image_z = image_y.cross(image_x).normal();  // Intentionally NOT X.cross(Y)

	// This term will stay the same for all generated rays, so it's worth 
	// pre-computing
	FVec2 image_half_dims = toFloatVector(image_dims) * 0.5;
	FVec3 w_prime = 
		image_x * -image_half_dims.x -
		image_y * -image_half_dims.y +
		image_z * (image_half_dims.y / tan(degreesToRadians(camera.fov) * 0.5));

	// Save results
	m_image_basis = {image_x, image_y, image_z};
	m_image_dims = image_dims;
	m_camera_pos = camera.pos;
	m_w_prime = w_prime;
}

Ray Rendering::CameraRayGenerator::rayFromPixelCoord(IVec2 coord) const{
	/*
	Given pixel coordinates, return a ray for that pixel.
	TODO: See if we can get away with not normalizing these.
	*/

	FVec3 ray_direction = {
		(coord.x * m_image_basis.v0) - 
		(coord.y * m_image_basis.v1) + 
		m_w_prime
	};

	Ray new_ray = {m_camera_pos, ray_direction};
	return new_ray.normal();
}

std::vector<Ray> Rendering::allRays(Camera camera, ImageConfig config){
	/*
	Just fire rays across an image without any tiling
	*/

	std::vector<Ray> output_rays;
	output_rays.reserve(config.num_pixels.x * config.num_pixels.y);

	CameraRayGenerator generator(camera, config.num_pixels);

	for(int y = 0; y < config.num_pixels.y; ++y){
		for(int x = 0; x < config.num_pixels.x; ++x){
			output_rays.push_back(generator.rayFromPixelCoord({x, y}));
		}
	}

	return output_rays;
}

//-------------------------------------------------------------------------------------------------
// Misc local helper functions
//-------------------------------------------------------------------------------------------------
IVec3 floorToInt(FVec3 vec){
	return {(int)floor(vec.x), (int)floor(vec.y), (int)floor(vec.z)};
}


IVec3 rayStep(FVec3 direction){
	/*
	The grid direction to step in for each dimension based on the directionality of the ray
	*/

	IVec3 directions;
	for(int i = 0; i < 3; ++i){
		if(direction[i] >= 0){
			directions[i] = 1;
		}else{
			directions[i] = -1;
		}
	}
	return directions;
}


//-------------------------------------------------------------------------------------------------
// Header raytracing code
//-------------------------------------------------------------------------------------------------
Intersection::Utils::GridRay
Intersection::Utils::localChunkIntersection(
	Intersection::Utils::GridRay grid_ray, const RawVoxelChunk& chunk){
	/*
	Using cached ray information, intersect the chunk and return whether or not a hit
	occurred. This is a helper function for intersectChunks and should not be called elsewhere.

	ATTRIBUTION: Efficient Ray-Grid traversal algorithm.
		https://www.scratchapixel.com/lessons/3d-basic-rendering/
			introduction-acceleration-structure/grid

	NOTE: Returns hit information in local space.
	YAGNI: Instead of 3D coord->linear index, have a value that just adds each time.
	*/
	
	IVec3& grid_coord = grid_ray.local_grid_coord;
	int smallest;  // The last stepped axis is needed when the loop terminates
	while(true){
		smallest = smallestIndexBranchless(grid_ray.t_next_crossing);
		grid_ray.t_next_crossing[smallest] += grid_ray.delta_t[smallest];
		grid_coord[smallest] += grid_ray.step_dir[smallest];
		
		// Check if the ray went out of bounds
		if(grid_coord.x >= CHUNK_LEN || grid_coord.y >= CHUNK_LEN || grid_coord.z >= CHUNK_LEN || 
			grid_coord.x < 0 || grid_coord.y < 0 || grid_coord.z < 0){

			break;
		}

		VoxelType type = chunk.data[linearChunkIndex(grid_coord)].type;
		if(!isAir(type)){
			grid_ray.is_hit = true;
			grid_ray.hit_voxel_type = type;
			break;
		}
	}
	grid_ray.last_stepped_axis = smallest;

	return grid_ray;
}

Intersection::Utils::GridRay
Intersection::Utils::traverseEmptyChunk(
	Intersection::Utils::GridRay grid_ray, Debug::DebugData& data){
	/*
	Used to accelerate traversal through a chunk where we know there won't
	be any collisions. 

	This is a helper function for intersectChunks and should not be called elsewhere.

	YAGNI: This could be done using a Ray-AABB intersection test, but I
	keep getting t offset errors whenever I try. Figure
	out why and replace the existing code to remove this stupid loop.
	*/

	IVec3& grid_coord = grid_ray.local_grid_coord;
	int smallest;
	while(true){
		smallest = smallestIndexBranchless(grid_ray.t_next_crossing);
		grid_ray.t_next_crossing[smallest] += grid_ray.delta_t[smallest];
		grid_coord[smallest] += grid_ray.step_dir[smallest];
		
		// Check if the ray went out of bounds
		if(grid_coord.x >= CHUNK_LEN || grid_coord.y >= CHUNK_LEN || grid_coord.z >= CHUNK_LEN || 
			grid_coord.x < 0 || grid_coord.y < 0 || grid_coord.z < 0){

			break;
		}
	}
	grid_ray.last_stepped_axis = smallest;

	return grid_ray;
}

RayIntersection 
Intersection::intersectChunks(Ray ray, const ChunkTable* table_ptr){
	/*

	*/

	RayIntersection intersection{INTERSECT_MISS};

	// Don't trace if the ray won't hit the world. Otherwise, advance it to
	// the first loaded chunk in the ray's path.
	ICuboid chunk_bounds = table_ptr->boundingVolumeChunkspace();
	RayIntersection box_hit = intersectCollider(ray, chunk_bounds * CHUNK_LEN);
	if(box_hit.type == INTERSECT_HIT_COLLIDER || box_hit.type == INTERSECT_INTERNAL_COLLIDER){
		ray.origin = ray.origin + ray.dir * box_hit.t_hit;
	}else{
		return intersection;
	}
	
	// The ray is almost never going to start in the exact corner of a voxel, so the
	// initial t value for the first crossing will be different than all subsequent ones.
	FVec3 dir = ray.dir.normal();
	FVec3 t_initial_crossing = {0, 0, 0};
	for(int i = 0; i < 3; ++i){
		int is_positive = ray.dir[i] >= 0;
		float initial_t = (floor(ray.origin[i]) + is_positive - ray.origin[i]) / dir[i];
		t_initial_crossing[i] = initial_t;
	}
	IVec3 global_voxel_coord = floorToInt(ray.origin);

	// Some of these computations only need to be done one time per ray. Calculate once
	// and re-use for each chunk intersection.
	Intersection::Utils::GridRay grid_ray;
	grid_ray.delta_t = {abs(1.0f / dir.x), abs(1.0f / dir.y), abs(1.0f / dir.z)};
	grid_ray.step_dir = rayStep(dir);
	grid_ray.local_grid_coord = localVoxelCoordFromGlobal(global_voxel_coord);
	grid_ray.t_next_crossing = t_initial_crossing;
	grid_ray.is_hit = false;

	IVec3 curr_chunk_coord = chunkCoordFromVoxelCoord(global_voxel_coord);
	while(table_ptr->isLoaded(curr_chunk_coord)){
		RawVoxelChunk chunk_data = *table_ptr->getChunkPtr(curr_chunk_coord);
		
		grid_ray = localChunkIntersection(grid_ray, chunk_data);
		if(grid_ray.is_hit){
			// For the last grid step, the ray enters the voxel, putting the t value beyond the
			// contact value. Pull the t value back by one step length to undo this and get the 
			// t value at the surface.
			int hit_axis = grid_ray.last_stepped_axis;
			float contact_t = grid_ray.t_next_crossing[hit_axis] - grid_ray.delta_t[hit_axis];
			bool is_axis_dir_negative = dir[hit_axis] < 0;
			int face_index = hit_axis * 2 + is_axis_dir_negative;

			IVec3 world_hit_coord = curr_chunk_coord * CHUNK_LEN + grid_ray.local_grid_coord;
			intersection.type = INTERSECT_HIT_CHUNK_VOXEL;
			intersection.t_hit = contact_t;
			intersection.voxel_hit.voxel = world_hit_coord;
			intersection.voxel_hit.face_index = (GridDirection) face_index;
			intersection.voxel_hit.palette_index = grid_ray.hit_voxel_type;
			break;
		}else{
			IVec3 offset_to_next_chunk = chunkCoordFromVoxelCoord(grid_ray.local_grid_coord);
			grid_ray.local_grid_coord = localVoxelCoordFromGlobal(grid_ray.local_grid_coord);
			curr_chunk_coord += offset_to_next_chunk;
		}
	}

	return intersection;
}


RayIntersection 
Intersection::intersectTree(Ray ray, const VoxelKDTree::TreeData* tree){
	/*
	Passthrough function to simplify intersection calls for functions that don't need to worry
	about the efficiency hit of reallocating the stack every time. That's justified for
	the raytracer, but not for stuff like one-off collision checks.
	*/

	Intersection::Utils::VKDTStack stack = Intersection::Utils::VKDTStack::init(
		tree->curr_max_depth);
	auto result = intersectTree(ray, tree, stack);
	stack.freeMemory();
	return result;
}

RayIntersection 
Intersection::intersectTree(Ray ray, const VoxelKDTree::TreeData* tree, 
	Intersection::Utils::VKDTStack stack){
	/*
	NOTE: Assumes normalized ray
	ATTRIBUTION: A traversal optimization to reduce stack operations by only pushing the far child 
		in the case of a double-hit and looping down the near child until a leaf is hit. 
		Only then is the next node popped. Ensures the stack is only used when a "fork"
		is explored in the tree. 
		-Virginia Tech lecture slides "Acceleration Structure For Ray Tracing" by Yong Cao
	
	NOTE: The stack is a simple POD struct passed in as a glorified array
		pointer with helper methods attached. This is meant to avoid reallocating
		memory every time intersectTree is called. At the beginning of a batch trace,
		the calling function allocates enough space for the stack to push 2*max_depth, 
		which would occur in a situation where every single node all the way down the tree 
		was a double hit. It might be possible to halve that minus a constant factor 
		since only half of nodes are actually pushed, but it isn't worth it at the moment.
	*/

	constexpr IntersectionType leaf_intersection_types[] = {
		INTERSECT_POSSIBLE_CHUNK_VOXEL,
		INTERSECT_HIT_CHUNK_VOXEL_UNKNOWN_TYPE,
		INTERSECT_HIT_CHUNK_VOXEL,
	};

	// Bail out early if the ray didn't even hit
	DetailedCuboidIntersection bounds_hit = Intersection::intersectColliderDetailed(
		ray, tree->bounds);
	RayIntersection hit_state = {INTERSECT_MISS};
	if(!bounds_hit.is_valid){
		return hit_state;
	}else if(bounds_hit.t_bounds[INDEX_VALUE_MAX] < 0){
		return hit_state;
	}
	
	FVec3 tree_world_offset = toFloatVector(tree->bounds.origin);
	Ray inverse_local_ray = {
		.origin=ray.origin - tree_world_offset,  // Local to tree origin
		.dir={  // Inverse dir
			1.0f / ray.dir.x, 
			1.0f / ray.dir.y,
			1.0f / ray.dir.z
		}
	};

	//stack.height = 0;  // Stack is a copy which always starts at 0
	VoxelKDTree::NodeIndex curr_node_index = 0;
	VoxelKDTree::PackedData curr_data;
	Axis last_min_axis = bounds_hit.last_min_axis;  // Last axis that raised the t_min value.
	Axis plane_axis;

	Bytes2 curr_type;
	float t_plane;
	float t_min = std::max(bounds_hit.t_bounds[INDEX_VALUE_MIN], 0.0f);
	float t_max = bounds_hit.t_bounds[INDEX_VALUE_MAX];

	bool is_backtrack_required = false;  // Happens after hitting a leaf
	while(!is_backtrack_required || stack.height > 0){
		// Get info for the current node prepared
		if(is_backtrack_required){
			is_backtrack_required = false;
			Utils::VKDTTraversalNode stack_top = stack.pop();
			curr_node_index = stack_top.curr_node_index;
			t_min = stack_top.t_min;
			t_max = stack_top.t_max;
			last_min_axis = stack_top.axis;
		}

		// Figure out node info
		curr_data = tree->geometry_nodes_ptr[curr_node_index].pack;
		curr_type = VoxelKDTree::nodeType(curr_data);

		// Leaf handling
		if(curr_type == VoxelKDTree::VALUE_LEAF_NODE){
			bool is_empty_leaf = curr_data & VoxelKDTree::FLAG_LEAF_IS_EMPTY;
			if(is_empty_leaf){
				is_backtrack_required = true;
				continue;
			}else{
				// WARNING: The voxel hit properties are being calculated in an unsafe manner
				// 	voxel_hit.voxel is likely to be off-by-one
				bool is_fully_confirmed = VoxelKDTree::isHomogenousLeaf(curr_data);
				bool is_unknown_type = (curr_data == VoxelKDTree::VALUE_SOLID_MIXED_LEAF);
				int hit_type_index = is_fully_confirmed * 2 + is_unknown_type;
				assert(hit_type_index >= 0 && hit_type_index < 3);

				// The last axis to reduce t_min (with sign for direction) is used
				// to index into an array for normal vectors.
				bool is_axis_dir_negative = ray.dir[last_min_axis] < 0;
				int face_index = last_min_axis * 2 + is_axis_dir_negative;

				// Fill in struct fields and return
				hit_state.type = leaf_intersection_types[hit_type_index];
				hit_state.t_hit = t_min;
				hit_state.voxel_hit.voxel = floorToInt(posFromT(ray, t_min));  // TODO: Verify. This feels wrong
				hit_state.voxel_hit.face_index = (GridDirection) face_index;
				if(is_fully_confirmed){
					hit_state.voxel_hit.palette_index = VoxelKDTree::paletteIndex(curr_data);
				}else{
					hit_state.voxel_hit.palette_index = 0;
				}

				return hit_state;
			}
		}

		// We're still traversing internal nodes
		plane_axis = (Axis) curr_type;
		Uint16 plane_offset = VoxelKDTree::planeOffset(curr_data);
		VoxelKDTree::NodeIndex child_base_index;
		if(tree->is_packed_tree){
			child_base_index = tree->descendant_nodes_ptr[curr_node_index].left_child_index;
		}else{
			child_base_index = 2 * curr_node_index + 1;
		}
		
		// Calculate the t_intersect value of the plane.
		t_plane = (plane_offset - inverse_local_ray.origin[plane_axis]) * 
			inverse_local_ray.dir[plane_axis];
		bool should_flip = ray.dir[plane_axis] < 0;

		if(t_plane <= t_min){
			// Definitely missed the near child, need to check the far.
			curr_node_index = child_base_index + !should_flip;
			continue;
		}

		if(t_plane >= t_max){
			// Definitely missed the far child, need to check the near
			curr_node_index = child_base_index + should_flip;
			continue;
		}

		// Both got hit. Push far child before continuing.
		Utils::VKDTTraversalNode far_info = {
			.curr_node_index=child_base_index+!should_flip,
			.t_min=t_plane,
			.t_max=t_max,
			.axis=plane_axis,
		};
		stack.push(far_info);
		
		curr_node_index = child_base_index + should_flip;
		t_max = t_plane;
		if(t_max <= t_min){
			break;
		}
	}

	hit_state.type = INTERSECT_MISS;
	return hit_state;
}


RayIntersection Intersection::intersectTriangle(Ray ray, const Triangle& triangle){
	/*
	ATTRIBUTION: Möller–Trumbore ray-triangle intersection algorithm example code:
		https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm

	ATTRIBUTION: Another explanation of Möller–Trumbore with example code.
		https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/
		moller-trumbore-ray-triangle-intersection
	*/

	RayIntersection intersection = {INTERSECT_MISS};

	const FVec3& v0 = triangle.vertices[0];
	const FVec3& v1 = triangle.vertices[1];
	const FVec3& v2 = triangle.vertices[2];

	FVec3 edge1 = v1 - v0;
	FVec3 edge2 = v2 - v0;
	FVec3 h = ray.dir.cross(edge2);
	float a = edge1.dot(h);

	// Ray is parallel to the plane
	if(abs(a) < ARBITRARY_EPSILON){
		return intersection;
	}

	float f = 1.0f / a;
	
	FVec3 s = ray.origin - v0;
	float u = f * s.dot(h);
	if(u < 0.0 || u > 1.0){
		return intersection;
	}

	FVec3 q = s.cross(edge1);
	float v = f * ray.dir.dot(q);
	if(v < 0 || u + v > 1.0){
		return intersection;
	}

	float t = f * edge2.dot(q);
	bool is_valid = t > 0.0f;
	intersection.type = is_valid ? INTERSECT_HIT_TRIANGLE_MESH : INTERSECT_MISS;
	intersection.t_hit = t;
	intersection.unaligned_hit.normal = h.normal();
	intersection.unaligned_hit.triangle_index = -1;
	return intersection;
}

RayIntersection Intersection::intersectTree(Ray ray, const MeshKDTree::TreeData* tree){
	/*
	Father forgive me, for I have sinned.

	TODO: An actual tree traversal and not linear search.
	*/

	RayIntersection best_hit{INTERSECT_MISS};
	best_hit.t_hit = LARGE_FLOAT;
	for(Uint64 i = 0; i < tree->intersect_data.size(); ++i){
		const Triangle& triangle = tree->intersect_data[i];
		RayIntersection triangle_hit = intersectTriangle(ray, triangle);
		if(isValid(triangle_hit) && triangle_hit.t_hit < best_hit.t_hit){
			best_hit = triangle_hit;
			best_hit.unaligned_hit.triangle_index = i;
		}
	}

	return best_hit;
}

RayIntersection Intersection::intersectCollider(Ray ray, const ICuboid& cuboid){
	/*
	ATTRIBUTION: Code structure
		https://www.scratchapixel.com/lessons/3d-basic-rendering/
		minimal-ray-tracer-rendering-simple-shapes/ray-box-intersection
	
	ATTRIBUTION: Ray vs AABB intersection code from Andrew Kensler
		http://psgraphics.blogspot.com/2016/02/
			new-simple-ray-box-test-from-andrew.html
	*/
	
	float t_min = 0;
	float t_max = LARGE_FLOAT;
	bool is_valid = true;

	FVec3 max_bounds = toFloatVector(cuboid.origin + cuboid.extent);
	for(int axis = 0; axis < NUM_3D_AXES; ++axis){
		float inverse_dir = 1.0f / ray.dir[axis];
		float t0 = (cuboid.origin[axis] - ray.origin[axis]) * inverse_dir;
		float t1 = (max_bounds[axis] - ray.origin[axis]) * inverse_dir;
		if(inverse_dir < 0.0f){
			float temp = t1;
			t1 = t0;
			t0 = temp;
		}

		t_min = t0 > t_min ? t0 : t_min;
		t_max = t1 < t_max ? t1 : t_max;
		if(t_max <= t_min){
			is_valid = false;
			break;
		}
	}

	RayIntersection result;
	result.type = is_valid ? INTERSECT_HIT_COLLIDER : INTERSECT_INVALID;
	result.t_hit = t_min;
	return result;
}

RayIntersection Intersection::intersectCollider(Ray ray, const FSphere& sphere){
	RayIntersection dummy_intersection;
	dummy_intersection.type = INTERSECT_INVALID;
	return dummy_intersection;
}


RayIntersection Intersection::intersectCollider(Ray ray, const FCuboid& cuboid){
	RayIntersection dummy_intersection;
	dummy_intersection.type = INTERSECT_INVALID;
	return dummy_intersection;
}

RayIntersection Intersection::intersectCollider(Ray ray, const FCylinder& cylinder){
	RayIntersection dummy_intersection;
	dummy_intersection.type = INTERSECT_INVALID;
	return dummy_intersection;
}

RayIntersection Intersection::intersectCollider(Ray ray, const FCone& cone){
	RayIntersection dummy_intersection;
	dummy_intersection.type = INTERSECT_INVALID;
	return dummy_intersection;
}

RayIntersection Intersection::intersectCollider(Ray ray, const FRect& rectangle){
	RayIntersection dummy_intersection;
	dummy_intersection.type = INTERSECT_INVALID;
	return dummy_intersection;
}

RayIntersection Intersection::intersectCollider(Ray ray, const FLine& line){
	RayIntersection dummy_intersection;
	dummy_intersection.type = INTERSECT_INVALID;
	return dummy_intersection;
}

RayIntersection Intersection::intersectCollider(Ray ray, const FPlane& plane){
	RayIntersection dummy_intersection;
	dummy_intersection.type = INTERSECT_INVALID;
	return dummy_intersection;
}


Intersection::DetailedCuboidIntersection
Intersection::intersectColliderDetailed(Ray ray, const ICuboid& cuboid){
	/*
	Returns vector of t_min, t_max for the box, as well as a bool indicating
	whether the ray missed entirely.

	ATTRIBUTION: Ray vs AABB intersection code from Andrew Kensler
		http://psgraphics.blogspot.com/2016/02/
			new-simple-ray-box-test-from-andrew.html
	*/
	
	float t_min = -LARGE_FLOAT;
	float t_max = LARGE_FLOAT;
	float tmin_old;
	bool is_valid = true;

	Axis last_min_axis = AXIS_X;

	FVec3 max_bounds = toFloatVector(cuboid.origin + cuboid.extent);
	for(int axis = 0; axis < NUM_3D_AXES; ++axis){
		float inverse_dir = 1.0f / ray.dir[axis];
		float t0 = (cuboid.origin[axis] - ray.origin[axis]) * inverse_dir;
		float t1 = (max_bounds[axis] - ray.origin[axis]) * inverse_dir;
		if(inverse_dir < 0.0f){
			float temp = t1;
			t1 = t0;
			t0 = temp;
		}

		tmin_old = t_min;
		t_min = t0 > t_min ? t0 : t_min;
		t_max = t1 < t_max ? t1 : t_max;
		if(t_min > tmin_old){
			last_min_axis = (Axis)axis;
		}
		if(t_max <= t_min){
			is_valid = false;
			break;
		}
	}

	DetailedCuboidIntersection result;
	result.t_bounds[INDEX_VALUE_MIN] = t_min;
	result.t_bounds[INDEX_VALUE_MAX] = t_max;
	result.is_valid = is_valid;
	result.last_min_axis = last_min_axis;
	return result;
}

Intersection::DetailedSphereIntersection 
Intersection::intersectColliderDetailed(Ray ray, const FSphere& sphere){
	/*
	ATTRIBUTION: Ray-sphere intersection formula
		Source: Just trust me bro. In all seriousness, I didn't derive this formula 
		myself, but can't remember where it's from.
		It's basically just finding the length of side P of a right triangle where:
			Side P: Half of the line segment made by extending the ray dir through the sphere.
			Side Q: The line from the sphere center to the intersecting segement perpendicular to P.
			Side R: The radius of the sphere (Hypotenuse of the triangle)
		This is using sides R and Q to solve for the length of P. This is then subtracted from and
		added to the t_to_hit_average value to get the values of t where the ray
		enters and exits respectively.
	*/
	
	DetailedSphereIntersection result;
	
	FVec3 to_sphere = sphere.origin - ray.origin;
	float t_to_hit_average = to_sphere.dot(ray.dir);
	FVec3 side_q = sphere.origin - (ray.origin + ray.dir * t_to_hit_average);
	float q_squared = side_q.x * side_q.x + side_q.y * side_q.y + side_q.z * side_q.z;
	float r_squared = sphere.radius * sphere.radius;
	result.is_valid = (q_squared <= r_squared);
	if(result.is_valid){
		float p_squared = r_squared - q_squared;
		float p_length = sqrt(p_squared);
		float t_enter = t_to_hit_average - p_length;
		float t_leave = t_to_hit_average + p_length;

		result.is_valid = t_enter > 0;  // A -enter +leave result means the ray was inside the sphere.
		result.t_bounds[INDEX_VALUE_MIN] = t_enter;
		result.t_bounds[INDEX_VALUE_MAX] = t_leave;
	}
	
	return result;
}

