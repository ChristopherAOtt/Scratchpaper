#pragma once

#include "Chunks.hpp"
#include "Primitives.hpp"
#include "Geometry.hpp"  // Ray is defined here
#include "KDTree.hpp"
#include "Debug.hpp"
#include "Camera.hpp"
#include "MathUtils.hpp"

#include <random>

enum IntersectionType: Uint8{
	INTERSECT_INVALID = 0,

	INTERSECT_MISS = 1,
	INTERSECT_HIT_CHUNK_VOXEL = 2,
	INTERSECT_HIT_TRIANGLE_MESH = 3,
	INTERSECT_HIT_COLLIDER = 4,

	// When a ray is emitted inside of something
	INTERSECT_INTERNAL_CHUNK_VOXEL = 5, 
	INTERSECT_INTERNAL_COLLIDER = 6, 

	// The ray definitely hit a voxel, but the type is unknown. Used for
	// acceleration structures that couldn't store enough type data to
	// determine the hit type.
	INTERSECT_HIT_CHUNK_VOXEL_UNKNOWN_TYPE = 7,

	// When a ray may have hit something but follow-up is needed.
	INTERSECT_POSSIBLE_CHUNK_VOXEL = 8,
};

constexpr const char* INTERSECTION_TYPE_STRINGS[] = {
	"INTERSECT_INVALID",

	"INTERSECT_MISS",
	"INTERSECT_HIT_CHUNK_VOXEL",
	"INTERSECT_HIT_TRIANGLE_MESH",
	"INTERSECT_HIT_COLLIDER",

	"INTERSECT_INTERNAL_CHUNK_VOXEL",
	"INTERSECT_INTERNAL_COLLIDER",
	
	"INTERSECT_HIT_CHUNK_VOXEL_UNKNOWN_TYPE",
	"INTERSECT_POSSIBLE_CHUNK_VOXEL"
};

inline bool requiresLookup(IntersectionType type){
	return type == INTERSECT_POSSIBLE_CHUNK_VOXEL || type == INTERSECT_HIT_CHUNK_VOXEL_UNKNOWN_TYPE;
}

struct RayIntersection{
	IntersectionType type;
	float t_hit;  // Length of the ray to the hit
	union{
		// For hits with non-grid aligned geometry
		struct{
			FVec3 normal;
			Int32 triangle_index;  // Only defined for triangle meshes
		} unaligned_hit;

		// For hits with voxel geometry or voxel acceleration structures
		struct{
			IVec3 voxel;
			GridDirection face_index;
			Int16 palette_index;  // -1 if the type couldn't be determined.
		} voxel_hit;
	};
};

inline bool isValid(const RayIntersection& intersection){
	return 
		intersection.type == INTERSECT_HIT_CHUNK_VOXEL ||
		intersection.type == INTERSECT_HIT_TRIANGLE_MESH ||
		intersection.type == INTERSECT_HIT_COLLIDER;
}

inline FVec3 posFromT(Ray ray, float t){
	return ray.origin + ray.dir * t;
}

int smallestIndex(float x, float y, float z);
int smallestIndexBranchless(float x, float y, float z);

namespace Rendering{
	struct ImageConfig{
		// Specified in Width/Height order
		IVec2 num_pixels;
		IVec2 tile_dims;
	};

	struct ImageTile{
		/*
		A subsection of an image. Used for better cache coherency
		and multithreaded rendering.
		*/

		Range32 range_x;
		Range32 range_y;
	};

	class CameraRayGenerator{
		/*
		Packages data needed to generate camera rays behind a simple interface.
		WARNING: Assumes static camera position. Not valid in between renders.

		ATTRIBUTION: Genrating camera rays
			https://www.scratchapixel.com/lessons/3d-basic-rendering/
				ray-tracing-generating-camera-rays/generating-camera-rays
		*/

		public:
			CameraRayGenerator();
			CameraRayGenerator(Camera camera, IVec2 image_dims);
			Ray rayFromPixelCoord(IVec2 coord) const;

