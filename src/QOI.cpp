#include "QOI.hpp"


Qoi::FileHeader Qoi::readHeader(FileIO::ByteBuffer& buffer){
	/*
	Reads the header out the front of a buffer.
	*/

	FileHeader header;
	buffer.setCursorIndex(0);
	buffer.copyFromCursor(sizeof(FileHeader), &header, true);
	header.width = ntohl(header.width);
	header.height = ntohl(header.height);

	printf("Loaded header for (%ix%i) image\n", header.width, header.height);

	return header;
}

Qoi::ChunkOpType Qoi::typeAtIndex(const FileIO::ByteBuffer& buffer, Int64 index){
	/*

	*/

	Bytes1 tag_byte = buffer.readBytes1(index);
	for(int i = 0; i < NUM_CHUNK_TYPES; ++i){
		auto [chunk_type, mask, tag_value] = extraction_arr[i];
		Bytes1 masked_result = tag_byte & mask;
		if(masked_result == tag_value){
			return chunk_type;
		}
	}

	return CHUNK_OP_INVALID;
}

Qoi::Operation Qoi::extractOperation(FileIO::ByteBuffer& buffer){
	/*
	Returns an operation struct from the current cursor position.
	*/

	Int64 curr_index = buffer.getCursorIndex();
	ChunkOpType type = typeAtIndex(buffer, curr_index);
	Operation operation{.type=type};
	if(type == CHUNK_OP_RGB){
		for(int i = 0; i < NUM_RGB_CHANNELS; ++i){
			operation.rgb.colors[i] = buffer.readBytes1(curr_index + i + 1);
		}
	}
	else if(type == CHUNK_OP_RGBA){
		for(int i = 0; i < NUM_RGBA_CHANNELS; ++i){
			operation.rgba.colors[i] = buffer.readBytes1(curr_index + i + 1);
		}
	}
	else if(type == CHUNK_OP_INDEX){
		Bytes1 data = buffer.readBytes1(curr_index);
		operation.index.index = data & ~MASK_SMALL_OP;
	}
	else if(type == CHUNK_OP_DIFF){
		Bytes1 data = buffer.readBytes1(curr_index);
		constexpr int DIFF_BIAS = -2;
		for(int i = 0; i < NUM_RGB_CHANNELS; ++i){
			Int8 value = (data & (0b0011'0000 >> i));
			operation.diff.delta_rgb[i] = value + DIFF_BIAS;
		}
	}
	else if(type == CHUNK_OP_LUMA){
		constexpr Bytes2 BRG_MASK_ARR[] = {
			0b0000'0000'0000'1111,
			0b0000'0000'1111'0000,
			0b0011'1111'0000'0000,
		};
		Bytes2 data = buffer.readBytes2(curr_index);
		for(int i = 0; i < 3; ++i){
			Uint16 masked_data = data & BRG_MASK_ARR[i];
			operation.luma.delta_rgb[i] = masked_data >> (4 * i);
		}
	}
	else if(type == CHUNK_OP_RUN){
		Bytes1 data = buffer.readBytes1(curr_index);
		operation.run.run_length = data & ~MASK_SMALL_OP;
	}

	return operation;
}

void Qoi::printOp(Operation op){
	ChunkOpType& type = op.type;
	printf("\t%s|", OP_STRINGS[type]);
	
	if(type == CHUNK_OP_RGB){
		printf("{");
		for(int i = 0; i < NUM_RGB_CHANNELS; ++i){
			printf("%i, ", op.rgb.colors[i]);
		}
		printf("}");
	}
	else if(type == CHUNK_OP_RGBA){
		printf("{");
		for(int i = 0; i < NUM_RGBA_CHANNELS; ++i){
			printf("%i, ", op.rgba.colors[i]);
		}
		printf("}");	
	}
	else if(type == CHUNK_OP_INDEX){
		printf("Index %i", op.index.index);
	}
	else if(type == CHUNK_OP_DIFF){
		printf("{");
		for(int i = 0; i < NUM_RGB_CHANNELS; ++i){
			printf("%i, ", op.diff.delta_rgb[i]);
		}
		printf("}");
	}
	else if(type == CHUNK_OP_LUMA){
		printf("{");
		for(int i = 0; i < NUM_RGB_CHANNELS; ++i){
			printf("%i, ", op.luma.delta_rgb[i]);
		}
		printf("}");
	}
	else if(type == CHUNK_OP_RUN){
		printf("RunLength %i", op.run.run_length);
	}
	else if(type == CHUNK_OP_INVALID){
		assert(false);
	}
	printf("\n");
}

void Qoi::iterateFile(FileIO::ByteBuffer& buffer){
	printf("Started iteration...\n");
	Int64 file_size = buffer.size();

	FileHeader header = readHeader(buffer);

	Int64 curr_index = sizeof(FileHeader);
	while(curr_index < file_size){
		printf("Byte %li|", curr_index);
		buffer.setCursorIndex(curr_index);

		Operation op = extractOperation(buffer);
		printOp(op);
		
		curr_index += OP_CHUNK_BYTE_SIZES[op.type];
	}
	printf("Finished iteration\n");
}



