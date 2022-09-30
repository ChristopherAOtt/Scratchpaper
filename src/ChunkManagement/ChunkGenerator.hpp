#pragma once

#include "Primitives.hpp"
#include "Chunks.hpp"

#include <memory>


//-------------------------------------------------------------------------------------------------
// Hashing
//-------------------------------------------------------------------------------------------------
typedef Bytes4 CoordHash;
namespace Hash{
	/*
	constexpr int LARGE_PRIME_1 = 198491317;
	constexpr int LARGE_PRIME_2 = 6542989;

	constexpr Bytes4 BIT_NOISE_1 = 0xB5297A4D;
	constexpr Bytes4 BIT_NOISE_2 = 0x68E31DA4;
	constexpr Bytes4 BIT_NOISE_3 = 0x1B56C4E9;

	Bytes4 toLinearValue(IVec3 coord);
	CoordHash squirrelNoise(IVec3 coord, Bytes4 seed);
	*/

};

//-------------------------------------------------------------------------------------------------
// Chunk Generator
//-------------------------------------------------------------------------------------------------
constexpr int NUM_DEFAULT_PALETTE_OPTIONS = 6;
constexpr Voxel DEFAULT_VOXEL_PALETTE[] = {
	{VoxelType::Air}, 
	{VoxelType::Grass}, 
	{VoxelType::Dirt}, 
	{VoxelType::Stone}, 
	{VoxelType::Concrete}, 
	{VoxelType::Metal}
};

inline Voxel clampDensityToPalette(const Voxel* palette_ptr, int num_palette_options, 
	float density){
	/*
	Given a density, clamp to fit within the palette and convert to an index
	*/

	int type_index = std::min(num_palette_options - 1, std::max((int) density, 0));
	return palette_ptr[type_index];
}

//-----------------------------------------------
// Base Class
//-----------------------------------------------
struct GenerationAlgorithm{
	public:
		GenerationAlgorithm();
		
		RawVoxelChunk virtual generate(const ChunkTable* table, IVec3 coords) = 0;
		//RawVoxelChunk virtual generateLodChunk(const ChunkTable* table, IVec3 coords, 
		//	int lod_power) = 0;
		void setSeed(Bytes8 seed);

	protected:
		Bytes8 m_seed;
};

//-----------------------------------------------
// Generation Algorithms
//-----------------------------------------------
class NoiseLayers: public GenerationAlgorithm{
	public:
		RawVoxelChunk generate(const ChunkTable* table, IVec3 coords);
		//RawVoxelChunk generateLodChunk(const ChunkTable* table, IVec3 coords, int lod_power);
};

class Fractal: public GenerationAlgorithm{
	public:
		RawVoxelChunk generate(const ChunkTable* table, IVec3 coords);
		//RawVoxelChunk generateLodChunk(const ChunkTable* table, IVec3 coords, int lod_power);
};

class CenteredSphere: public GenerationAlgorithm{
	public:
		RawVoxelChunk generate(const ChunkTable* table, IVec3 coords);
		//RawVoxelChunk generateLodChunk(const ChunkTable* table, IVec3 coords, int lod_power);
};

class Scratch: public GenerationAlgorithm{
	public:
		RawVoxelChunk generate(const ChunkTable* table, IVec3 coords);
		//RawVoxelChunk generateLodChunk(const ChunkTable* table, IVec3 coords, int lod_power);
};

class PillarsAndCaves: public GenerationAlgorithm{
	public:
		RawVoxelChunk generate(const ChunkTable* table, IVec3 coords);
		//RawVoxelChunk generateLodChunk(const ChunkTable* table, IVec3 coords, int lod_power); 
};

class MeshTesting: public GenerationAlgorithm{
	public:
		RawVoxelChunk generate(const ChunkTable* table, IVec3 coords);
		//RawVoxelChunk generateLodChunk(const ChunkTable* table, IVec3 coords, int lod_power);
};

//-----------------------------------------------
// ChunkGenerator Class
//-----------------------------------------------
class ChunkGenerator{
	/*
	TODO: Add support for multiple algorithms running simultaneously.
	*/

	public:
		void setSeed(Bytes8 seed);
		void setAlgorithm(std::shared_ptr<GenerationAlgorithm> algorithm);
		RawVoxelChunk generate(const ChunkTable* table, IVec3 coords);
		//RawVoxelChunk generateLodChunk(const ChunkTable* table, IVec3 coords, int lod_power);

	private:
		std::shared_ptr<GenerationAlgorithm> m_algorithm;
		//std::vector<std::shared_ptr<GenerationAlgorithm>> m_algorithm_list;
};

