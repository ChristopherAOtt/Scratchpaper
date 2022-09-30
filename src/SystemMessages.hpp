#pragma once

#include "Types.hpp"
#include "Primitives.hpp"

//-------------------------------------------------------------------------------------------------
// Shared structures and definitions
//-------------------------------------------------------------------------------------------------
enum SystemType{
	SYSTEM_INVALID = 0,

	SYSTEM_ENGINE,
	SYSTEM_PHYSICS,
	SYSTEM_ENTITIES,
	SYSTEM_CHUNK_MANAGER,
	SYSTEM_RENDERER
};

enum SystemEventType{
	LOADING_COMPLETE,
};


//-------------------------------------------------------------------------------------------------
// System Events
//-------------------------------------------------------------------------------------------------
struct SystemEvent{
	/*
	An event that happened in a specific system that may or may not require follow up. This is 
	useful for tracking 
	*/

	SystemType source_system;

	SystemEventType type;
	union{

	};
};


//-------------------------------------------------------------------------------------------------
// System Instructions
//-------------------------------------------------------------------------------------------------
//-----------------------------------------------
// Asset instruction
//-----------------------------------------------
enum AssetInstructionType{
	ASSET_INVALID = 0,

	LOAD_ASSET_BY_ID,
	UNLOAD_ASSET_BY_ID,

	LOAD_ASSET_BY_NAME,
	UNLOAD_ASSET_BY_NAME,
};

struct AssetInstruction{
	AssetInstructionType type;
	
	union{
		ResourceHandle handle;
		PODString name;	
	};
};


//-----------------------------------------------
// Chunk management instructions
//-----------------------------------------------
enum ChunkInstructionType{
	CHUNK_INVALID = 0,

	// Directed @ChunkManager->Deals with RawVoxelChunk structures
	CHUNK_GENERATE,
	CHUNK_ERASE,
	CHUNK_USE,
	CHUNK_LOAD_FROM_FILE,
	CHUNK_SAVE_TO_FILE,
	CHUNK_ERASE_FROM_FILE,

	// Directed @MeshGenerator->Deals with ChunkVoxelMesh structures
	CHUNK_GENERATE_UNLIT_MESH,
	CHUNK_ERASE_UNLIT_MESH,
	CHUNK_USE_UNLIT_MESH,
	CHUNK_LOAD_FROM_FILE_UNLIT_MESH,
	CHUNK_SAVE_TO_FILE_UNLIT_MESH,
	CHUNK_ERASE_FROM_FILE_UNLIT_MESH,

	// Directed @MeshGenerator->Deals with LightmappedChunkVoxelMesh structures
	CHUNK_GENERATE_LIGHTMAPPED_MESH,
	CHUNK_ERASE_LIGHTMAPPED_MESH,
	CHUNK_USE_LIGHTMAPPED_MESH,
	CHUNK_LOAD_FROM_FILE_LIGHTMAPPED_MESH,
	CHUNK_SAVE_TO_FILE_LIGHTMAPPED_MESH,
	CHUNK_ERASE_FROM_FILE_LIGHTMAPPED_MESH,
};

struct ChunkInstruction{
	ChunkInstructionType type;
	CellAddress lod_addr;
	Leniency leniency;
};

//-----------------------------------------------
// Text instruction
//-----------------------------------------------
struct TextInstruction{
	PODString text;
};


//-----------------------------------------------
//
//-----------------------------------------------
enum InstructionType{
	INSTRUCTION_ASSET,
	INSTRUCTION_CHUNK,
	INSTRUCTION_GENERAL_TEXT,
};

struct SystemInstruction{
	/*
	This is an order that should be followed up on. Basically a deferred function call.
	*/

	SystemType source_system;
	SystemType target_system;

	InstructionType type;
	union{
		AssetInstruction asset_instruction;
		ChunkInstruction chunk_instruction;
		TextInstruction text_instruction;
	};
};
