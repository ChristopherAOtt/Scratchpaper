#include "Chunks.hpp"

//------------------------------------------------------------------------------
// Helper functions
//------------------------------------------------------------------------------
ICuboid expandIfNecessary(ICuboid existing_bounds, IVec3 coord){
	/*
	Given a bounding box, expand it if the given coordinate is outside of those bounds.

	REFACTOR: Figure out a way to reduce code duplication between this function and 
		chunkspaceLoadedWorldBounds() that isn't just calling this in a loop.
	*/

	ICuboid output;

	IVec3 bounds[2] = {
		existing_bounds.origin,
		existing_bounds.origin + existing_bounds.extent
	};
	for(int i = 0; i < NUM_3D_AXES; ++i){
		bounds[INDEX_VALUE_MIN][i] = std::min(bounds[INDEX_VALUE_MIN][i], coord[i]);
		bounds[INDEX_VALUE_MAX][i] = std::max(bounds[INDEX_VALUE_MAX][i], coord[i] + 1);
	}
	output.origin = bounds[INDEX_VALUE_MIN];
	output.extent = bounds[INDEX_VALUE_MAX] - bounds[INDEX_VALUE_MIN];

	return output;
}

ICuboid chunkspaceLoadedWorldBounds(const std::vector<IVec3>& coords){
	/*
	Returns a bounding box around all the given coords.
	*/

	ICuboid output = {
		.origin={0, 0, 0},
		.extent={0, 0, 0},
	};

	for(IVec3 coord : coords){
		output = expandIfNecessary(output, coord);
	}

	return output;
}

bool isFullyContained(ICuboid volume, IVec3 coord){
	for(int i = 0; i < NUM_3D_AXES; ++i){
		int max_volume = volume.origin[i] + volume.extent[i];
		if(coord[i] <= volume.origin[i] || coord[i] >= max_volume - 1){
			return false;
		}
	}
	return true;
}


//------------------------------------------------------------------------------
// Chunk-Related POD Structs
//------------------------------------------------------------------------------
//--------------------------------------
// VoxelType
// NO CURRENT IMPLEMENTATION
//--------------------------------------
//--------------------------------------
// Block
// NO CURRENT IMPLEMENTATION
//--------------------------------------

//--------------------------------------
// RawVoxelChunk
//--------------------------------------
VoxelType RawVoxelChunk::voxelTypeAt(VoxelCoord coord) const{
	int flat_index = linearChunkIndex(coord.x, coord.y, coord.z);
	return data[flat_index].type;
}

RawVoxelChunk initVoxelChunk(){
	RawVoxelChunk output;
	memset(&output, 0, sizeof(RawVoxelChunk));
	return output;
}

void printChunkStats(const RawVoxelChunk& chunk){
	/*
	Prints count of different block types
	*/

	int block_counts[NUM_BLOCK_TYPES] = {0, 0, 0, 0, 0, 0, 0, 0};
	const char* BLOCK_NAMES[] = {
		"EMPTY", "LOOKUP", "Air", "Grass", "Dirt", "Stone", "Concrete", "Metal"
	};

	for(int i = 0; i < CHUNK_VOLUME; ++i){
		block_counts[chunk.data[i].type] += 1;
	}

	printf("Block count summary:\n");
	for(int i = 0; i < NUM_BLOCK_TYPES; ++i){
		if(block_counts[i] > 0){
			printf("\t%s: %i\n", BLOCK_NAMES[i], block_counts[i]);
		}
	}
}


//--------------------------------------------------------------------------------------------------
// Chunk Table
//--------------------------------------------------------------------------------------------------
ChunkTable::ChunkTable(){
	m_bounds = {
		.origin={0,0,0}, 
		.extent={0,0,0}
	};
}

ChunkTable::~ChunkTable(){

}


bool ChunkTable::isLoaded(IVec3 coord) const{
	return m_loaded_chunks.find(coord) != m_loaded_chunks.end();
}

void ChunkTable::setChunk(IVec3 coord, RawVoxelChunk chunk_data){
	/*
	YAGNI: Do this against the current bounds instead of stupidly
	*/

	m_loaded_chunks[coord] = chunk_data;
	m_bounds = expandIfNecessary(m_bounds, coord);
}

RawVoxelChunk& ChunkTable::getChunkRef(IVec3 coord){
	assert(m_loaded_chunks.find(coord) != m_loaded_chunks.end());
	return m_loaded_chunks[coord];
}

const RawVoxelChunk* const ChunkTable::getChunkPtr(IVec3 coord) const{
	auto iter = m_loaded_chunks.find(coord);
	if(iter != m_loaded_chunks.end()){
		return &(iter->second);
	}else{
		return NULL;
	}
}

void ChunkTable::eraseChunks(std::vector<IVec3> coords){
	/*
	YAGNI: Find a smarter way to recalculate the boundary than just
	scanning the whole table with every deletion. Perhaps batch the
	chunks into edge regions and only trigger the resize if they're
	found there?
	*/

	bool is_resize_avoidable = true;
	for(IVec3 coord : coords){
		is_resize_avoidable &= isFullyContained(m_bounds, coord);
		m_loaded_chunks.erase(coord);	
	}

	if(!is_resize_avoidable){
		m_bounds = chunkspaceLoadedWorldBounds(allLoadedChunks());
	}
}

std::vector<IVec3> ChunkTable::allLoadedChunks() const{
	/*
	Returns a vector containing the addresses of all loaded chunks
	*/

	std::vector<IVec3> loaded_chunks;
	for(auto iter = m_loaded_chunks.begin(); iter != m_loaded_chunks.end(); ++iter){
		loaded_chunks.push_back(iter->first);
	}
	return loaded_chunks;
}

ICuboid ChunkTable::boundingVolumeChunkspace() const{
	/*
	YAGNI: Update as chunks are added instead of this full recalculation nonsense

	Returns the bounding volume of the loaded world, where the coordinates
	are measured in chunks instead of individual voxels. For example, a world with 2x2
	loaded chunks centered on the origin:
		Correct Chunkspace Coordinates {
			.origin={-1, -1, -1},
			.extent={2, 2, 2}
		};
		
		Incorrect Voxel Coordinates {
			.origin={-CHUNK_LEN, -CHUNK_LEN, -CHUNK_LEN}
			.extent={2*CHUNK_LEN, 2*CHUNK_LEN, 2*CHUNK_LEN}
		};
	*/

	return m_bounds;
}
