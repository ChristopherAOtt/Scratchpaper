#include "MathUtils.hpp"

//------------------------------------------------------------------------------
float min(float a, float b){
	if(a < b){
		return a;
	}
	return b;
}

float max(float a, float b){
	if(a > b){
		return a;
	}
	return b;	
}

float lerp(int a, int b, float t){
	return ((1 - t) * a) + (t * b);
}

float clamp(float value, float min, float max){
	if(value < min) value = min;
	if(value > max) value = max;
	return value;
}

int min(int a, int b){
	if(a < b){
		return a;
	}
	return b;
}

int max(int a, int b){
	if(a > b){
		return a;
	}
	return b;
}

int clamp(int value, int min, int max){
	if(value < min) value = min;
	if(value > max) value = max;
	return value;
}

IVec2 min(IVec2 vec, IVec2 min_vec){
	for(int i = 0; i < 2; ++i){
		vec[i] = min(vec[i], min_vec[i]);
	}
	return vec;
}

IVec2 max(IVec2 vec, IVec2 max_vec){
	for(int i = 0; i < 2; ++i){
		vec[i] = max(vec[i], max_vec[i]);
	}
	return vec;
}

IVec3 clamp(IVec3 vec, int min, int max){
	for(int i = 0; i < 3; ++i){
		vec[i] = clamp(vec[i], min, max);
	}
	return vec;
}

FVec3 lerp(FVec3 a, FVec3 b, float t){
	return ((1 - t) * a) + (t * b);
}

//------------------------------------------------------------------------------
glm::vec3 toGlm(const FVec3& vec){
	return {vec.x, vec.y, vec.z};
}

FMat4 fromGlm(glm::mat4 glm_matrix){
	FMat4 output_matrix;
	for(int y = 0; y < 4; ++y){
		for(int x = 0; x < 4; ++x){
			output_matrix[x][y] = glm_matrix[x][y];
		}
	}
	return output_matrix;
}

FMat4 MathUtils::Matrix::makePerspectiveProjectionMatrix(const Camera& camera){
	/*
	ATTRIBUTION: Perspective matrix
	https://www.scratchapixel.com/lessons/3d-basic-rendering/
		perspective-and-orthographic-projection-matrix/
		opengl-perspective-projection-matrix

	return 
	*/

	// Hacky conversion from glm to FMat4 to get something rendering faster.
	glm::mat4 glm_matrix = glm::perspective(
		glm::radians(camera.fov), 
		camera.aspect_ratio, 
		camera.near_plane, 
		camera.far_plane);
	return fromGlm(glm_matrix);


	float vertical_scale = tan(camera.fov * 0.5 * PI / 180) * camera.near_plane;

	float plane_diff = camera.far_plane - camera.near_plane;
	float r = camera.aspect_ratio * vertical_scale;
	float l = -r;
	float t = +vertical_scale;
	float b = -vertical_scale;
	
	FMat4 output_matrix = {
		{
			(2 * camera.near_plane) / (r - l), 
			0, 
			(r + l) / (r - l), 
			0
		},
		{
			0, 
			(2 * camera.near_plane) / (t - b), 
			(t + b) / (t - b), 
			0
		},
		{
			0, 
			0, 
			-(camera.far_plane + camera.near_plane) / plane_diff, 
			-(2 * camera.far_plane * camera.near_plane) / plane_diff
		},
		{
			0, 
			0, 
			-1, 
			0
		},
	};
	return output_matrix;
}

FMat4 MathUtils::Matrix::makeViewMatrix(const Camera& camera){
	FVec3 look_target = camera.pos + camera.basis.v1;
	glm::mat4 glm_output = glm::lookAt(
		toGlm(camera.pos), 
		toGlm(look_target), 
		toGlm(camera.basis.v2));

	return fromGlm(glm_output);
}

