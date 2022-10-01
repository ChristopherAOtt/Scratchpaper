#include "FileIO.hpp"

//-------------------------------------------------------------------------------------------------
// Loading from various files
//-------------------------------------------------------------------------------------------------
std::pair<bool, Settings> FileIO::loadSettingsFromFile(std::string filepath){
	/*
	Returns a pair of <is_valid, Settings> from a config file.
	*/

	std::pair<bool, Settings> out_pair;
	out_pair.first = false;

	auto [is_tokenization_valid, tokenized_file] = Parser::tokenizeFile(filepath);
	if(!is_tokenization_valid){
		return out_pair;
	}
	InputBuffer buffer{tokenized_file.text};

	// If the file could be parsed correctly, start turning tokens into
	// PODstructs and strings as necessary.
	auto [is_settings_valid, result] = jankParseSettingsFile(tokenized_file);
	if(!is_settings_valid){
		return out_pair;
	}else{
		Settings& loaded_settings = out_pair.second;
		for(auto [namespace_name, namespace_data] : result.namespaces){
			Settings::Namespace new_namespace;
			new_namespace.contained_namespaces = namespace_data.namespaces;
			
			// Handle KVPairs
			for(auto [key_token, value_token_list] : namespace_data.pairs){
				PODVariant value_variant;
				if(value_token_list.size() == 1){
					/*
					The value is a standalone type that can be deduced from a single
					token.
					*/

					std::string token_text;
					Token value_token = value_token_list[0];
					switch(value_token.type){
						case TOKEN_BOOLEAN:
							value_variant = PODVariant::init(value_token.data.value_bool);
							break;
						case TOKEN_INTEGER:
							value_variant = PODVariant::init((int) value_token.data.value_int);
							break;
						case TOKEN_REAL:
							value_variant = PODVariant::init((float) value_token.data.value_real);
							break;
						case TOKEN_STRING:
						case TOKEN_IDENTIFIER:
							token_text = tokenText(value_token, &buffer);
							value_variant = PODVariant::init(PODString::init(&token_text[0]));
							break;
						default:
							printf("ERROR: Unrecognized token type!\n");
							return out_pair;
					}
				}else if(value_token_list.size() < 4){
					/*
					The value is a vector
					*/
					Int32 num_int = 0;
					Int32 num_real = 0;
					Int32 num_other = 0;
					for(Token token : value_token_list){
						if(token.type == TOKEN_INTEGER){
							num_int += 1;
						}else if(token.type == TOKEN_REAL){
							num_real += 1;
						}else{
							num_other += 1;
						}
					}

					if(num_other > 0){
						printf("ERROR: Vectors can only have Real or Int components!\n");
						return out_pair;
					}else{
						// NOTE: I tried doing this a "smarter" way and the void ptr
						// conversions ended up creating super messy code.
						Int32 vector_len = (Int32) value_token_list.size();
						bool is_vec3 = vector_len == 3;
						bool is_int_vector = num_int == vector_len;
						if(is_int_vector){
							// Int vector
							if(is_vec3){
								IVec3 vec;
								for(int i = 0; i < 3; ++i){
									vec[i] = value_token_list[i].data.value_int;
								}
								value_variant = PODVariant::init(vec);
							}else{
								IVec2 vec;
								for(int i = 0; i < 2; ++i){
									vec[i] = value_token_list[i].data.value_int;
								}
								value_variant = PODVariant::init(vec);
							}
						}else{
							// Float vector
							if(is_vec3){
								FVec3 vec;
								for(int i = 0; i < 3; ++i){
									vec[i] = value_token_list[i].data.value_real;
								}
								value_variant = PODVariant::init(vec);
							}else{
								FVec2 vec;
								for(int i = 0; i < 2; ++i){
									vec[i] = value_token_list[i].data.value_real;
								}
								value_variant = PODVariant::init(vec);
							}
						}
					}
				}else{
					printf("ERROR: Value token list of length %li wasn't valid.\n", 
						value_token_list.size());
					return out_pair;
				}

				new_namespace.dict[tokenText(key_token, &buffer)] = value_variant;
			}

			loaded_settings.update(namespace_name, new_namespace);
		}
	}

	out_pair.first = true;
	return out_pair;
}

