#include "MeshLoader.hpp"

MeshLoader::Obj::DeclarationType 
MeshLoader::Obj::getType(Token token, const InputBuffer& buffer){
	/*
	Returns the obj line declaration type for a given token. 
	*/

	if(token.type != TOKEN_IDENTIFIER){
		return TYPE_INVALID;
	}

	struct TextToType{
		char text[7];
		Int32 num_chars;
		DeclarationType resulting_type;
	};

	constexpr TextToType table[] = {
		{"v",      1, TYPE_V},
		{"vt",     2, TYPE_VT},
		{"vn",     2, TYPE_VN},
		{"vp",     2, TYPE_VP},
		{"f",      1, TYPE_F},
		{"s",      1, TYPE_S},
		{"o",      1, TYPE_O},
		{"g",      1, TYPE_G},
		{"l",      1, TYPE_L},
		{"usemtl", 6, TYPE_USEMTL},
		{"mtllib", 6, TYPE_MTLLIB},
	};

	StringSlice slice = token.data.value_string;
	for(auto [text, num_chars, resulting_type] : table){
		if(slice.num_bytes == num_chars){
			bool is_match = true;
			for(int i = 0; i < num_chars; ++i){
				is_match &= (text[i] == buffer.contents[slice.start_byte + i]);
			}

			if(is_match){
				return resulting_type;
			}
		}
	}
	printf("NO MATCH!\n");

	return TYPE_INVALID;
}

