#include "ChunkGenerator.hpp"


//-------------------------------------------------------------------------------------------------
// Coordinate Hashing
//-------------------------------------------------------------------------------------------------

/*
Bytes4 Hash::toLinearValue(IVec3 coord){
	return coord.x + (coord.y * LARGE_PRIME_1) + (coord.z * LARGE_PRIME_2);
}


CoordHash Hash::squirrelNoise(IVec3 coord, Bytes4 seed){
	//ATTRIBUTION: 
	//https://www.gdcvault.com/play/1024365/Math-for-Game-Programmers-Noise
	//Squirrel Eiserloh : Squirrel noise

	CoordHash mangled = toLinearValue(coord);
	
	mangled *= BIT_NOISE_1;
	mangled += seed;
	mangled ^= (mangled >> 8);
	mangled += BIT_NOISE_2;
	mangled ^= (mangled << 8);
	mangled *= BIT_NOISE_3;
	mangled ^= (mangled >> 8);

	return mangled;
}
*/

//-------------------------------------------------------------------------------------------------
// Generation Algorithm Base Class
//-------------------------------------------------------------------------------------------------
GenerationAlgorithm::GenerationAlgorithm(){
	m_seed = 0;
}

void GenerationAlgorithm::setSeed(Bytes8 seed){
	m_seed = seed;
}

//-------------------------------------------------------------------------------------------------
// Generation Algorithms
//-------------------------------------------------------------------------------------------------
//-----------------------------------------------
// NoiseLayers
//-----------------------------------------------
RawVoxelChunk NoiseLayers::generate(const ChunkTable* table, IVec3 coords){
	/*
	WARNING: Incomplete function
	*/

	RawVoxelChunk chunk;
	
	float density;

	IVec3 offset = coords * CHUNK_LEN;
	for(int z = 0; z < CHUNK_LEN; ++z)
	for(int y = 0; y < CHUNK_LEN; ++y)
	for(int x = 0; x < CHUNK_LEN; ++x){
		IVec3 local_coord = {x, y, z};
		IVec3 world_coord = offset + local_coord;
		density = toFloatVector(world_coord).length() / 5;

		chunk.data[linearChunkIndex(x, y, z)] = clampDensityToPalette(
			DEFAULT_VOXEL_PALETTE, 
			NUM_DEFAULT_PALETTE_OPTIONS, 
			density);
	}

	return chunk;
}

/*
RawVoxelChunk NoiseLayers::generateLodChunk(const ChunkTable* table, IVec3 coords, 
	int lod_power){
	//WARNING: Incomplete function
	assert(false);
	RawVoxelChunk chunk;
	return chunk;
}
*/


//-----------------------------------------------
// Fractal
//-----------------------------------------------
RawVoxelChunk Fractal::generate(const ChunkTable* table, IVec3 coords){
	/*
	WARNING: Incomplete function
	*/

	RawVoxelChunk chunk;
	
	float density;

	IVec3 offset = coords * CHUNK_LEN;
	for(int z = 0; z < CHUNK_LEN; ++z)
	for(int y = 0; y < CHUNK_LEN; ++y)
	for(int x = 0; x < CHUNK_LEN; ++x){
		IVec3 local_coord = {x, y, z};
		IVec3 world_coord = offset + local_coord;
		density = !(
			std::abs(world_coord.x) ^ 
			std::abs(world_coord.y) ^ 
			std::abs(world_coord.z));

		chunk.data[linearChunkIndex(x, y, z)] = clampDensityToPalette(
			DEFAULT_VOXEL_PALETTE, 
			NUM_DEFAULT_PALETTE_OPTIONS, 
			density);
	}

	return chunk;
}

/*
RawVoxelChunk Fractal::generateLodChunk(const ChunkTable* table, IVec3 coords, 
	int lod_power){
	//WARNING: Incomplete function
	assert(false);
	RawVoxelChunk chunk;
	return chunk;
}
*/


//-----------------------------------------------
// Centered Sphere
//-----------------------------------------------
RawVoxelChunk CenteredSphere::generate(const ChunkTable* table, IVec3 coords){
	/*
	
	*/

	RawVoxelChunk chunk;

	FVec3 origin = {0, 0, 0};
	
	constexpr float MAX_RADIUS = 1000;
	float radius = std::max(0.0f, std::min((float)m_seed, MAX_RADIUS));
	float distance, density;

	for(int z = 0; z < CHUNK_LEN; ++z)
	for(int y = 0; y < CHUNK_LEN; ++y)
	for(int x = 0; x < CHUNK_LEN; ++x){
		IVec3 local_pos = {x, y, z};
		IVec3 global_pos = (coords * CHUNK_LEN) + local_pos;
		FVec3 from_origin = toFloatVector(global_pos) - origin;
		distance = from_origin.length();
		density = (radius - distance / 2);

		chunk.data[linearChunkIndex(x, y, z)] = clampDensityToPalette(
			DEFAULT_VOXEL_PALETTE, NUM_DEFAULT_PALETTE_OPTIONS, density);
	}

	return chunk;
}