std::vector<FileIO::ResourceDeclaration> FileIO::loadResourceDeclarations(std::string filepath){
	/*
	Parses a declarations file and returns a list of all declarations that
	were made.
	*/

	std::vector<FileIO::ResourceDeclaration> declarations;
	
	constexpr int NUM_REQUIRED_DECL_TYPES = 8;
	constexpr TokenType REQUIRED_DECL_TYPES[] = {
		TOKEN_OPEN_SQUARE_BRACKET,
		TOKEN_IDENTIFIER,
		TOKEN_VERTICAL_BAR,
		TOKEN_IDENTIFIER,
		TOKEN_CLOSE_SQUARE_BRACKET,
		TOKEN_EQUALS_SIGN,
		TOKEN_STRING,
		TOKEN_SEMICOLON,
	};

	const std::unordered_map<std::string, ResourceType> type_map = {
		{"Font",               RESOURCE_FONT},
		{"Texture",            RESOURCE_TEXTURE},
		{"RigidTriangleMesh",  RESOURCE_RIGID_TRIANGLE_MESH},
		{"RiggedSkeletalMesh", RESOURCE_RIGGED_SKELETAL_MESH},
	};

	auto [is_valid, tokenized_file] = Parser::tokenizeFile(filepath);
	if(!is_valid){
		return declarations;
	}
	InputBuffer buffer{tokenized_file.text};

	Int64 read_pos = 0;
	std::vector<Token>& tokens = tokenized_file.tokens;
	Token& curr_token = tokens[0];
	while(read_pos < (Int64) tokens.size()){
		// Consume leading newlines
		while(curr_token.type == TOKEN_NEWLINE){
			curr_token = tokens[read_pos++];
		}

		// Parse a new declaration
		Token token_array[NUM_REQUIRED_DECL_TYPES];
		for(int i = 0; i < NUM_REQUIRED_DECL_TYPES; ++i){
			if(curr_token.type != REQUIRED_DECL_TYPES[i]){
				printf("ERROR: Invalid declaration format!\n");
				return {};
			}else{
				token_array[i] = curr_token;
				curr_token = tokens[read_pos++];
			}
		}

		// Create the declaration object and add it to the list
		std::string type_string = tokenText(token_array[1], &buffer);
		std::string internal_name = tokenText(token_array[3], &buffer);
		std::string source_filepath = tokenText(token_array[6], &buffer);
		ResourceType type_enum;
		auto iter = type_map.find(type_string);
		if(iter != type_map.end()){
			type_enum = iter->second;
		}else{
			printf("ERROR: Type '%s' not recognized!\n", type_string.c_str());
			return {};
		}
		declarations.push_back({internal_name, type_enum, source_filepath});

		curr_token = tokens[read_pos++];
		if(curr_token.type == TOKEN_STREAM_END){
			break;
		}
	}

	return declarations;
}

//-------------------------------------------------------------------------------------------------
// Byte Buffer
//-------------------------------------------------------------------------------------------------
FileIO::ByteBuffer::ByteBuffer(Int64 num_bytes){
	allocateBuffer(num_bytes);
	m_is_valid = true;
}

FileIO::ByteBuffer::ByteBuffer(std::string filepath){
	/*
	ATTRIBUTION: Getting number of file bytes
		https://stackoverflow.com/a/18816228
	*/

	Int64 num_file_bytes = 0;
	std::ifstream file(&filepath[0], std::ios::binary | std::ios::ate);
	if(file.good()){
		num_file_bytes = file.tellg();
		file.seekg(0, std::ios::beg);

		assert(num_file_bytes < ByteBuffer::MAX_ALLOWED_BUFFER_SIZE);
		allocateBuffer(num_file_bytes);
		if(file.read((char*) m_buffer_ptr, num_file_bytes)){
			m_is_valid = true;
		}else{
			m_is_valid = false;
			assert(m_is_valid);
		}
	}
	m_num_bytes = num_file_bytes;
}

