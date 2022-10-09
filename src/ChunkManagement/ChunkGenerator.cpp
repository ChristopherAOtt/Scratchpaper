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
	WARNING: The conversion from voxel_space->sample_space->voxel_space introduces
		unacceptable rounding error that causes the "cage" to go out of bounds. This
		is currently being dealt with by artificially clamping the t_values into range.

	TODO: Rewrite to keep voxel-space cage positions available at all times to avoid
		the round-trip through floating point math.
	*/

	RawVoxelChunk chunk;

	// TODO: Remove after testing
	for(const Layer& layer : m_layers){
		for(Int32 i = 0; i < 3; ++i){
			assert(layer.scale[i] > 0);
		}

		Int32 layer_range_extent = layer.value_range.y - layer.value_range.x;
		assert(layer_range_extent >= 0);
	}

	// Noise layers add/subtract "density" from each cell, so the buffer needs
	// to be cleared before each run.
	setNoiseScratchBufferValue(0);
	IVec3 gdb_chunk_coord = coords;
	IVec3 vox_offset_chunk_base = coords * CHUNK_LEN;
	FVec3 vox_start = toFloatVector(vox_offset_chunk_base);
	FVec3 vox_stop = vox_start + FVec3{CHUNK_LEN, CHUNK_LEN, CHUNK_LEN};
	for(Layer layer : m_layers){
		Int32 layer_range_extent = layer.value_range.y - layer.value_range.x;
		FVec3 voxels_per_sample = layer.scale * NUM_VOXELS_PER_METER;
		FVec3 sample_t_per_voxel;
		for(Int32 i = 0; i < 3; ++i){
			sample_t_per_voxel[i] = (1.0f / voxels_per_sample[i]);
		}
		FVec3 sample_chunk_start = hadamard(vox_start, sample_t_per_voxel);
		FVec3 sample_chunk_stop = hadamard(vox_stop, sample_t_per_voxel);

		IVec2 ssr[3];  // Sample Space Ranges
		for(Int32 i = 0; i < 3; ++i){
			// Range of sample space cell coordinates
			IVec2 sample_range = {
				(Int32) floor(sample_chunk_start[i]),
				(Int32) ceil(sample_chunk_stop[i]),
			};
			ssr[i] = sample_range;
		}

		// For larger sample spaces this should degenerate to the chunk's bounding
		// coordinates measured in sample space.
		for(Int32 z = ssr[INDEX_Z][INDEX_VALUE_MIN]; z < ssr[INDEX_Z][INDEX_VALUE_MAX]; ++z)
		for(Int32 y = ssr[INDEX_Y][INDEX_VALUE_MIN]; y < ssr[INDEX_Y][INDEX_VALUE_MAX]; ++y)
		for(Int32 x = ssr[INDEX_X][INDEX_VALUE_MIN]; x < ssr[INDEX_X][INDEX_VALUE_MAX]; ++x){
			IVec3 sample_coord_low =  {x + 0, y + 0, z + 0};
			IVec3 sample_coord_high = {x + 1, y + 1, z + 1};
			
			FVec3 t_sample_low =  max(toFloatVector(sample_coord_low), sample_chunk_start);
			FVec3 t_sample_high = min(toFloatVector(sample_coord_high), sample_chunk_stop);
			
			IVec3 vox_cage_origin = toIntVector(hadamard(t_sample_low, voxels_per_sample));
			IVec3 vox_cage_end =    toIntVector(hadamard(t_sample_high, voxels_per_sample));
			IVec3 vox_cage_extent = vox_cage_end - vox_cage_origin;
			FVec3 local_t_sample = {
				clamp(floatMod(t_sample_low.x, 1), 0.0, 1.0), 
				clamp(floatMod(t_sample_low.y, 1), 0.0, 1.0), 
				clamp(floatMod(t_sample_low.z, 1), 0.0, 1.0)
			};

			SampleCage cage = {
				.cage_local_volume={
					.origin=vox_cage_origin - vox_offset_chunk_base,
					.extent=vox_cage_extent
				},
				.t_local_initial=local_t_sample,
				.t_increment=sample_t_per_voxel,
			};

			SampleCoords sample_coords = sampleCoordsFromPosition(sample_coord_low);
			for(Int32 i = 0; i < NUM_CORNERS_PER_CUBE; ++i){
				IVec3 sample_coord = sample_coords.coords[i];
				Hash::CoordHash hash = Hash::modifiedSquirrelNoise(sample_coord, m_seed);
				Range32 rand_range = {layer.value_range.x, layer_range_extent};
				cage.samples[i] = MathUtils::Random::randInRange(hash, rand_range);
			}

			iterateCageValues(cage);
		}
	}

	constexpr Int32 NUM_PALETTE_OPTIONS = 4;
	constexpr Voxel PALETTE[] = {
		{VoxelType::Air}, 
		{VoxelType::Dirt},
		{VoxelType::Dirt},
		{VoxelType::Stone},
	};

	// Now iterate through the results and clamp to the palette
	Int32 index = 0;
	for(Int32 z = 0; z < CHUNK_LEN; ++z)
	for(Int32 y = 0; y < CHUNK_LEN; ++y)
	for(Int32 x = 0; x < CHUNK_LEN; ++x){
		float height_adjustment = -(z + vox_offset_chunk_base.z) * 0.17;
		float final_density = m_noise_scratch_buffer[index] + height_adjustment;

		chunk.data[index] = clampDensityToPalette(PALETTE, NUM_PALETTE_OPTIONS, final_density);
		++index;
	}

	// Final type-dependent touch-ups
	// TODO: Reference other chunks for border voxels
	index = 0;
	for(Int32 z = 0; z < CHUNK_LEN; ++z)
	for(Int32 y = 0; y < CHUNK_LEN; ++y)
	for(Int32 x = 0; x < CHUNK_LEN; ++x){
		VoxelType& curr_type = chunk.data[index].type;
		if(curr_type == VoxelType::Dirt){
			VoxelType above_type = chunk.data[linearChunkIndex(x, y, z + 1)].type;
			bool is_above_air = (z == CHUNK_LEN - 1) || above_type == VoxelType::Air;

			if(is_above_air){
				curr_type = VoxelType::Grass;
			}
		}

		++index;
	}

	return chunk;
}

