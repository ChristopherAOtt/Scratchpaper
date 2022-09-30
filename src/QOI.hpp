#pragma once

#include "Primitives.hpp"
#include "FileIO.hpp"
#include <arpa/inet.h>

/*
https://qoiformat.org/qoi-specification.pdf
Specification Version 1.0, 2022.01.05 – qoiformat.org – Dominic Szablewski
*/

namespace Qoi{
	constexpr int NUM_RGBA_CHANNELS = 4;
	constexpr int NUM_RGB_CHANNELS = 3;

	struct CompactRGB{
		union{
			Uint8 arr[NUM_RGB_CHANNELS];
			struct{
				Uint8 r, g, b;
			}named;
		};
	};

	struct CompactRGBA{
		union{
			Uint8 arr[NUM_RGBA_CHANNELS];
			struct{
				Uint8 r, g, b, a;
			}named;	
		};
	};

	struct FileHeader{
		char magic[4];  // Must equal "qoif"
		Uint32 width;
		Uint32 height;
		Uint8 num_channels; // 3 = RGB, 4 = RGBA
		Uint8 colorspace;  // 0 = sRGB with linear alpha, 1 = all linear
	};

	constexpr int NUM_CHUNK_TYPES = 6;
	enum ChunkOpType{
		CHUNK_OP_RGB     = 0,
		CHUNK_OP_RGBA    = 1,
		CHUNK_OP_INDEX   = 2,
		CHUNK_OP_DIFF    = 3,
		CHUNK_OP_LUMA    = 4,
		CHUNK_OP_RUN     = 5,
		CHUNK_OP_INVALID = 6,
	};

	constexpr int OP_CHUNK_BYTE_SIZES[] = {
		4,  // CHUNK_OP_RGB
		5,  // CHUNK_OP_RGBA
		1,  // CHUNK_OP_INDEX
		1,  // CHUNK_OP_DIFF
		2,  // CHUNK_OP_LUMA
		1,  // CHUNK_OP_RUN
		1,  // CHUNK_OP_INVALID
	};

	constexpr const char* OP_STRINGS[] = {
		"CHUNK_OP_RGB",
		"CHUNK_OP_RGBA",
		"CHUNK_OP_INDEX",
		"CHUNK_OP_DIFF",
		"CHUNK_OP_LUMA",
		"CHUNK_OP_RUN",
		"CHUNK_OP_INVALID",
	};

	// Two types of tags to mask out
	constexpr Bytes1 MASK_BIG_OP   = 0b1111'1111;
	constexpr Bytes1 MASK_SMALL_OP = 0b1100'0000;
	
	// The possible tag results
	constexpr Bytes1 TAG_RGB       = 0b1111'1110;
	constexpr Bytes1 TAG_RGBA      = 0b1111'1111;
	constexpr Bytes1 TAG_INDEX     = 0b0000'0000;
	constexpr Bytes1 TAG_DIFF      = 0b0100'0000;
	constexpr Bytes1 TAG_LUMA      = 0b1000'0000;
	constexpr Bytes1 TAG_RUN       = 0b1100'0000;

	constexpr Bytes8 END_OF_STREAM_VALUE = 0x00'00'00'00'00'00'00'01;

	struct TagExtractionInfo{
		ChunkOpType type;
		Bytes1 mask;
		Bytes1 tag_result;
	};

	const TagExtractionInfo extraction_arr[] = {
		{CHUNK_OP_RGB,   MASK_BIG_OP,   TAG_RGB},
		{CHUNK_OP_RGBA,  MASK_BIG_OP,   TAG_RGBA},
		{CHUNK_OP_INDEX, MASK_SMALL_OP, TAG_INDEX},
		{CHUNK_OP_DIFF,  MASK_SMALL_OP, TAG_DIFF},
		{CHUNK_OP_LUMA,  MASK_SMALL_OP, TAG_LUMA},
		{CHUNK_OP_RUN,   MASK_SMALL_OP, TAG_RUN},
	};

	constexpr inline int index(CompactRGBA color){
		constexpr int weights[4] = {3, 5, 7, 11};
		int index_position = 0;
		for(int i = 0; i < NUM_RGBA_CHANNELS; ++i){
			index_position += color.arr[i] * weights[i];
		}
		return index_position % 64;
	}

	struct Operation{
		ChunkOpType type;
		union{
			struct{
				Uint8 colors[3];
			}rgb;

			struct{
				Uint8 colors[4];
			}rgba;

			struct{
				Uint8 index;
			}index;

			struct{
				Int8 delta_rgb[3];
			}diff;

			struct{
				Int8 delta_rgb[3];
			}luma;

			struct{
				Uint8 run_length;
			}run;
		};
	};

	void printOp(Operation op);
	FileHeader readHeader(FileIO::ByteBuffer& buffer);
	ChunkOpType typeAtIndex(const FileIO::ByteBuffer& buffer, Int64 byte_index);
	Operation extractOperation(FileIO::ByteBuffer& buffer);
	void iterateFile(FileIO::ByteBuffer& buffer);
};
