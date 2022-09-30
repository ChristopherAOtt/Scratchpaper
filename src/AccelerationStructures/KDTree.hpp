#pragma once

#include "Primitives.hpp"
#include "Geometry.hpp"
#include "Types.hpp"
#include "Constants.hpp"
#include "Chunks.hpp"
#include "FileIO.hpp"
#include "Meshes.hpp"

#include <vector>
#include <stack>

//-----------------------------------------------------------------------------
// VoxelKDTree
// TODO: Refactor and package tree functions into a class
//-----------------------------------------------------------------------------
namespace VoxelKDTree{
	/*
	REFACTOR: Encapsule structure and remove cruft once testing is done.
	
	NOTE: The plane offset is the index of the first voxel in the far
		node relative to the origin. The near child is 0 and the far
		child is 1. In a simplified situation where the offset only has 4 bits,
		the near node is labelled N and the far node is labelled F.
		
		Scenario A       
		Offset = 0101 = 5
		┏━━━━━━━━━┳━━━━━━━━━━━━━━━━━━━━━┓
		┃0 1 2 3 4┃5 6 7 8 9 a b c d e f┃
		┃N        ┃F                    ┃
		┗━━━━━━━━━┻━━━━━━━━━━━━━━━━━━━━━┛

		Scenario B
		Offset = 1111 = 15 (Maximum possible offset for 4 bit example)
		┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┳━┓
		┃0 1 2 3 4 5 6 7 8 9 a b c d e┃f┃
		┃N                            ┃F┃
		┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┻━┛
		
		Scenario C
		Offset = 0001 = 1 (Minimum possible offset)
		┏━┳━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
		┃0┃1 2 3 4 5 6 7 8 9 a b c d e f┃
		┃N┃F                            ┃
		┗━┻━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛

	This means that the offset cannot take on a value that results in a 
	zero-volume node. Offset must be >= 1, and <= the number of valid offsets
	minus one. These are defined in MIN_VALID_PLANE_OFFSET, 
	MAX_VALID_PLANE_OFFSET, and INVALID_PLANE_OFFSET.
	*/

	typedef Bytes2 PackedData;
	typedef Int32 NodeIndex;
	
	// How many bits of different properties
	static constexpr int NUM_PACKED_DATA_BITS = 16;
	static constexpr int NUM_PLANE_OFFSET_BITS = 14;
	static constexpr int NUM_NODE_TYPE_BITS = 2;
	static constexpr int NUM_CHILDREN_PER_SPLIT = 2;

	// Masks/flags used to manipulate bits
	static constexpr Bytes2 MASK_NODE_TYPE_BITS       = 0b0000'0000'0000'0011;
	static constexpr Bytes2 MASK_PLANE_OFFSET_BITS    = 0b1111'1111'1111'1100;
	static constexpr Bytes2 FLAG_LEAF_IS_EMPTY        = 0b0000'0000'0000'0100;
	static constexpr Bytes2 FLAG_LEAF_IS_MIXED_TYPE   = 0b0000'0000'0000'1000;
	static constexpr Bytes2 MASK_LEAF_DESCRIPTION     = 0b0000'0000'0000'1111;
	static constexpr Bytes2 MASK_LEAF_PERCENT_SOLID   = 0b0111'1111'0000'0000;
	static constexpr Bytes2 MASK_LEAF_PALETTE_INDEX   = 0b1111'1111'0000'0000;

	// How much to right-shift bits by before extracting their values
	static constexpr int SHIFT_PLANE_OFFSET = 2;
	static constexpr int SHIFT_LEAF_PERCENT_FULL = 8;
	static constexpr int SHIFT_LEAF_PALETTE_INDEX = 8;

	// Values, defined for easier visualization of their layout. 
	static constexpr Bytes2 VALUE_UNSET_NODE          = 0b0000'0000'0000'0000;
	static constexpr Bytes2 VALUE_EMPTY_LEAF          = 0b0000'0000'0000'0111;
	static constexpr Bytes2 VALUE_SOLID_MIXED_LEAF    = 0b0110'0100'0000'1011;

	// These are the result of a mask being applied. For example, 
	// MASK_LEAF_DESCRIPTION applied to a homogenous leaf will 
	// equal VALUE_HOMOGENOUS_LEAF. A node with a Y plane will equal
	// VALUE_PLANE_Y after MASK_NODE_TYPE_BITS is applied.
	// TODO: Change VALUE to RESULT to make this clearer
	static constexpr Bytes2 VALUE_PLANE_X             = 0b0000'0000'0000'0000;
	static constexpr Bytes2 VALUE_PLANE_Y             = 0b0000'0000'0000'0001;
	static constexpr Bytes2 VALUE_PLANE_Z             = 0b0000'0000'0000'0010;
	static constexpr Bytes2 VALUE_LEAF_NODE           = 0b0000'0000'0000'0011;
	static constexpr Bytes2 VALUE_HOMOGENOUS_LEAF     = 0b0000'0000'0000'0011;
	static constexpr Bytes2 NODE_TYPES_ARR[4] = {
		VALUE_PLANE_X, 
		VALUE_PLANE_Y, 
		VALUE_PLANE_Z,
		VALUE_LEAF_NODE
	};

