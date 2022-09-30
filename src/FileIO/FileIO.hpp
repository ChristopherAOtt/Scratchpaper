#pragma once

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <fstream>  // For shader loading
#include <vector>
#include <arpa/inet.h>  // For byte ordering

#include "Primitives.hpp"
#include "Image.hpp"
#include "Parser.hpp"
#include "Tokenizer.hpp"
#include "Settings.hpp"

/*
GENERAL OVERVIEW:
This file is meant to compartmentalize the code necessary for reading data out of text files.
Right now it uses a very janky string/index/vector strategy to parse the text. This makes piles of
unnecessary copies and makes error handling very difficult to do properly. 

TODO: Change the strategy to a system that uses a token-based parsing strategy. This will make it
easy to report errors via an error field. It will also function similarly to a StringView, avoiding
excessive text copying. 
*/

//-------------------------------------------------------------------------------------------------
// Helper Functions
//-------------------------------------------------------------------------------------------------
namespace FileIO{
	//---------------------------------------------------------------------
	// Misc struct types
	//---------------------------------------------------------------------
	struct ResourceDeclaration{
		/*
		Loaded from resource file. Declares a resource as a string 
		identifier, a type, and a source filepath.
		*/

		std::string name;
		ResourceType type;
		std::string source_filepath;
	};

	// Misc functions
	std::pair<bool, Settings> loadSettingsFromFile(std::string filepath);
	std::vector<ResourceDeclaration> loadResourceDeclarations(std::string filepath);

	namespace SaveFile{
		//---------------------------------------------------------------------
		// General SaveFile structs and definitions
		//---------------------------------------------------------------------
		typedef Uint32 NodeId;
		typedef Uint64 UniqueId;
		
		constexpr const char* MAGIC_VALUE = "VOXELBIN";
		struct Header{
			char magic[8];  // Must == MAGIC_VALUE
			Uint8 version_info[3];
			Uint32 directory_location;
		};

		enum NodeType: Uint32{
			NODE_INVALID = 0,

			// Contains positions of nodes so the user can jump to position
			NODE_DIRECTORY               = 1,

			// Type determined by text label
			NODE_CUSTOM_TYPE             = 2,

			// Arbitrary bookkeeping info
			NODE_INFO                    = 3,

			// Various different types of voxel data. This can include chunks,
			// volumes, and other storage types.
			NODE_VOXEL_DATA              = 4,

			// Saved acceleration structures to avoid recalculating every time.
			NODE_ACCELERATION_STRUCTURES = 5,

			// Currently unused
			NODE_ENTITY_DATA             = 6,
		};

		struct NodeHeader{
			/*
			Must come at the start of every node. 
			*/

			NodeType type;
			NodeId id;
			Uint64 payload_size_bytes;  // Starts counting immediately after the header

			// Only needs to be filled if NODE_CUSTOM_TYPE is specified. Otherwise
			// it's just padding for future-proofing.
			char optional_text_label[64];
		};

		struct TableHeader{
			/*
			Nodes can reserve more space than immediately necessary to avoid
			having to resize every time a value is added. The exact amount
			will vary depending on the type. 
			*/

			constexpr static Int32 VARIABLE_SIZE_ENTRY = -1;

			Int32 entry_size_bytes;  // If -1, entries are of variable size
			Uint32 num_spaces_allocated;
			Uint32 num_spaces_used;
		};

		struct UniqueIdEntry{
			UniqueId id;
			char text_label[64];
		};

		//---------------------------------------------------------------------
		// Node-specific structures
		//---------------------------------------------------------------------
		//---------------------------------------
		// Directory
		//
		// First node in the file. Contains a table of node positions so the
		// reader can jump directly to the relevant info.
		//---------------------------------------
		struct DirectoryHeader{
			/*
			Entries appear in an array immediately after this header
			*/

			TableHeader entry_table_header;
		};