std::pair<bool, TriangleMesh> 
MeshLoader::Obj::loadFromFile(std::string filepath){
	/*
	Loads a triangle mesh from an obj file.

	NOTE: Since I can't find a good explanation of the difference between
		groups and objects, I'm going to take Blender's lead and just roll 
		them both under the same label. I don't like using the word "Object" in
		code because it's too "overloaded", so both G and O are both 
		counted as "groups"

	NOTE: I'm forcing all faces in the file to have a consistent format.
		This is not required by the spec, but I'm doing it for the moment
		to get this code finished.

	REFACTOR: "sCaAaRy mOnOlItH nOiSeS eeeeEeeEEeeEEEeEEEEEEEEEEE"
		I've broken this function into "Steps", most of which are 
		self-contained enough to be split off into their own functions.

	TODO: Add support for object and group information
	TODO: Calculate normals if they are missing.
		If smoothing groups are enabled, find all connecting faces in the
		smoothing group and average their normals at that vertex. If not, 
		just calculate the face normal and apply to each vertex of 
		the triangle.

	WARNING: Cannot handle malformed face definitions.
	WARNING: Only supports a very limited subset of OBJ functionality
	*/

	// Have an output defaulted to invalid to return any time something
	// goes wrong.
	std::pair<bool, TriangleMesh> out_pair;
	out_pair.first = false;

	/*
	STEP 1: Convert file to tokens
	*/
	Tokenizer::Settings settings = Tokenizer::Settings::initDefault();
	settings.line_comment_start = "#";
	settings.block_comment_start = "";
	settings.block_comment_end = "";
	settings.continue_identifiers_until_whitespace = true;
	settings.emit_whitespace_tokens = true;
	settings.attempt_reserved_words = false;

	Parser::TokenizedFile file = Parser::tokenizeFile(filepath, settings);
	InputBuffer buffer{file.text};
	
	/*
	STEP 2: Group tokens into ranges for each declaration
	*/
	std::vector<Declaration> declarations;
	Int32 counts[NUM_VALID_DECLARATION_TYPES + 1] = {};
	Int32 max_face_vert_count = 0;
	Int64 read_pos = 0;
	Int64 num_tokens = file.tokens.size();
	Token& curr_token = file.tokens[read_pos];
	while(read_pos < num_tokens){
		// Eat newlines and whitespace, and quit if we're done
		bool should_consume = true;
		while(should_consume){
			curr_token = file.tokens[read_pos++];
			
			should_consume = false;
			should_consume |= curr_token.type == TOKEN_NEWLINE;
			should_consume |= curr_token.type == TOKEN_WHITESPACE;
			should_consume &= read_pos < num_tokens;
		}
		if(curr_token.type == TOKEN_STREAM_END){
			break;
		}

		// Every line in an obj should start with one of the known declaration
		// type strings.
		DeclarationType type = getType(curr_token, buffer);
		++counts[type];
		if(type == TYPE_INVALID){
			printf("ERROR: Invalid OBJ value starting at token : ");
			Tokenization::printToken(curr_token, &buffer); printf("\n");
			return out_pair;
		}

		// Find the end of the line
		Int32 range_start = read_pos; // After every id is a whitespace
		Int32 num_data_tokens = 0;
		do{
			curr_token = file.tokens[read_pos++];

			TokenType ttype = curr_token.type;
			num_data_tokens += (ttype == TOKEN_INTEGER || ttype == TOKEN_REAL);
		}while(curr_token.type != TOKEN_NEWLINE && read_pos < num_tokens);
		
		// Finalize the declaration range
		Int32 range_end = read_pos - 1;
		Int32 num_tokens_in_range = range_end - range_start;
		Declaration new_declaration = {
			.type=type, 
			.token_range={range_start, num_tokens_in_range}
		};
		declarations.push_back(new_declaration);

		// Bail out if there was an error. That way we know it's safe to
		// construct vectors from these data in the following steps.
		if((type == TYPE_V || type == TYPE_VN) && num_data_tokens != 3){
			// Check the 3-vectors
			printf("ERROR: Declaration type %s had %i values instead of 3\n",
				DECLARATION_TYPE_STRINGS[type], num_data_tokens);
			return out_pair;
		}else if(type == TYPE_VT && num_data_tokens != 2){
			// Check the 2-vectors
			printf("ERROR: Declaration type %s had %i values instead of 2\n",
				DECLARATION_TYPE_STRINGS[type], num_data_tokens);
			return out_pair;
		}else if(type == TYPE_F){
			// Make sure the face declaration was formatted properly
			if(num_data_tokens < 3){
				printf("ERROR: Face must have >= 3 integer values, got %i\n",
					num_data_tokens);
				return out_pair;
			}

			Int32 num_spacings = 0;  // Equals the number of face verts
			for(int i = range_start; i < range_end; ++i){
				TokenType t_type = file.tokens[i].type;
				num_spacings += (t_type == TOKEN_WHITESPACE);
				if(i == range_start + 1 && t_type != TOKEN_INTEGER){
					printf("ERROR: Face must start with integer token, not :");
					Tokenization::printToken(file.tokens[i], &buffer); 
					printf("\n");
					return out_pair;
				}

				if(t_type != TOKEN_FORWARD_SLASH && t_type != TOKEN_INTEGER &&
					t_type != TOKEN_WHITESPACE){
					printf("ERROR: Invalid token in face declaration:");
					Tokenization::printToken(file.tokens[i], &buffer); 
					printf("\n");
					return out_pair;
				}
			}
			if(num_spacings > max_face_vert_count){
				max_face_vert_count = num_spacings;
			}
		}
	}

	// Quick summary
	if(false){
		printf("Finished grouping tokens. Type counts:\n");
		for(int i = 0; i < NUM_VALID_DECLARATION_TYPES + 1; ++i){
			printf("\t%s: %i\n", DECLARATION_TYPE_STRINGS[i], counts[i]);
		}
		printf("\n");
	}

	/*
	STEP 3: Init structs for vector types (V, VT, VN)

	WARNING: Unsafely assumes that normals are unit normals.
	*/
	std::vector<FVec3> vertex_positions;
	std::vector<FVec2> vertex_uvs;
	std::vector<FVec3> vertex_normals;
	vertex_positions.reserve(counts[TYPE_V]);
	vertex_uvs.reserve(counts[TYPE_VT]);
	vertex_normals.reserve(counts[TYPE_VN]);
	for(Declaration& decl : declarations){
		// We're only initializing data for positions, uvs, and normals
		DeclarationType type = decl.type;
		if(!(type == TYPE_V || type == TYPE_VT || type == TYPE_VN)){
			continue;
		}

		Range32& range = decl.token_range;
		if(range.extent % 2 != 0){
			printf("ERROR: Invalid token range for vector {%i - %i}\n",
				range.origin, range.origin + range.extent);
			return out_pair;
		}

		// Init vector value from tokens. Pattern will be alternating
		// SPACE|REAL|SPACE|REAL with 2 or 3 repeats depending on vector type.
		float vec_data[3];
		for(int i = 0; i < range.extent; i += 2){
			Token& token = file.tokens[range.origin + i + 1];
			if(token.type == TOKEN_REAL){
				vec_data[i / 2] = token.data.value_real;
			}else if(token.type == TOKEN_INTEGER){
				vec_data[i / 2] = (float) token.data.value_int;
			}else{
				printf("ERROR: Invalid token type in vector: ");
				Tokenization::printToken(token, &buffer); printf("\n");
				return out_pair;
			}
		}

		// Convert to relevant vector length and save to vector by type.
		void* general_ptr = (void*) &vec_data[0];
		if(type == TYPE_V){
			FVec3* typed_vec_ptr = (FVec3*) general_ptr;
			vertex_positions.push_back(*typed_vec_ptr);
		}else if(type == TYPE_VT){
			FVec2* typed_vec_ptr = (FVec2*) general_ptr;
			vertex_uvs.push_back(*typed_vec_ptr);
		}else{
			FVec3* typed_vec_ptr = (FVec3*) general_ptr;
			vertex_normals.push_back(*typed_vec_ptr);
		}
	}
	Int32 data_array_sizes[3] = {
		(Int32) vertex_positions.size(),
		(Int32) vertex_uvs.size(),
		(Int32) vertex_normals.size()
	};
	const char* data_array_names[3] = {
		"VERTEX_POSITIONS",
		"VERTEX_UVS",
		"VERTEX_NORMALS",
	};

	/*
	STEP 4: Start generating faces.

	There are four possible formats for a vertex assemblage, each with
	a unique number of tokens.
		Format A: Positions           |P ...      | #Tokens 1
		Format B: Positions + UVs     |P/U ...    | #Tokens 3
		Format C: Positions + Normals |P//N ...   | #Tokens 4
		Format D: Everything          |P/U/N ...  | #Tokens 5
	There are >= 3 vertices for each face. If the number is greater
	than 3, the face is split into a triangle fan so that all output
	faces are triangles.

	*/

	// Required types based on token count. If these are not met the file
	// is considered corrupt and the loading process fails.
	std::vector<TokenType> REQUIRED_TYPE_VECTORS[] = {
		{
			// Placeholder
		},
		{	// Format A
			TOKEN_INTEGER
		},
		{
			// Placeholder
		},
		{	// Format B
			TOKEN_INTEGER, 
			TOKEN_FORWARD_SLASH, 
			TOKEN_INTEGER
		},
		{	// Format C
			TOKEN_INTEGER, 
			TOKEN_FORWARD_SLASH, 
			TOKEN_FORWARD_SLASH, 
			TOKEN_INTEGER
		},
		{	// Format D
			TOKEN_INTEGER, 
			TOKEN_FORWARD_SLASH, 
			TOKEN_INTEGER, 
			TOKEN_FORWARD_SLASH, 
			TOKEN_INTEGER
		},
	};

	struct TriangleFace{
		VertexAssemblage vertices[3];
	};

	struct TokenNamedRange{
		/*
		Converted nearly 1:1 into a TriangleMesh::NamedRange later. This
		is just to avoid excessive std::string allocations
		*/

		Int32 name_token_index;
		Range32 range;
	};

	Int32 curr_face_index = 0;
	constexpr Int32 TYPE_TO_INDEX_ADJUSTMENT = TYPE_S;
	TokenNamedRange active_ranges[4] = {
		{-1, {0, 0}},  // S
		{-1, {0, 0}},  // O
		{-1, {0, 0}},  // G
		{-1, {0, 0}},  // USEMTL
	};
	std::vector<TokenNamedRange> parsed_ranges[4] = {};
	parsed_ranges[0].reserve(counts[TYPE_S]);
	parsed_ranges[0].reserve(counts[TYPE_O]);
	parsed_ranges[0].reserve(counts[TYPE_G]);
	parsed_ranges[0].reserve(counts[TYPE_USEMTL]);

	VertexAssemblage default_v = {-1, -1, -1};
	std::vector<VertexAssemblage> face_verts(max_face_vert_count, default_v);
	std::vector<TriangleFace> triangulated_faces;
	bool is_first_face = true;
	Int32 num_tokens_per_vertex = 0;
	std::vector<TokenType>& required_types = REQUIRED_TYPE_VECTORS[0];
	for(Declaration& decl : declarations){
		bool is_ranged_type = decl.type >= TYPE_S && decl.type <= TYPE_USEMTL;
		if(decl.type != TYPE_F && !is_ranged_type){
			continue;
		}

		// STEP: If this declaration isn't a face, it's one of the four 
		// different types of ranges
		if(is_ranged_type){
			if(decl.token_range.extent != 2){
				// Some of the test files have malformed lines with just a 
				// starting string and nothing else. IE: 
				// "g\n", "usemtl\n", etc
				continue;
			}

			if(false){
				Int32 start_index = decl.token_range.origin - 1;
				printf("New declaration @%05d:", start_index);
				for(int i = 0; i < decl.token_range.extent + 1; ++i){
					Token t = file.tokens[start_index + i];
					Tokenization::printToken(t, &buffer); 
				}
				printf("\n");
			}

			// Remove old range if applicable, add new afterwards
			Int32 type_index = decl.type - TYPE_TO_INDEX_ADJUSTMENT;
			TokenNamedRange& curr_range = active_ranges[type_index];
			if(curr_range.name_token_index != -1){
				Int32 num_faces = curr_face_index - curr_range.range.origin;
				curr_range.range.extent = num_faces;
				
				parsed_ranges[type_index].push_back(curr_range);
			}

			TokenNamedRange new_range = {
				// First token in range should be whitespace
				.name_token_index=decl.token_range.origin + 1,
				.range={
					.origin=curr_face_index,
					.extent=-1,
				}
			};
			curr_range = new_range;
			continue;
		}

		// STEP: Determine the format. This loader will demand consistency
		// throughout the entire file.
		if(is_first_face){
			is_first_face = false;

			// Find size of first vertex, space-to-space
			Int32 i = 1;  // First token in declaration is a space.
			while(file.tokens[decl.token_range.origin + i].type != 
				TOKEN_WHITESPACE){

				++i;
			}
			num_tokens_per_vertex = i - 1;

			// Make sure the number is valid before proceeding with the format
			if(num_tokens_per_vertex > 5 || num_tokens_per_vertex == 2 || 
				num_tokens_per_vertex == 0){
				printf("ERROR: Face vertex had wrong number of elements!\n");
				return out_pair;
			}
			required_types = REQUIRED_TYPE_VECTORS[num_tokens_per_vertex];
		}

		// Create all vertex assemblages for the current face, while making
		// sure they all follow the same format. If not, call it quits.
		Int32 vertex_stride = num_tokens_per_vertex + 1;  // Counts extra space
		Int32 num_vertices = 0;
		Int32 vert_write_index = 0;
		Int32 token_i = 1;  // Token index. Skip leading space
		while(token_i < decl.token_range.extent){
			VertexAssemblage new_vertex = default_v;
			for(Int32 vert_i = 0; vert_i < num_tokens_per_vertex; ++vert_i){
				Int32 read_index = decl.token_range.origin + token_i + vert_i;
				Token& curr_token = file.tokens[read_index];
				if(curr_token.type != required_types[vert_i]){
					printf("ERROR: Inconsistent face format!\n");
					return out_pair;
				}

				if(required_types[vert_i] == TOKEN_INTEGER){
					// Convert from 1 to 0 indexing.
					Int32 data_index = curr_token.data.value_int - 1;
					if(data_index >= data_array_sizes[vert_write_index]){
						printf(
							"ERROR: Index %i is too large for data array '%s'"
							", which has a size of %i\n",
							data_index, 
							data_array_names[vert_write_index],
							data_array_sizes[vert_write_index]);
						return out_pair;
					}

					new_vertex.data_indices[vert_write_index] = data_index;
				}else if(required_types[vert_i] == '/'){
					++vert_write_index;
				}
			}
			face_verts[num_vertices] = new_vertex;
			token_i += vertex_stride;
			vert_write_index = 0;
			++num_vertices;
		}
		
		// STEP: Triangulate the faces
		Int32 num_triangles = num_vertices - 2;
		curr_face_index += num_triangles;
		for(Int32 i = 0; i < num_triangles; ++i){
			TriangleFace new_triangle = {
				face_verts[0], face_verts[i + 1], face_verts[i + 2]
			};

			triangulated_faces.push_back(new_triangle);
		}
	}

	// Flush all remaining ranges
	for(int i = 0; i < 4; ++i){
		TokenNamedRange& last_range = active_ranges[i];
		if(last_range.name_token_index == -1){
			continue;
		}

		last_range.range.extent = curr_face_index - last_range.range.origin;
		parsed_ranges[i].push_back(last_range);
	}

	/*
	STEP 5: Generate mesh data. For missing index information, generate
	reasonable defaults

	WARNING: Incomplete. 
	*/

	constexpr Int32 NUM_VERTICES_PER_TRIANGLE = 3;
	std::vector<TriangleMesh::Triangle>& triangles = out_pair.second.triangles;
	triangles.reserve(triangulated_faces.size());
	for(TriangleFace& face : triangulated_faces){
		TriangleMesh::Triangle new_triangle;
		for(int i = 0; i < NUM_VERTICES_PER_TRIANGLE; ++i){
			VertexAssemblage& va = face.vertices[i];

			// Handle Positions
			Int32 index_pos = va.data_indices[VertexAssemblage::INDEX_POS];
			if(index_pos != -1){
				new_triangle.positions[i] = vertex_positions[index_pos];
			}else{
				assert(false);
			}

			// Handle UVs
			Int32 index_uv = va.data_indices[VertexAssemblage::INDEX_UV];
			if(index_uv != -1){
				new_triangle.uvs[i] = vertex_uvs[index_uv];
			}else{
				// TODO: Box mapping
				new_triangle.uvs[i] = {(float) i, (float) i}; 
			}

			// Handle Normals
			Int32 index_norm = va.data_indices[VertexAssemblage::INDEX_NORM];
			if(index_norm != -1){
				new_triangle.normals[i] = vertex_normals[index_norm];
			}else{
				// TODO: Calculate from geometry.
				new_triangle.normals[i] = {0, 0, 1};
			}
		}

		triangles.push_back(new_triangle);
	}

	/*
	STEP 6: Output ranges. 
	WARNING: At the moment only O, G, and USEMTL are processed, and O gets
		rolled into G without distinction
	TODO: Add other types

	*/

	std::vector<TriangleMesh::NamedRange>& group_ranges = 
		out_pair.second.group_ranges;
	std::vector<TriangleMesh::NamedRange>& material_ranges = 
		out_pair.second.material_ranges;
	group_ranges.reserve(counts[TYPE_G] + counts[TYPE_O]);
	material_ranges.reserve(counts[TYPE_USEMTL]);
	for(Int32 i = 1; i < 4; ++i){
		for(TokenNamedRange tnr : parsed_ranges[i]){
			Token token = file.tokens[tnr.name_token_index];
			std::string text = tokenText(token, &buffer);
			if(i == 3){
				material_ranges.push_back({text, tnr.range});
			}else{
				group_ranges.push_back({text, tnr.range});
			}
		}
	}

	if(false){
		printf("Done. Ended with %li triangle faces\n", triangles.size());
		printf("\t%li materials\n", material_ranges.size());
		printf("\t%li groups\n", group_ranges.size());
	}

	out_pair.first = true;
	return out_pair;
}