FileIO::ByteBuffer::~ByteBuffer(){
	free(m_buffer_ptr);
}

Int64 FileIO::ByteBuffer::size() const{
	return m_num_bytes;
}

void FileIO::ByteBuffer::setCursorIndex(Int64 byte_index){
	assert(byte_index >= 0 && byte_index < m_num_bytes);
	m_cursor_index = byte_index;
}

Int64 FileIO::ByteBuffer::getCursorIndex(){
	return m_cursor_index;
}

void FileIO::ByteBuffer::writeToCursor(void* source_data_ptr, Int64 num_write_bytes){
	/*
	WARNING: Not fully tested.
	*/

	assertSafeRange(m_cursor_index, num_write_bytes);
	memcpy(&m_buffer_ptr[m_cursor_index], source_data_ptr, num_write_bytes);
	m_cursor_index += num_write_bytes;
}

void FileIO::ByteBuffer::copyFromCursor(Int64 num_read_bytes, void* target_arr, 
	bool should_advance){
	/*
	REFACTOR: Use an iterator instead of this nonsense
	*/

	assertSafeRange(m_cursor_index, num_read_bytes);
	memcpy(target_arr, &m_buffer_ptr[m_cursor_index], num_read_bytes);

	if(should_advance){
		m_cursor_index += num_read_bytes;
	}
}

Bytes1 FileIO::ByteBuffer::readBytes1(Int64 index) const{
	assertSafeRange(index, 1);
	return m_buffer_ptr[index];
}

Bytes2 FileIO::ByteBuffer::readBytes2(Int64 index) const{
	assertSafeRange(index, 2);
	Bytes2* cast_ptr = (Bytes2*)&m_buffer_ptr[index];
	return cast_ptr[0];
}

Bytes4 FileIO::ByteBuffer::readBytes4(Int64 index) const{
	assertSafeRange(index, 4);
	Bytes4* cast_ptr = (Bytes4*)&m_buffer_ptr[index];
	return cast_ptr[0];
}

void FileIO::ByteBuffer::assertSafeRange(Int64 read_index, Int64 num_bytes) const{
	assert(read_index >= 0);
	assert(num_bytes > 0);
	assert(read_index + num_bytes <= m_num_bytes);
}

void FileIO::ByteBuffer::allocateBuffer(Int64 num_bytes){
	m_buffer_ptr = (Bytes1*) malloc(num_bytes);
}


//-------------------------------------------------------------------------------------------------
// Helper Functions
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
// One-off-functions
//-------------------------------------------------------------------------------------------------
std::unordered_map<std::string, std::string> loadShadersFromFile(std::string filepath){
	/*
	Given a filepath, return a map of SHADER_TYPE --> SHADER_TEXT. If the target file has no
	BLOCK_START_LABEL, the map should be empty. Any content appearing before the 
	first BLOCK_START_LABEL is slop that shouldn't be included in the map. A BLOCK_START_LABEL 
	must appear at the beginning of the line, and the rest of the line will be used
	as the name of the label.

	REFACTOR: Code is currently very sloppy, inefficient, and wasteful.
	WARNING: Code makes numerous unsafe assumptions about the structure of the file.
	WARNING: Doesn't handle unicode.
	*/

	std::unordered_map<std::string, std::string> shader_name_to_text_map;

	const std::string BLOCK_START_LABEL = "#DEFINE ";
	const int BLOCK_START_LABEL_SIZE = BLOCK_START_LABEL.length();
	
	std::string current_block_name;
	std::string current_block_contents;
	std::string current_line;

	bool match_previously_found = false;
	std::size_t match_location;
	std::ifstream infile;
	infile.open(filepath);
	while(std::getline(infile, current_line)){
		match_location = current_line.find(BLOCK_START_LABEL);
		if(match_location == 0){
			// We're at the start of a new block. Save the data from the old block then reset the
			// tracking variables.
			if(match_previously_found){
				shader_name_to_text_map[current_block_name] = current_block_contents;
			}
			match_previously_found = true;
			current_block_name = current_line.substr(BLOCK_START_LABEL_SIZE);
			current_block_contents = "";
		}else if(match_previously_found){
			// Just extend the current block
			current_block_contents += current_line + "\n";
		}
	}

	// Push final shader block as long as there was 
	if(match_previously_found){
		shader_name_to_text_map[current_block_name] = current_block_contents;	
	}

	return shader_name_to_text_map;
}

