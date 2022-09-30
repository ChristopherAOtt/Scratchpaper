#pragma once

#include "Primitives.hpp"
#include "Geometry.hpp"

#include <unordered_map>
#include <vector>
#include <mutex>

//--------------------------------------------------------------------------------------------------
// Constants
//--------------------------------------------------------------------------------------------------
constexpr int CHUNK_LEN = 32;
constexpr int CHUNK_AREA = CHUNK_LEN * CHUNK_LEN;
constexpr int CHUNK_VOLUME = CHUNK_LEN * CHUNK_LEN * CHUNK_LEN;

//--------------------------------------------------------------------------------------------------
// Helper Functions
//--------------------------------------------------------------------------------------------------
inline int linearChunkIndex(int x, int y, int z){
	return (x) + (y * CHUNK_LEN) + (z * CHUNK_AREA);
}

inline int linearChunkIndex(IVec3 local_coords){
	return (local_coords.x) + (local_coords.y * CHUNK_LEN) + (local_coords.z * CHUNK_AREA);
}

inline IVec3 localVoxelCoordFromGlobal(IVec3 global_coord){
	/*
	TODO: Find a more elegant formula that isn't as "clunky" as this.

	NOTE: This is to keep values wrapping consistently across the negative
	axis. For example, in a system with a repeat of 4 (instead of CHUNK_LEN)
	
	Global Coordinate: -10,-9,-8,-7,-6,-5,-4,-3,-2,-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, etc
	Wrapped Coordinate:  2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, etc
	*/

	IVec3 output;
	for(int i = 0; i < 3; ++i){
		output[i] = (global_coord[i] % CHUNK_LEN + CHUNK_LEN) % CHUNK_LEN;
	}
	return output;
}

inline IVec3 chunkCoordFromVoxelCoord(IVec3 coord){
	return {
		(int) floor(coord.x / (float) CHUNK_LEN),
		(int) floor(coord.y / (float) CHUNK_LEN),
		(int) floor(coord.z / (float) CHUNK_LEN),
	};
}

//--------------------------------------------------------------------------------------------------
// Aliases
//--------------------------------------------------------------------------------------------------
typedef IVec3 VoxelCoord;
typedef IVec3 ChunkCoord;

//--------------------------------------
// VoxelType
//--------------------------------------
constexpr int NUM_BLOCK_TYPES = 2 + 6;  // 2 special + additional types
enum VoxelType : Uint8{
	EMPTY  = 0,  // When merging chunks, do not overwrite the existing value.
	LOOKUP = 1,  // Special type. Need to do a lookup to determine this block's info.

	Air          = 2, // When merging chunks, overwrite the existing value with air.
	Grass        = 3,
	Dirt         = 4,
	Stone        = 5,
	Concrete     = 6,
	Metal        = 7,

	LightEmitter = 128,
};

inline bool isAir(VoxelType type){
	return type == VoxelType::EMPTY || type == VoxelType::Air;
}

constexpr FVec3 VOXEL_COLOR_BY_TYPE[] = {
	// These are bright red because if you see them, something went wrong.
	[EMPTY]        = {1.0, 0.0, 0.0},
	[LOOKUP]       = {1.0, 0.0, 0.0},
	[Air]          = {1.0, 0.0, 0.0},

	// These are vaaaaaguely supposed to resemble their materials
	[Grass]        = {0.3608, 0.9529, 0.3608},
	[Dirt]         = {0.5294, 0.2431, 0.1373},
	[Stone]        = {0.4824, 0.4784, 0.4745},
	[Concrete]     = {0.7804, 0.7804, 0.7804},
	[Metal]        = {0.2784, 0.2863, 0.3201},
	
	// error: expected identifier before numeric constant
	// [8 ... 127] = {1.0, 0.0, 0.0},

	// sOrRy, UnImPlEmEnTeD: nOn-TrIvIaL dEsIgNaTeD iNiTiAlIzErS nOt SuPpOrTeD
	// [LightEmitter] = {0.5000, 1.0000, 0.8000};
};
constexpr FVec3 LIGHT_EMITTING_VOXEL_COLOR = {0.5000, 1.0000, 0.8000};

//--------------------------------------
// Voxel
//--------------------------------------
struct Voxel{
	VoxelType type;
};
static_assert(sizeof(Voxel) == 1, "Voxel must be 1 byte");

//--------------------------------------
// RawVoxelChunk
//--------------------------------------
struct RawVoxelChunk{
	VoxelType voxelTypeAt(VoxelCoord coord) const;

	Voxel data[CHUNK_VOLUME];
};
RawVoxelChunk initVoxelChunk();
void printChunkStats(const RawVoxelChunk& chunk);

//--------------------------------------------------------------------------------------------------
// Chunk Table
//--------------------------------------------------------------------------------------------------
class ChunkTable{
	/*
	Abstracts chunk data management
	TODO: Max loaded chunk count
	YAGNI: Better data packing. Use fixed-size memory block with freelist allocator.
	*/
	public:
		ChunkTable();
		~ChunkTable();

		bool isLoaded(IVec3 coord) const;
		void setChunk(IVec3 coord, RawVoxelChunk chunk_data);
		RawVoxelChunk& getChunkRef(IVec3 coord);
		const RawVoxelChunk* const getChunkPtr(IVec3 coord) const;
		void eraseChunks(std::vector<IVec3> coords);
		std::vector<IVec3> allLoadedChunks() const;
		ICuboid boundingVolumeChunkspace() const;

	public:
		std::mutex m_access_mutex;

	private:
		ICuboid m_bounds;
		std::unordered_map<IVec3, RawVoxelChunk, PODHasher> m_loaded_chunks;

};

static_assert(sizeof(Voxel) * CHUNK_VOLUME == sizeof(RawVoxelChunk), 
	"RawVoxelChunk struct must be a dense array of Voxel structs");