void NoiseLayers::addNoiseLayer(Layer layer){
	m_layers.push_back(layer);
}

NoiseLayers::SampleCoords NoiseLayers::sampleCoordsFromPosition(IVec3 coord){
	NoiseLayers::SampleCoords coords;

	Int32 write_index = 0;
	for(Int32 z = 0; z < 2; ++z)
	for(Int32 y = 0; y < 2; ++y)
	for(Int32 x = 0; x < 2; ++x){
		coords.coords[write_index++] = coord + IVec3{x, y, z};
	}

	return coords;
}

void NoiseLayers::iterateCageValues(SampleCage cage){
	/*
	All samples will fall within the sample cage. Helps avoid unnecessary
	re-sampling for noise layers with very large distances between samples.
	*/
	
	ICuboid vol = cage.cage_local_volume;
	IVec3 end = vol.origin + vol.extent;
	for(Int32 i = 0; i < 3; ++i){
		assert(vol.origin[i] >= 0);
		assert(end[i] <= CHUNK_LEN);
	}

	FVec3 curr_t = cage.t_local_initial;
	for(int z = vol.origin.z; z < end.z; ++z){
		for(int y = vol.origin.y; y < end.y; ++y){
			for(int x = vol.origin.x; x < end.x; ++x){
				float value = trilinearInterpolation(cage.samples, curr_t);
				m_noise_scratch_buffer[linearChunkIndex(x, y, z)] += value;
				
				curr_t.x += cage.t_increment.x;
			}
			curr_t.x = cage.t_local_initial.x;
			curr_t.y += cage.t_increment.y;
		}
		curr_t.y = cage.t_local_initial.y;
		curr_t.z += cage.t_increment.z;
	}
}

float NoiseLayers::trilinearInterpolation(const float* samples, FVec3 t_values) const{
	/*
	Given a cube of samples and the t_values between them, perform a 
	trilinear interpolation and return the value.
	*/

	// Lerp coords along x
	float lerp_x1 = lerp(samples[0], samples[1], t_values.x);
	float lerp_x2 = lerp(samples[2], samples[3], t_values.x);
	float lerp_x3 = lerp(samples[4], samples[5], t_values.x);
	float lerp_x4 = lerp(samples[6], samples[7], t_values.x);

	// Lerp prior results along y
	float lerp_y1 = lerp(lerp_x1, lerp_x2, t_values.y);
	float lerp_y2 = lerp(lerp_x3, lerp_x4, t_values.y);

	// Lerp prior results along z.
	float lerp_z1 = lerp(lerp_y1, lerp_y2, t_values.z);

	return lerp_z1;
}

void NoiseLayers::setNoiseScratchBufferValue(float value){
	for(Int32 i = 0; i < CHUNK_VOLUME; ++i){
		m_noise_scratch_buffer[i] = value;
	}
}


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
				Hash::CoordHash hash = Hash::modifiedSquirrelNoise(hash_pos, seed);
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