/*
RawVoxelChunk CenteredSphere::generateLodChunk(const ChunkTable* table, IVec3 coords, 
	int lod_power){


	RawVoxelChunk chunk;

	FVec3 origin = {0, 0, 0};

	int voxel_scale_factor = 1 << lod_power;
	int chunk_scale_factor = CHUNK_LEN * voxel_scale_factor;
	IVec3 chunk_voxel_coords = coords * chunk_scale_factor;

	float distance, density;

	for(int z = 0; z < CHUNK_LEN; ++z)
	for(int y = 0; y < CHUNK_LEN; ++y)
	for(int x = 0; x < CHUNK_LEN; ++x){
		IVec3 local_pos = {x, y, z};
		IVec3 global_pos = chunk_voxel_coords + local_pos * voxel_scale_factor;
		FVec3 from_origin = toFloatVector(global_pos) - origin;
		distance = from_origin.length();
		density = (50 - distance / 2);

		chunk.data[linearChunkIndex(x, y, z)] = clampDensityToPalette(
			DEFAULT_VOXEL_PALETTE, NUM_DEFAULT_PALETTE_OPTIONS, density);
	}

	return chunk;
}
*/

//-----------------------------------------------
// Scratch
//-----------------------------------------------
RawVoxelChunk Scratch::generate(const ChunkTable* table, IVec3 coords){
	RawVoxelChunk chunk;

	FVec3 origin = {0, 0, 0};

	float distance, density;
	constexpr int NOISE_MAX = 5;

	for(int z = 0; z < CHUNK_LEN; ++z)
	for(int y = 0; y < CHUNK_LEN; ++y)
	for(int x = 0; x < CHUNK_LEN; ++x){
		IVec3 local_pos = {x, y, z};
		FVec3 global_pos = toFloatVector((coords * CHUNK_LEN) + local_pos);
		FVec3 from_origin = global_pos - origin;
		distance = from_origin.length();
		density = (20 - distance / 10);
		density += (sin(global_pos.x / 10.0)*NOISE_MAX - NOISE_MAX/2);
		density += (cos(global_pos.y / 5.0)*NOISE_MAX - NOISE_MAX/2);
		density += (tan(distance / 10.0)*NOISE_MAX - NOISE_MAX/2);

		chunk.data[linearChunkIndex(x, y, z)] = clampDensityToPalette(
			DEFAULT_VOXEL_PALETTE, NUM_DEFAULT_PALETTE_OPTIONS, density);
	}
	return chunk;
}

/*
RawVoxelChunk Scratch::generateLodChunk(const ChunkTable* table, IVec3 coords, int lod_power){
	//WARNING: Incomplete function
	assert(false);
	RawVoxelChunk chunk;
	return chunk;
}
*/

//-----------------------------------------------
// Pillars And Caves
//-----------------------------------------------
RawVoxelChunk PillarsAndCaves::generate(const ChunkTable* table, IVec3 coords){
	/*
	https://iquilezles.org/articles/distfunctions/
	float opRep(FVec3 p, FVec3 c, in sdf3d primitive){
		vec3 q = mod(p+0.5*c,c)-0.5*c;
		return primitive(q);
	}
	*/
	
	RawVoxelChunk chunk;
	constexpr int NUM_PALETTE_OPTIONS = 2;
	Voxel VOXEL_PALETTE[] = {
		{VoxelType::Air}, 
		{VoxelType::Concrete},
	};

	float density;

	constexpr int PILLAR_WIDTH = 20;
	constexpr int PILLAR_MARGIN = 5;
	constexpr int CELL_SHIFT_AMOUNT = 17;
	int seed = 6789;
	seed = m_seed;

	IVec3 chunk_corner_pos = coords * CHUNK_LEN;

	for(int z = 0; z < CHUNK_LEN; ++z)
	for(int y = 0; y < CHUNK_LEN; ++y)
	for(int x = 0; x < CHUNK_LEN; ++x){
		IVec3 local_pos = {x, y, z};
		IVec3 global_pos = local_pos + chunk_corner_pos;

		IVec3 hash_pos = {
			(global_pos.x / CELL_SHIFT_AMOUNT) * CELL_SHIFT_AMOUNT,
			(global_pos.y / CELL_SHIFT_AMOUNT) * CELL_SHIFT_AMOUNT,
			(global_pos.z / CELL_SHIFT_AMOUNT) * CELL_SHIFT_AMOUNT
		};

		// Space transforms
		IVec3 pillar_space = {
			((int)(abs(global_pos.x) + 0.5 * PILLAR_WIDTH) % PILLAR_WIDTH) - 0.5 * PILLAR_WIDTH, 
			((int)(abs(global_pos.y) + 0.5 * PILLAR_WIDTH) % PILLAR_WIDTH) - 0.5 * PILLAR_WIDTH, 
			global_pos.z
		};

		// Density modifications
		bool is_pillar = true;
		if(global_pos.z > 5){
			density = 0;
		}else{
			for(int i = 0; i < 2; ++i){
				is_pillar &= pillar_space[i] <= PILLAR_WIDTH - PILLAR_MARGIN;
				is_pillar &= pillar_space[i] >= PILLAR_MARGIN;
			}
			density = is_pillar;
			
			if(!is_pillar && global_pos.z < -20){
				// Large blocky rooms
				CoordHash hash = Hash::modifiedSquirrelNoise(hash_pos, seed);
				density += hash;
			}
		}
		
		// Write data out
		chunk.data[linearChunkIndex(x, y, z)] = clampDensityToPalette(
			VOXEL_PALETTE, NUM_PALETTE_OPTIONS, density);

		VoxelType type = chunk.data[linearChunkIndex(x, y, z)].type;
		assert(type == VoxelType::Air || type == VoxelType::Concrete);
	}
	return chunk;
}