	// Misc constants
	static constexpr NodeIndex INVALID_NODE_INDEX = -1;
	static constexpr int INVALID_PLANE_OFFSET = 0;
	static constexpr int MIN_VALID_PLANE_OFFSET = 1;
	static constexpr int MAX_VALID_PLANE_OFFSET = (1<<NUM_PLANE_OFFSET_BITS)-1;

	enum NodeType: Uint8{
		NODE_PLANE_X = VALUE_PLANE_X,   // = 0
		NODE_PLANE_Y = VALUE_PLANE_Y,   // = 1
		NODE_PLANE_Z = VALUE_PLANE_Z,   // = 2
		NODE_LEAF    = VALUE_LEAF_NODE, // = 3
	};

	struct GeometryNode{
		PackedData pack;
	};

	inline NodeType nodeType(PackedData data){
		return (NodeType)(data & MASK_NODE_TYPE_BITS);
	}

	inline Uint16 planeOffset(PackedData data){
		return (data & MASK_PLANE_OFFSET_BITS) >> SHIFT_PLANE_OFFSET;
	}

	inline Uint8 paletteIndex(PackedData data){
		return (data & MASK_LEAF_PALETTE_INDEX) >> SHIFT_LEAF_PALETTE_INDEX;
	}

	inline bool isHomogenousLeaf(PackedData data){
		return (data & MASK_LEAF_DESCRIPTION) == VALUE_HOMOGENOUS_LEAF;
	}

	struct PropertyNode{
		/*
		Optional node type. Allows traversal to terminate at any point
		in the tree, not just leaf nodes. Information about the subtree
		can then be retrieved from these nodes for various purposes.
		*/

		Uint8 density_percentage;
	};

	struct DescendantNode{
		NodeIndex left_child_index{INVALID_NODE_INDEX};
	};

	struct SplitPlane{
		Axis axis;
		Int16 offset;  // From tree origin, NOT the node origin.
	};

	struct BuildSettings{
		/*
		Settings for the KDTree construction
		*/

		// Higher numbers mean more optimized, but slower tree
		// construction.
		OptimizationLevel optimization_level{OPTIMIZE_EXHAUSTIVE};

		// Tree cannot be deeper than this. All contents at this
		// level will be forced into a leaf node regardless of type.
		int max_depth{16};

		// If true, the memory for the max_depth number of nodes will be
		// allocated all at once on construction. Not recommended outside of
		// testing.
		bool should_preallocate_max_node_space{false};

		// If the volume of a split is <= this value, the node
		// is forced to be a leaf.
		int mandatory_leaf_volume{8};

		// If true, nodes will appear tightly packed in the array regardless
		// of parent index. If false, they will follow the 2n+1 rule. Sparsity
		// makes it possible to test runtime tree resizing without needing to
		// reallocate subtrees whenever a node is expanded.
		bool should_pack_nodes{true};

		// Calculate property info for all nodes, not just leaves. This
		// makes partial tree traversals possible at the cost of extra memory.
		bool should_calculate_non_leaf_properties{false};

		// If true, the tree builder tries to make splits that produce 
		// homogenous leaves if the situation allows. 
		// Doesn't guarantee that will happen (Hitting max depth/Min Volume)
		bool should_differentiate_types{false};

		ICuboid bounds;
	};

	struct TreeData{
		ICuboid bounds{{0,0,0},{0,0,0}};

		Uint64 node_capacity{0};
		Uint64 node_count{0};
		Int32 curr_max_depth{0};
		
		bool is_packed_tree{true};
		bool has_property_nodes{false};
		GeometryNode*   geometry_nodes_ptr{NULL};
		PropertyNode*   property_nodes_ptr{NULL};
		DescendantNode* descendant_nodes_ptr{NULL};
	};

	Int64 writeToBuffer(const TreeData* tree);
	void debuggingPrintTreeContents(const TreeData* tree);
	void debuggingPrintTreeStats(const TreeData* tree);
	void debuggingPrintPackedData(VoxelKDTree::PackedData data);

	namespace TreeBuilder{
		//---------------------------------------
		// Structure Definitions
		//---------------------------------------
		struct SplitReferenceData{
			/*
			Provided to node splitting function 
			*/

			BuildSettings* settings;
			const ChunkTable* chunk_table;

			const TreeData* tree_data;
			NodeIndex node_to_split;
			ICuboid node_cuboid;
			int node_depth;
		};

		struct SplitResult{
			/*
			Returned from split function
			*/

			// This will need to be incorporated into the parent node
			SplitPlane split;
			ICuboid cuboids[2];

			// Un-split internal tree nodes need to be updated once they
			// are split later. If the node is a leaf then all the
			// information necessary to finalize it is already present.
			// This means said nodes can be copy-pasted into the final
			// node structure.
			bool is_leaf_node[2];
			GeometryNode children[2];

