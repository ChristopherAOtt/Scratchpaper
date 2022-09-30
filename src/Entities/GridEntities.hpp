#pragma once

#include "Primitives.hpp"
#include "Geometry.hpp"

#include <unordered_map>
#include <array>

struct SimpleCollider{
	// TODO: Add geometric info here.
};

//--------------------------------------------------------------------------------------------------
// Entity Data
//--------------------------------------------------------------------------------------------------
enum GridEntityType : unsigned char{
	GENT_INVALID = 0,
	GENT_OTHER = 1,
	GENT_ITEM_PIPE = 2,
	GENT_FLUID_PIPE = 3,
	GENT_ASSEMBLER = 4,
	GENT_STORAGE_BLOCK = 5
};
constexpr static int NUM_ENTITY_TYPES = 5;

struct GEItemPipe{

};

struct GEFluidPipe{

};

struct GEAssembler{

};

struct GEStorageBlock{

};

struct GridEntityData{
	GridEntityType type;
	union{
		GEItemPipe item_pipe;
		GEFluidPipe fluid_pipe;
		GEAssembler assembler;
		GEStorageBlock storage_block;
	};
};

//--------------------------------------------------------------------------------------------------
// Footprint
//--------------------------------------------------------------------------------------------------
enum FootprintType{
	FPRINT_INVALID = 0,
	FPRINT_GRID_BOUNDS,
	FPRINT_COLLIDER_LIST,
	FPRINT_ARBITRARY_MESH
};

constexpr int MAX_COLLIDERS = 8;
struct ColliderList{
	int num_colliders;
	SimpleCollider colliders[MAX_COLLIDERS];
};

struct MeshInfo{
	int mesh_id;
};

struct GridEntityFootprint{
	FootprintType type;
	union{
		ColliderList colliders;
		MeshInfo mesh_info;
	};
};


//--------------------------------------------------------------------------------------------------
// Rendering
//--------------------------------------------------------------------------------------------------
enum GridEntityRenderType{
	GENTRENDER_INVALID = 0,
	GENTRENDER_TEXTURED_AABB,
	GENTRENDER_MESH_FIXED,
	GENTRENDER_MESH_SEGMENTED
};

struct GridEntityRendering{
	// TODO: Fill in fields with TexturedAABB, Mesh (Fixed size + patches)
	GridEntityRenderType type;
};

struct GridEntityRecord{
	GridEntityData data;
	IVec3 occupied_voxels;
};


//--------------------------------------------------------------------------------------------------
// Grid Entity Table
// Abstracts chunk data management
// TODO: Better memory layout. Get entity data into tightly packed arrays with parallel arrays for
//		less frequently accessed data.
//--------------------------------------------------------------------------------------------------
struct SharedEntityInfo{
	/*
	This info is shared by all entities of a given type.
	*/
	GridEntityFootprint footprint;
	GridEntityRendering rendering;
};

// NOTE: The EntityId of 0 is reserved to indicate an invalid id.
constexpr static int INVALID_ENTITY_ID_OLD = 0;
typedef Bytes8 EntityIdOld;

class GridEntityTable{
	public:
		GridEntityTable();
		~GridEntityTable();

		EntityIdOld addEntity(GridEntityData data, IVec3 occupied_voxels);
		bool deleteEntity(EntityIdOld entity_id);

	private:
		void initSharedEntityInfoTable();

	private:
		EntityIdOld m_next_available_id;
		std::array<SharedEntityInfo, NUM_ENTITY_TYPES> m_shared_info;
		std::unordered_map<EntityIdOld, GridEntityRecord> m_records;
};
