#pragma once

#include "Primitives.hpp"
#include "Chunks.hpp"
#include "MathUtils.hpp"

#include <memory>


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
		struct Layer{
			FVec3 scale;  // Meters per sample
			IVec2 value_range;
		};

	public:
		RawVoxelChunk generate(const ChunkTable* table, IVec3 coords);
		void addNoiseLayer(Layer layer);
		//RawVoxelChunk generateLodChunk(const ChunkTable* table, IVec3 coords, int lod_power);

	private:
		struct SampleCage{
			ICuboid cage_local_volume;

			FVec3 t_local_initial;
			FVec3 t_increment;

			float samples[NUM_CORNERS_PER_CUBE];
		};

		struct SampleCoords{
			IVec3 coords[NUM_CORNERS_PER_CUBE];
		};

	private:
		SampleCoords sampleCoordsFromPosition(IVec3 coord);
		void iterateCageValues(SampleCage cage);
		float trilinearInterpolation(const float* samples, FVec3 t_values) const;

		void setNoiseScratchBufferValue(float value);

	private:
		std::vector<Layer> m_layers;
		float m_noise_scratch_buffer[CHUNK_VOLUME];
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

