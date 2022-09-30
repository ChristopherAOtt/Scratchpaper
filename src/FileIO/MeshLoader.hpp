#pragma once

#include "Parser.hpp"
#include "Tokenizer.hpp"
#include "Meshes.hpp"
#include "Primitives.hpp"

#include <string>

namespace MeshLoader{
	namespace Obj{
		/*
		NOTE: Based on the OBJ file format description given here:
			https://en.wikipedia.org/wiki/Wavefront_.obj_file
		*/

		// WARNING: In the loader, I'm exploiting the fact that 
		// F, S, O, G, and USEMTL appear consecutively. 
		// Be careful before changing the ordering here.
		constexpr Int32 NUM_VALID_DECLARATION_TYPES = 11;
		enum DeclarationType: Uint08{
			TYPE_INVALID = 0,

			TYPE_V      = 1,   // Vertex position
			TYPE_VT     = 2,   // Vertex texture (uv coords)
			TYPE_VN     = 3,   // Vertex normal
			TYPE_VP     = 4,   // Vertex params (Not supported yet)
			TYPE_L      = 5,   // Polyline (Not supported yet)
			TYPE_F      = 6,   // Polygonal face
			TYPE_S      = 7,   // Set smoothing group or change smoothing state
			TYPE_O      = 8,   // Declare an object
			TYPE_G      = 9,   // Declare a group
			TYPE_USEMTL = 10,  // What material to use
			TYPE_MTLLIB = 11,  // Declare use of a material file
		};

		constexpr const char* DECLARATION_TYPE_STRINGS[] = {
			"TYPE_INVALID",
			"TYPE_V",
			"TYPE_VT",
			"TYPE_VN",
			"TYPE_VP",
			"TYPE_L",
			"TYPE_F",
			"TYPE_S",
			"TYPE_O",
			"TYPE_G",
			"TYPE_USEMTL",
			"TYPE_MTLLIB",
		};

		constexpr char DECLARATION_TYPE_CHARS[] = {
			' ',  // TYPE_INVALID
			'V',  // TYPE_V
			'T',  // TYPE_VT
			'N',  // TYPE_VN
			'P',  // TYPE_VP
			'L',  // TYPE_L
			'F',  // TYPE_F
			'S',  // TYPE_S
			'O',  // TYPE_O
			'G',  // TYPE_G
			'U',  // TYPE_USEMTL
			'M',  // TYPE_MTLLIB
		};

		struct Declaration{
			DeclarationType type;
			Range32 token_range;
		};

		struct VertexAssemblage{
			static constexpr Int32 INDEX_POS = 0;
			static constexpr Int32 INDEX_UV = 1;
			static constexpr Int32 INDEX_NORM = 2;
			/*
			Specifies a single vertex of a face.
			*/

			Int32 data_indices[3];
		};

		DeclarationType getType(Token token, const InputBuffer& buffer);
		std::pair<bool, TriangleMesh> loadFromFile(
			std::string filepath);
	};	
};