			// NOTE: This is only filled if the build setting 
			// "should_calculate_non_leaf_properties" is
			// set to True. Otherwise this will be undefined.
			bool is_properties_defined[2];
			PropertyNode child_properties[2];
		};

		struct CuboidSplit{
			ICuboid results[2];
		};

		struct AxisSummary{
			/*
			NOTE: I'm planning on making it possible to have slices of fixed 
			thickness for large volumes. For example, on a 1024 cubed volume,
			each slice might have a thickness of 64 units so that there are 
			only 16 slices in the axis summary. That's a longwinded way of 
			explaining why some of these are Int64 instead of Int32. The 
			number of solids in these slices could be greater than a max Int32.
			*/

			Axis axis;
			Int16 offset_from_origin;
			Int64 plane_perimeter;
			Int64 plane_area;
			Int64 total_solids;
			std::vector<Int64> num_solid_in_plane;
			std::vector<VoxelType> type_if_homogenous;
		};
		void debuggingPrintAxisSummary(const AxisSummary& summary);

		class VoxelLookup{
			/*
			Abstracts lookup operations to the table.
			YAGNI: Cache recently referenced chunks to accelerate lookups
			*/
			static constexpr Int32 CACHE_CAPACTIY = 16;

			public:
				VoxelLookup(const ChunkTable* table);
				Voxel voxelAtCoord(IVec3 coord);

			private:
				// World's crappiest lru cache
				Int32 m_num_cached_chunks;
				IVec3 m_cached_chunk_coords[CACHE_CAPACTIY];
				Uint8 m_num_misses_since_last_refresh[CACHE_CAPACTIY];
				RawVoxelChunk m_cached_chunks[CACHE_CAPACTIY];

				const ChunkTable* m_table_ptr;
		};

		struct SplitRecommendation{
			/*
			Returned by the split picker functions
			*/

			SplitPlane plane;
			float split_score;  // Higher for better results
		};

		//---------------------------------------
		// Functions
		//---------------------------------------
		// Private helper functions
		AxisSummary generateSummary(const ChunkTable* table, 
			IVec3 volume_origin_offset, ICuboid volume, Axis axis);
		SplitResult generateSplit(const SplitReferenceData& ref);
		CuboidSplit splitCuboid(ICuboid cuboid, SplitPlane split);
		std::array<PackedData, NUM_CHILDREN_PER_SPLIT> leafStates(
			const AxisSummary& summary, SplitPlane split);

		// Axis pickers
		Axis axisPickerStupid(const SplitReferenceData& ref);
		Axis axisPickerLongest(const SplitReferenceData& ref);

		// Offset pickers
		SplitRecommendation axisPickerBlindCenter(
			const SplitReferenceData& ref, const AxisSummary& summary);
		SplitRecommendation offsetPickerLongestRunBias(
			const SplitReferenceData& ref, const AxisSummary& summary);

		// Public interface. 
		// TODO: Move to the VoxelKDTree namespace when done
		TreeData* buildTree(const ChunkTable& table, BuildSettings settings);
	};

	// Make Private
	bool resizeToCapacity(TreeData& tree, Int32 capacity);
	void freeTreeData(TreeData*& data);
	TreeData* loadTreeFromFile(std::string filepath, bool is_packed=true);
};

namespace MeshKDTree{
	/*
	This has been slapped together even more sloppily than the VKDTree
	just for the sake of getting something working. 

	TODO: Get the code style between these two different tree types
		more consistent with each other.
	*/

	typedef Int32 NodeIndex;

	struct TriangleSurfaceData{
		/*
		Normal and UV info only becomes relevant after a hit has been
		cofirmed. This packs the data into a parallel array.
		*/

		FVec2 uvs[3];
		FVec3 normals[3];
	};

	struct SplitNode{
		/*
		Internal tree node. If the axis index is 4 the child
		is a leaf.
		*/

		Uint8 axis_index;
		float plane_offset;
		NodeIndex left_child;
	};
	
	struct LeafNode{
		/*
		All triangles are allocated in one big linear
		vector. This indexes into that vector.
		*/

		Range32 triangle_range;
	};

	struct TreeData{
		/*

		*/

		FCuboid tree_bounds{{0,0,0},{0,0,0}};

		Uint64 num_triangles{0};
		Uint64 node_capacity{0};
		Uint64 node_count{0};

		std::vector<SplitNode> split_nodes;
		std::vector<LeafNode> leaf_nodes;

		// These two are a giant linear buffer of triangle data. There
		// will be duplicates in here, since leaf nodes need to be able 
		// to index into here in a contiguous range.
		std::vector<Triangle> intersect_data;
		std::vector<TriangleSurfaceData> surface_data; 
	};

	class MKDTree{
		public:
			MKDTree(TreeData data);
			~MKDTree();

		private:
			TreeData m_data;
	};

	TreeData buildTree(const TriangleMesh& mesh);
};