FMat4 MathUtils::Matrix::makeChunkModelMatrix(const IVec3& chunkpos){
	/*
	TODO: Fix for arbitrary locations
	*/

	glm::mat4 output;

	constexpr int CHUNK_LEN = 32;  // TODO: Refactor to remove duplicate CHUNK_LEN definitions

	glm::vec3 rot = {0, 0, 0};
	glm::vec3 sca = {1, 1, 1};
	glm::vec3 pos = {
		chunkpos.x * CHUNK_LEN, 
		chunkpos.y * CHUNK_LEN, 
		chunkpos.z * CHUNK_LEN
	};

	output = glm::translate(output, pos);
	
	output = glm::rotate(output, glm::radians(rot.x), {1, 0, 0});
	output = glm::rotate(output, glm::radians(rot.y), {0, 1, 0});
	output = glm::rotate(output, glm::radians(rot.z), {0, 0, 1});

	output = glm::scale(output, sca);

	return fromGlm(output);
}

FMat4 MathUtils::Matrix::makeModelMatrix(const FVec3& position){
	glm::mat4 output;

	//glm::vec3 rot = toGlm(entity.rotation);
	//glm::vec3 sca = toGlm(entity.scale);
	//glm::vec3 pos = toGlm(entity.position);

	glm::vec3 rot = {0, 0, 0};
	glm::vec3 sca = {1, 1, 1};
	glm::vec3 pos = toGlm(position);

	output = glm::translate(output, pos);
	
	output = glm::rotate(output, glm::radians(rot.x), {1, 0, 0});
	output = glm::rotate(output, glm::radians(rot.y), {0, 1, 0});
	output = glm::rotate(output, glm::radians(rot.z), {0, 0, 1});

	output = glm::scale(output, sca);

	return fromGlm(output);
}

FMat4 MathUtils::Matrix::initIdentityMatrix(){
	FMat4 output = {
		{1, 0, 0, 0},
		{0, 1, 0, 0},
		{0, 0, 1, 0},
		{0, 0, 0, 1},
	};

	return output;
}

//------------------------------------------------------------------------------
// Random namespace
//------------------------------------------------------------------------------
Uint64 MathUtils::Random::SebVignaSplitmix64::next(){
	/*
	ATTRIBUTION: https://prng.di.unimi.it/splitmix64.c
	*/

	Uint64 z = (state +=  0x9e3779b97f4a7c15);
	z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
	z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
	return z ^ (z >> 31);
}

MathUtils::Random::Xoroshiro128Plus 
MathUtils::Random::Xoroshiro128Plus::init(Uint64 seed){
	/*
	ATTRIBUTION: Initializing the state with splitmix
		https://prng.di.unimi.it/xoroshiro128plus.c
	*/

	SebVignaSplitmix64 splitmix{seed};

	Xoroshiro128Plus new_generator;
	new_generator.state[0] = splitmix.next();
	new_generator.state[1] = splitmix.next();
	return new_generator;
}

Uint64 MathUtils::Random::Xoroshiro128Plus::next(){
	/*
	ATTRIBUTION: This is from the xoroshiro128+ code listed at:
		https://prng.di.unimi.it/xoroshiro128plus.c
	*/

	const Uint64 s0 = state[0];
	Uint64 s1 = state[1];
	const Uint64 result = s0 + s1;

	s1 ^= s0;
	state[0] = rotateLeft(s0, 24) ^ s1 ^ (s1 << 16); // a, b
	state[1] = rotateLeft(s1, 37); // c

	return result;
}

void MathUtils::Random::Xoroshiro128Plus::jump(){
	/*
	ATTRIBUTION: This is from the xoroshiro128+ code listed at:
		https://prng.di.unimi.it/xoroshiro128plus.c

	AUTHOR COMMENT BELOW:
	This is the jump function for the generator. It is equivalent
	to 2^64 calls to next(); it can be used to generate 2^64
	non-overlapping subsequences for parallel computations.	
	*/

	static const Uint64 JUMP[2] = {
		0xdf900294d8f554a5, 0x170865df4b3201fc
	};

	Uint64 s0 = 0;
	Uint64 s1 = 0;
	for(int i = 0; i < sizeof(JUMP) / sizeof(*JUMP); i++){
		for(int b = 0; b < 64; b++){
			if (JUMP[i] & UINT64_C(1) << b){
				s0 ^= state[0];
				s1 ^= state[1];
			}
			next();
		}
	}

	state[0] = s0;
	state[1] = s1;
}
