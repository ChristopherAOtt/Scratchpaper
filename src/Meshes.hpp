#pragma once

#include "Widgets.hpp"
#include "Chunks.hpp"
#include "Constants.hpp"

/*
This file defines general purpose meshes used in the program. Before rendering, these will be
converted to OpenGL/Vulkan/Whatever format by a module for that format. 
*/

//-------------------------------------------------------------------------------------------------
// Vertex Definitions for different mesh types
//-------------------------------------------------------------------------------------------------
struct VoxelMeshVertex{
	/*
	For base resolution voxel meshes
	*/

	FVec3 position;
	FVec2 uv;
	Uint32 type_index;
	Uint32 normal_index;  // (+X,-X,+Y,-Y,+Z,-Z). Six possible values.
};

struct LightmappedVoxelMeshVertex{
	/*
	Lit voxel meshes
	NOTE: I'm making VoxelMeshVertex lightmapped by default, so this has been made unnecessary
	*/

	FVec3 position;
	FVec2 uv;
	Uint16 type;
	Uint8 normal_index;
};

struct WidgetMeshVertex{
	FVec3 pos;
	FVec2 uv;
	FVec3 color;
};

//-------------------------------------------------------------------------------------------------
// Mesh Definitions
//-------------------------------------------------------------------------------------------------
enum MeshRenderType{  // TODO: Change to MeshFormat
	MESH_INVALID, 

	MESH_POINTS,
	MESH_LINES,
	MESH_TRIANGLES,
};

typedef Uint32 MeshIndex;
struct WidgetMesh{
	MeshRenderType type;
	std::vector<WidgetMeshVertex> vertices;
	std::vector<MeshIndex> indices;
};

struct EntityMesh{
	// TODO: Implement
};

struct TriangleMesh{
	/*
	NOTE: Triangles are supposed to be grouped by material to create long
	homogenous runs of the same material type. Not required, but it reduces
	the number of range structs per mesh and makes batched operations 
	easier.
	*/

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

struct ChunkVoxelMesh{
	CellAddress address;
	std::vector<VoxelMeshVertex> vertices;
	std::vector<MeshIndex> indices;
};

struct LightmappedChunkVoxelMesh{
	CellAddress address;
	std::vector<LightmappedVoxelMeshVertex> vertices;
	std::vector<MeshIndex> indices;
};

enum MeshType{
	MESHTYPE_INVALID = 0,

	MESHTYPE_WIDGET,
	MESHTYPE_ENTITY,
	MESHTYPE_CHUNK_VOXEL,
	MESHTYPE_LIGHTMAPPED_CHUNK_VOXEL
};