std::unordered_map<std::string, std::string> loadSettingsMap(std::string filepath){
	/*
	Reads a file line-by-line and reads data in the following format:
		key_string:value_string\n

	Returns a map of key->value strings.

	REFACTOR: Code is inefficient.
	WARNING: If multiple identical keys are specified, the latest value will be used.
	WARNING: If multiple ':' characters are specified correct behavior cannot be guaranteed.
	WARNING: Doesn't handle unicode.
	*/

	std::unordered_map<std::string, std::string> kv_pair_map;

	std::string current_line;
	std::string key;
	std::string value;
	std::size_t match_location;

	std::ifstream infile;
	infile.open(filepath);
	if(infile.is_open()){
		while(std::getline(infile, current_line)){
			match_location = current_line.find(":");
			if(match_location != std::string::npos){
				key = current_line.substr(0, match_location);
				value = current_line.substr(match_location + 1);
				kv_pair_map[key] = value;
			}else{
				// This line doesn't fit the format, just ignore it.
				continue;
			}
		}
		printf("FileIO: Loaded %i settings from file '%s'\n", 
			(int)kv_pair_map.size(), &filepath[0]);
	}else{
		printf("FileIO: Could not open file '%s'\n", &filepath[0]);
	}
	
	return kv_pair_map;
}

void saveToPPM(Image& image, std::string filepath){
	/*
	P6 - Uncompressed Binary Color
	P5 - Uncompressed Binary Greyscale

	P3 - ASCII version of P6. Not supported.
	P2 - ASCII version of P5. Not supported.
	*/

	Int32 max_sat = image.maxSaturation();
	IVec2 dimensions = image.dimensions();
	Int64 image_pixels = dimensions.x * dimensions.y;
	Image::PixelType image_type = image.type();

	std::ofstream outfile;
	outfile.open(filepath);
	if(image_type == Image::PIXELTYPE_MONOCHROME){
		outfile << "P5";
	}else{
		outfile << "P6";
	}
	outfile << dimensions.x << " " << dimensions.y << " " << max_sat << "\n";
	
	if(image_type == Image::PIXELTYPE_MONOCHROME){
		for(Int64 p = 0; p < image_pixels; ++p){
			Image::PixelMonochrome& pixel = image.pixelMonochrome(p);
			outfile << pixel.colors[0];
		}
	}else if(image_type == Image::PIXELTYPE_RGB){
		for(Int64 p = 0; p < image_pixels; ++p){
			Image::PixelRGB& pixel = image.pixelRGB(p);
			for(int i = 0; i < 3; ++i){
				outfile << pixel.colors[i];
			}
		}
	}else if(image_type == Image::PIXELTYPE_RGBA){
		printf("WARNING: PPM does not support RGBA. Writing as RGB!\n");
		for(Int64 p = 0; p < image_pixels; ++p){
			Image::PixelRGBA& pixel = image.pixelRGBA(p);
			for(int i = 0; i < 3; ++i){
				outfile << pixel.colors[i];
			}
		}
	}else{
		assert(false);
	}
	
	outfile.close();
	Int64 num_bytes = image_pixels * Image::PIXEL_TYPE_SIZES[image_type];
	printf("Finished writing %li bytes to file '%s'\n", num_bytes, &filepath[0]);
}