		struct DirectoryEntry{
			NodeType type;
			NodeId id;
			Uint64 start_byte_index;
		};

		//---------------------------------------
		// VoxelData
		//
		// Various different forms of voxel data
		//---------------------------------------
		struct VoxelDataHeader{
			/*
			The different sections headers follow immediately after this header.
			*/

			TableHeader section_table_header;
		};

		enum VoxelDataSectionType: Uint8{
			SEGMENT_INVALID            = 0,

			// Chunk data is saved without compression
			SEGMENT_RAW_VOXEL_CHUNK    = 1,

			// Instead of specifying data in chunks, specify a volume with 
			// arbitrary dimensions and voxel info afterwards.
			SEGMENT_ARBITRARY_VOLUMES  = 2,

			// TODO: Different compression types
		};

		struct VoxelDataSectionTableEntry{
			/*
			These headers appear in a table after the 
			*/

			VoxelDataSectionType type;
			TableHeader section_entries_header;

			// Section entries don't immediately follow these headers. Instead, they

			Uint64 section_offset; 
		};

		//---------------------------------------
		// Acceleration Structures
		//---------------------------------------
		struct AccelerationHeader{
			/*
			This is followed by a list of AccelerationEntry structs
			*/

			TableHeader allocation_table_header;
		};

		enum AccelerationStructureType: Uint32{
			ACCELERATION_INVALID = 0,

			ACCELERATION_VKDTREE = 1,  // Voxel KD Tree
			ACCELERATION_EKDTREE = 2,  // Entity KD Tree (Not Implemented)
			ACCELERATION_VOCTREE = 3,  // Voxel Octree (Not Implemented)
			ACCELERATION_EOCTREE = 4,  // Entity Octree (Not Implemented)
		};

		struct AccelerationEntry{
			AccelerationStructureType type;

		};
	};

	class ByteBuffer{
		/*
		An awful wrapper class that metastasized over time.
		REFACTOR: Simplify interface 
		*/
		
		// Felt temporary, might remove later.
		static constexpr Int64 MAX_ALLOWED_BUFFER_SIZE = 2 << 25;

		public:
			ByteBuffer(std::string filepath);
			ByteBuffer(Int64 num_bytes);
			~ByteBuffer();

			Int64 size() const;
			void setCursorIndex(Int64 byte_index);
			Int64 getCursorIndex();
			void writeToCursor(void* source_data_ptr, Int64 num_write_bytes);
			void copyFromCursor(Int64 num_read_bytes, void* target_arr, 
				bool should_advance=true);
			Bytes1 readBytes1(Int64 index) const;
			Bytes2 readBytes2(Int64 index) const;
			Bytes4 readBytes4(Int64 index) const;

		private:
			void assertSafeRange(Int64 index, Int64 num_bytes) const;
			void allocateBuffer(Int64 num_bytes);

		private:
			Int64 m_num_bytes{0};
			Int64 m_cursor_index{0};
			Bytes1* m_buffer_ptr{NULL};
			bool m_is_valid;
	};

	inline Int32 flipEndian(Int32 input){
		return ntohl((Uint32) input);
	}

	inline Uint32 flipEndian(Uint32 input){
		return ntohl(input);
	}

	inline Int64 flipEndian(Int64 input){
		return ntohl((Uint64) input);
	}

	inline Uint64 flipEndian(Uint64 input){
		return ntohl(input);
	}

	inline IVec3 flipEndian(IVec3 vec){
		return {flipEndian(vec.x), flipEndian(vec.y), flipEndian(vec.z)};
	}


	namespace Utils{

	};
};

//-------------------------------------------------------------------------------------------------
// One-off functions
//-------------------------------------------------------------------------------------------------
std::unordered_map<std::string, std::string> loadShadersFromFile(std::string filepath);
std::unordered_map<std::string, std::string> loadSettingsMap(std::string filepath);
void saveToPPM(Image& image, std::string filepath);
