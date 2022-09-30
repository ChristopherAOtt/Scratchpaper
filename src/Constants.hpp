#pragma once
#include <limits.h>
#include "Types.hpp"

// General
constexpr float GOLDEN_RATIO = 1.618033988749895;
constexpr float PI = 3.14159265358979;
constexpr float TAU = PI * 2;
constexpr int BITS_PER_BYTE = 8;
constexpr float ARBITRARY_EPSILON = 0.0000001;
constexpr float LARGE_FLOAT = 99999999999;
constexpr uint MAX_INT32_VALUE = INT_MAX;
constexpr long long int MAX_INT64_VALUE = LLONG_MAX;
constexpr Int32 NUM_VERTICES_PER_TRIANGLE = 3;

// Used when indexing into min/max arrays to make the ordering clearer
constexpr int INDEX_VALUE_MIN = 0;
constexpr int INDEX_VALUE_MAX = 1;

// Ordering of the X, Y, and Z axes
constexpr int INDEX_X = 0;
constexpr int INDEX_Y = 1;
constexpr int INDEX_Z = 2;
constexpr int NUM_3D_AXES = 3;
enum Axis{
	AXIS_X = INDEX_X,  // = 0
	AXIS_Y = INDEX_Y,  // = 1
	AXIS_Z = INDEX_Z,  // = 2
	AXIS_INVALID = AXIS_Z + 1,  // = 3
};

enum GridDirection : unsigned char{
	/*
	NOTE: This can be used as an index, so the INVALID numerical value is 
	"above" the valid ones.

	NOTE: These can be converted to an axis by integer division by 2.
	IE: DIR_Y_NEG / 2 = 1, which is the Y axis
	*/

	DIR_X_POS = 0,
	DIR_X_NEG = 1,

	DIR_Y_POS = 2,
	DIR_Y_NEG = 3,

	DIR_Z_POS = 4,
	DIR_Z_NEG = 5,

	DIR_INVALID = 6,
};

static_assert(DIR_X_POS / 2 == AXIS_X);
static_assert(DIR_X_NEG / 2 == AXIS_X);
static_assert(DIR_Y_POS / 2 == AXIS_Y);
static_assert(DIR_Y_NEG / 2 == AXIS_Y);
static_assert(DIR_Z_POS / 2 == AXIS_Z);
static_assert(DIR_Z_NEG / 2 == AXIS_Z);

// Voxel-related
constexpr int NUM_FACES_PER_VOXEL = 6;

enum OptimizationLevel{
	/*
	To be used in the construction settings of structures 
	built/managed with a build_speed/optimization tradeoff.
	*/

	OPTIMIZE_NONE       = 0,  // Get structure built, no matter how badly.
	OPTIMIZE_LOW        = 1,  // Trivial, fast optimizations.
	OPTIMIZE_MEDIUM     = 2,  // Moderate speed loss for increase in quality.
	OPTIMIZE_HIGH       = 3,  // When there's lots of time to build it right.
	OPTIMIZE_EXHAUSTIVE = 4,  // Literally everything, no matter how slow.
};
