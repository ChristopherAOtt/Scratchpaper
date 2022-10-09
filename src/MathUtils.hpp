#pragma once

#include "Primitives.hpp"
#include "Camera.hpp"
#include "GLIncludes.hpp"

/*
NOTE: This file has become a dumping ground for random helper functions, and
has become increasingly disorganized over time. Cleanup is needed.
*/

namespace MathUtils{
	namespace Matrix{
		FMat4 makePerspectiveProjectionMatrix(const Camera& camera);
		FMat4 makeViewMatrix(const Camera& camera);
		FMat4 makeChunkModelMatrix(const IVec3& chunkpos);
		FMat4 makeModelMatrix(FVec3 position);
		FMat4 makeModelMatrix(FVec3 position, Basis basis);

		FMat4 initIdentityMatrix();
	}

	namespace Random{
		/*
		ATTRIBUTION: For basically everything in this namespace, heavy reference 
		was made to prng explanations given here:
			https://prng.di.unimi.it/
		*/

		static inline double doubleFromUint64(Uint64 x){
			const union{
				Uint64 i;
				double d;
			} u = {
				.i = UINT64_C(0x3FF) << 52 | x >> 12
			};
		
			return u.d - 1.0;
		}

		static inline Uint64 rotateLeft(const Uint64 x, int k){
			/*
			Rotates bits k amount to the left with wraparound.

			ATTRIBUTION: This is from the xoroshiro128+ code listed at:
				https://prng.di.unimi.it/xoroshiro128plus.c
			*/

			return (x << k) | (x >> (64 - k));
		}

		struct SebVignaSplitmix64{
			/*
			ATTRIBUTION: Sebastiano Vigna's Splitmix64 algorithm
				https://prng.di.unimi.it/splitmix64.c
			*/

			Uint64 state;

			Uint64 next();
		};

		struct Xoroshiro128Plus{
			/*
			Slight modifications were made, such as packaging the
			state and functions into a struct instead of having 
			them reference static info.

			ATTRIBUTION: Xoroshiro128+ algorithm by David Blackman and Sebastiano Vigna
				https://prng.di.unimi.it/xoroshiro128plus.c
			*/

			Uint64 state[2];

			static Xoroshiro128Plus init(Uint64 seed);
			Uint64 next();
			void jump();
		};

		static inline float randInRange(Uint64 seed, Range32 range){
			double ratio = ((double) seed) / (double) MAX_UINT64_VALUE;
			float result = (ratio * range.extent) + range.origin;

			return result;
		}
	};
};

// TODO: Templates
float min(float a, float b);
float max(float a, float b);
float lerp(float a, float b, float t);
float lerp(int a, int b, float t);
float clamp(float value, float min, float max);

int min(int a, int b);
int max(int a, int b);
int clamp(int value, int min, int max);

IVec2 min(IVec2 vec, IVec2 min_vec);
IVec2 max(IVec2 vec, IVec2 max_vec);

IVec3 clamp(IVec3 vec, int min, int max);

FVec3 min(FVec3 a, FVec3 b);
FVec3 max(FVec3 a, FVec3 b);
FVec3 lerp(FVec3 a, FVec3 b, float t);

Int32 directionalMod(Int32 value, Int32 mod);
float floatMod(float value, Int32 mod);
float signedFract(float input);