		private:
			Basis m_image_basis;
			IVec2 m_image_dims;
			FVec3 m_camera_pos;
			FVec3 m_w_prime;  // Precalculated term used to generate rays
	};

	std::vector<ImageTile> tiles(Camera camera, ImageConfig config);
	std::vector<Ray> allRays(Camera camera, ImageConfig config);
};

namespace Intersection{
	namespace Utils{
		struct GridRay{
			/*
			Package of info to avoid recalculating values
			when iterating rays through chunks. Gets passed to functions,
			which return an updated version of this struct. If a
			hit occurred, then the local_hit_index will be set. 
			*/

			// Fixed values
			FVec3 delta_t;
			IVec3 step_dir;

			// Updated values
			IVec3 local_grid_coord;
			FVec3 t_next_crossing;
			int last_stepped_axis;
			bool is_hit;
			VoxelType hit_voxel_type;
		};

		GridRay localChunkIntersection(GridRay grid_ray, const RawVoxelChunk& chunk);
		GridRay traverseEmptyChunk(GridRay grid_ray, Debug::DebugData& data);
	
		struct VKDTTraversalNode{
			/*
			NOTE: The size of this node should be a power of 2
			*/

			VoxelKDTree::NodeIndex curr_node_index;
			float t_min;
			float t_max;
			Axis axis;
		};
		static_assert(sizeof(VKDTTraversalNode) == 16);

		struct VKDTStack{
			/*
			Used to avoid re-allocations for every ray traced. Generated once
			in the tracing function and allocates a buffer 
			of height (max_tree_depth * 2), which can never overflow.
			*/

			VKDTTraversalNode* buffer_ptr;
			Int32 height;
			
			inline VKDTTraversalNode pop(){
				return buffer_ptr[--height];
			};

			inline void push(const VKDTTraversalNode& node){
				buffer_ptr[height++] = node;
			}

			static VKDTStack init(Int32 max_tree_depth){
				Int32 buffer_capacity = max_tree_depth * 2;
				//printf("Allocating stack with capacity %i\n", buffer_capacity);

				VKDTStack new_stack;
				new_stack.buffer_ptr = (VKDTTraversalNode*) malloc(
					buffer_capacity * sizeof(VKDTTraversalNode));
				new_stack.height = 0;
				return new_stack;
			}

			void freeMemory(){
				free(buffer_ptr);
			}
		};
	};

	RayIntersection intersectChunks(Ray ray, const ChunkTable* table_ptr);
	RayIntersection intersectTree(Ray ray, const VoxelKDTree::TreeData* tree);
	RayIntersection intersectTree(Ray ray, const VoxelKDTree::TreeData* tree, 
		Utils::VKDTStack stack);
	
	RayIntersection intersectTriangle(Ray ray, const Triangle& triangle);
	RayIntersection intersectTree(Ray ray, const MeshKDTree::TreeData* tree);

	RayIntersection intersectCollider(Ray ray, const ICuboid& cuboid);
	RayIntersection intersectCollider(Ray ray, const FSphere& sphere);
	RayIntersection intersectCollider(Ray ray, const FCuboid& cuboid);
	RayIntersection intersectCollider(Ray ray, const FCylinder& cylinder);
	RayIntersection intersectCollider(Ray ray, const FCone& cone);
	RayIntersection intersectCollider(Ray ray, const FRect& rectangle);
	RayIntersection intersectCollider(Ray ray, const FLine& line);
	RayIntersection intersectCollider(Ray ray, const FPlane& plane);

	struct DetailedCuboidIntersection{
		/*
		Returns both the enter and exit values of t.
		*/

		float t_bounds[2];
		bool is_valid;
		Axis last_min_axis;
	};

	DetailedCuboidIntersection intersectColliderDetailed(
		Ray ray, const ICuboid& cuboid);

	struct DetailedSphereIntersection{
		float t_bounds[2];
		bool is_valid;
	};

	DetailedSphereIntersection intersectColliderDetailed(
		Ray ray, const FSphere& sphere);
}