/*
RawVoxelChunk PillarsAndCaves::generateLodChunk(const ChunkTable* table, IVec3 coords, int lod_power){
	//WARNING: Incomplete function
	assert(false);
	RawVoxelChunk chunk;
	return chunk;
}
*/


//-----------------------------------------------
// MeshTesting
//-----------------------------------------------
bool containsVoxel(ICuboid cuboid, IVec3 voxel_coord){
	/*
	Returns true if the voxel coordinate is inside the cuboid.
	NOTE: An extent of 0 means no volume and no intersection.
	*/

	for(int i = 0; i < 3; ++i){
		if(voxel_coord[i] < cuboid.origin[i] || 
			voxel_coord[i] >= cuboid.origin[i] + cuboid.extent[i]){
			return false;
		}
	}
	return true;
}

RawVoxelChunk MeshTesting::generate(const ChunkTable* table, IVec3 coords){
	RawVoxelChunk chunk;
	constexpr int NUM_PALETTE_OPTIONS = 2;
	Voxel VOXEL_PALETTE[] = {
		{VoxelType::Air}, 
		{VoxelType::Metal},
	};

	float density;

	int CUBE_LENGTH = m_seed;
	ICuboid solid_range = {
		{CUBE_LENGTH, CUBE_LENGTH, CUBE_LENGTH}, 
		{CUBE_LENGTH, CUBE_LENGTH, CUBE_LENGTH}
	};


	IVec3 chunk_corner_pos = coords * CHUNK_LEN;

	for(int z = 0; z < CHUNK_LEN; ++z)
	for(int y = 0; y < CHUNK_LEN; ++y)
	for(int x = 0; x < CHUNK_LEN; ++x){
		IVec3 local_pos = {x, y, z};
		IVec3 global_pos = local_pos + chunk_corner_pos;

		density = containsVoxel(solid_range, global_pos);

		// Write data out
		chunk.data[linearChunkIndex(x, y, z)] = clampDensityToPalette(
			VOXEL_PALETTE, NUM_PALETTE_OPTIONS, density);
	}

	return chunk;
}

/*
RawVoxelChunk MeshTesting::generateLodChunk(const ChunkTable* table, IVec3 coords, int lod_power){
	//WARNING: Incomplete function
	
	assert(false);
	RawVoxelChunk chunk;
	return chunk;
}
*/


//-------------------------------------------------------------------------------------------------
// Chunk Generator
//-------------------------------------------------------------------------------------------------
void ChunkGenerator::setSeed(Bytes8 seed){
	if(seed == 0){
		seed = time(NULL);
	}
	printf("ChunkGenerator: Seed set to '%li'\n", seed);
	srand(seed);

	m_algorithm->setSeed(seed);
}

void ChunkGenerator::setAlgorithm(std::shared_ptr<GenerationAlgorithm> new_generator){
	m_algorithm = new_generator;
}

RawVoxelChunk ChunkGenerator::generate(const ChunkTable* table, IVec3 coords){
	return m_algorithm->generate(table, coords);
}

/*
RawVoxelChunk ChunkGenerator::generateLodChunk(const ChunkTable* table, IVec3 coords, 
	int lod_power){

	return m_algorithm->generateLodChunk(table, coords, lod_power);
}
*/

