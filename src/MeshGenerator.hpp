#pragma once

#include "Primitives.hpp"
#include "Chunks.hpp"
#include "Debugging.hpp"
#include "Meshes.hpp"

//-------------------------------------------------------------------------------------------------
// Helper functionality
//-------------------------------------------------------------------------------------------------
// Flags for accessing specific bits. Defined for clarity and in case bitfields produce bad code.
constexpr Bytes1 FLAG_X_POS          = 0b00000001;
constexpr Bytes1 FLAG_X_NEG          = 0b00000010;
constexpr Bytes1 FLAG_Y_POS          = 0b00000100;
constexpr Bytes1 FLAG_Y_NEG          = 0b00001000;
constexpr Bytes1 FLAG_Z_POS          = 0b00010000;
constexpr Bytes1 FLAG_Z_NEG          = 0b00100000;
constexpr Bytes1 FLAG_X_AXIS         = 0b00000011;
constexpr Bytes1 FLAG_Y_AXIS         = 0b00001100;
constexpr Bytes1 FLAG_Z_AXIS         = 0b00110000;
constexpr Bytes1 FLAG_ANY_VALUES_SET = 0b00111111;
constexpr Bytes1 FLAG_CLEAR          = 0b00000000;
constexpr Bytes1 FLAGS_IN_ORDER_BY_FACE[NUM_FACES_PER_VOXEL] = {
	FLAG_X_POS, FLAG_X_NEG,
	FLAG_Y_POS, FLAG_Y_NEG,
	FLAG_Z_POS, FLAG_Z_NEG,
};
constexpr Int32 SIGNED_AXIS_OFFSETS[2] = {1, -1};

struct FaceEmitFlags{
	/*
	A 1-byte structure with bits indicating whether a face should be emitted in the given
	direction. pos = positive, neg = negative.

	NOTE: unused_padding is needed to account for all the bits.
	*/
	union{
		// Address specific directions in the positive/negative axis
		struct{
			Bytes1 x_pos: 1;
			Bytes1 x_neg: 1;
			Bytes1 y_pos: 1;
			Bytes1 y_neg: 1;
			Bytes1 z_pos: 1;
			Bytes1 z_neg: 1;

			Bytes1 unused_padding: 2;
		} directional;

		// Address just the axis
		struct{
			Bytes1 x_axis: 2;
			Bytes1 y_axis: 2;
			Bytes1 z_axis: 2;

			Bytes1 unused_padding: 2;
		} axis;

		// Address all defined bits
		struct{
			Bytes1 values: 6;

			Bytes1 unused_padding: 2;
		} defined_values;

		// Address all bits
		Bytes1 all_bits;
	};
};


constexpr Int32 NUM_EDGES_PER_CUBE = 12;
constexpr Int32 NUM_TRIANGLES_PER_RECTANGLE = 2;
constexpr Int32 NUM_VERTICES_PER_FACE = 4;
constexpr Int32 NUM_VERTICES_PER_VOXEL = 8;
constexpr Int32 NUM_INDICES_PER_FACE = NUM_VERTICES_PER_TRIANGLE * NUM_TRIANGLES_PER_RECTANGLE;
constexpr Int32 VERT_INDICES[NUM_FACES_PER_VOXEL][NUM_VERTICES_PER_FACE] = {
	{5, 7, 1, 3},  // +X
	{6, 4, 2, 0},  // -X
	{7, 6, 3, 2},  // +Y
	{4, 5, 0, 1},  // -Y
	{6, 7, 4, 5},  // +Z
	{3, 2, 1, 0},  // -Z
};

constexpr FVec3 CUBE_VERTEX_POSITIONS[NUM_VERTICES_PER_VOXEL] = {
	{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {1, 1, 0}, // Bottom verts (0 - 3)
	{0, 0, 1}, {1, 0, 1}, {0, 1, 1}, {1, 1, 1}, // Top verts (4 - 7)
};

constexpr Int32 FACE_INDICES[NUM_INDICES_PER_FACE] = {
	2, 1, 0,  // First triangle
	2, 3, 1,  // Second triangle
};

constexpr FVec3 DIRECTION_COLORS[NUM_FACES_PER_VOXEL] = {
	{1.0, 0.0, 0.0}, {0.5, 0.0, 0.0},  // +X, -X
	{0.0, 1.0, 0.0}, {0.0, 0.5, 0.0},  // +Y, -Y
	{0.0, 0.0, 1.0}, {0.0, 0.0, 0.5},  // +Z, -Z
};

// Dummy values to act as placeholders 
constexpr FVec3 DUMMY_POS = {0, 0, 0};
constexpr FVec2 DUMMY_UV = {0, 0};
constexpr FVec3 DUMMY_NORMAL = {0, 0, 1};

struct MeshFace{
	FVec3 vertex_positions[NUM_VERTICES_PER_FACE];
};

struct AdjacencyNode{
	VoxelType voxel_type;
	FaceEmitFlags flags;
};

struct MesherAdjacencyGrid{
	/*
	A cube of adjacency nodes for each voxel in a chunk.
	*/
	AdjacencyNode nodes[CHUNK_VOLUME];
	Int32 num_total_faces;
};


static_assert(std::has_unique_object_representations<FaceEmitFlags>::value, "Not compact!");
//static_assert((sizeof(FVec3) * 2 + sizeof(FVec2)) == sizeof(MeshVertex), "Must be compact!");
static_assert(sizeof(FaceEmitFlags) == 1);
static_assert(sizeof(AdjacencyNode) == 2);
static_assert((sizeof(VoxelType) + sizeof(FaceEmitFlags)) == sizeof(AdjacencyNode), 
	"Must be compact!");

MesherAdjacencyGrid generateAdjacencyGrid(const ChunkTable* table, CellAddress addr);
MesherAdjacencyGrid generateAdjacencyGrid(const RawVoxelChunk& chunk_data);

ChunkVoxelMesh generateChunkMesh(const MesherAdjacencyGrid& adjacency, CellAddress addr);
ChunkVoxelMesh generateGreedyChunkMesh(const MesherAdjacencyGrid& adjacency);
LightmappedChunkVoxelMesh generateLightmappedChunkMesh(const MesherAdjacencyGrid& adjacency);

WidgetMesh generateAdjacencyGridWidget(const MesherAdjacencyGrid& grid);
WidgetMesh generateWidgetMesh(const std::vector<Widget>& widgets);


//-------------------------------------------------------------------------------------------------
// Meshing
//-------------------------------------------------------------------------------------------------
