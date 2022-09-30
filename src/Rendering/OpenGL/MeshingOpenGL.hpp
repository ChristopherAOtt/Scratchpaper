#pragma once

#include <vector>

#include "Primitives.hpp"
#include "Meshes.hpp"

//-----------------------------------------------
// Vertex description definitions
//-----------------------------------------------
enum MemberBaseType{
	TYPE_FLOAT, 
	TYPE_UINT
};

struct VertexMemberDescription{
	/*
	Different vertex types will have different member data. This describes
	each element in turn. 
	FVec3 = {base_type=FLOAT, num_per_member=3}
	Vec2 = {base_type=FLOAT, num_per_member=2}
	uint = {base_type=UINT, num_per_member=1}
	VoxelPos = {base_type=INT, num_per_member=3}
	*/

	// Basic datatype that makes this member up. 
	MemberBaseType base_type;

	// Numer of base type making up this instance. 
	// FVec3 = 3 floats
	// IVec2 = 2 ints
	int num_per_member;
};

constexpr int MAX_MEMBERS_PER_VERTEX = 4;
struct VertexDescription{
	VertexMemberDescription member_descriptions[MAX_MEMBERS_PER_VERTEX];
	int num_described_members;
	int vertex_total_size;
};

VertexDescription vertexDescriptionVoxelVertex();
VertexDescription vertexDescriptionVoxelLightingVertex();
VertexDescription vertexDescriptionWidgetVertex();

std::vector<Bytes1> toByteVector(void* arr_base_ptr, int num_bytes);


//-------------------------------------------------------------------------------------------------
// Mesh-related structs and functions
//-------------------------------------------------------------------------------------------------
struct UniversalMesh{
	/*
	Used for all meshes about to be sent to the GPU. 
	Not used at the current moment due to unresolved issues.
		TODO: Handle meshes with multiple render types (line + triangle)
		TODO: Associate meshes with shaders, since some shaders will assume specific vertex formats
	*/

	VertexDescription vertex_description;
	std::vector<Bytes1> vertex_bytes;
	std::vector<Uint32> indices;
	
	int num_vertices;
	int vertex_size;
};

// NOTE: For now there's no difference between the "common type" meshes and the
// format used by OGL. This is just a placeholder.
typedef WidgetMesh WidgetMeshOpengl;
typedef ChunkVoxelMesh ChunkVoxelMeshOpengl;

struct RigidTriangleMeshVertex{
	FVec3 pos;
	FVec2 uv;
	FVec3 normal;
};

struct RigidTriangleMeshOpengl{
	std::vector<RigidTriangleMeshVertex> vertices;
	std::vector<Uint32> indices;

	std::unordered_map<std::string, Range32> material_ranges;
};

namespace MesherOpenGL{
	WidgetMeshOpengl toOpenglFormat(const WidgetMesh& mesh);
	ChunkVoxelMeshOpengl toOpenglFormat(const ChunkVoxelMesh& mesh);
	RigidTriangleMeshOpengl toOpenglFormat(const TriangleMesh& mesh);
};

