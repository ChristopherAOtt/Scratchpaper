#pragma once

#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "Widgets.hpp"
#include "GLIncludes.hpp"
#include "MeshGenerator.hpp"
#include "MeshingOpenGL.hpp"
#include "MathUtils.hpp"

#include "ResourceManager.hpp"  // TODO: Remove

//-------------------------------------------------------------------------------------------------
// Tracking Data
//-------------------------------------------------------------------------------------------------
// Maps directly to a RenderableBufferMetadata struct.
typedef Uint32 RenderableId;
constexpr RenderableId INVALID_RENDERABLE_ID = 0;

//
struct RenderableBufferMetadata{
	/*
	This is cluster of all the info relating to one indexed mesh
	*/

	// Renderer info
	RenderableId id{INVALID_RENDERABLE_ID};
	MeshRenderType render_type{MESH_INVALID};

	// Buffer info
	GLuint vao{0};
	GLuint vbo{0};
	GLuint ebo{0};
	GLuint num_vertices{0};
	GLuint num_indices{0};
};


//-------------------------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------------------------
class AssetManagerOpenGL{
	public:
		enum AssetType{
			ASSET_INVALID = 0,

			ASSET_WIDGET_MESH,
			ASSET_CHUNK_VOXEL_MESH,
			ASSET_RIGID_TRIANGLE_MESH,
			ASSET_TEXTURE,
		};

	public:
		AssetManagerOpenGL();
		~AssetManagerOpenGL();

		// Set
		RenderableId setRenderableData(ResourceHandle handle, const TriangleMesh& data);
		RenderableId setRenderableData(ResourceHandle handle, TempTexture data);
		RenderableId setRenderableData(CellAddress address, const ChunkVoxelMesh& data);
		RenderableId setRenderableData(std::string group_name, const WidgetMesh& widget_mesh);

		// Get
		RenderableId getId(ResourceHandle handle);
		RenderableId getId(CellAddress addr);
		RenderableId getId(std::string group_name);
		RenderableBufferMetadata getMetadata(RenderableId id) const;

		// Delete
		void deleteRenderable(RenderableId id);

	private:
		RenderableId generateNewId();

		RenderableBufferMetadata initTriangleMesh(const TriangleMesh& triangle_mesh) const;
		RenderableBufferMetadata initWidgetMesh(const WidgetMeshOpengl& widget_mesh) const;
		RenderableBufferMetadata initChunkVoxelMesh(const ChunkVoxelMeshOpengl& chunk_mesh) const;
		
	private:
		RenderableId m_next_id;
		MathUtils::Random::SebVignaSplitmix64 m_id_generator;
		
		std::unordered_map<ResourceHandle, RenderableId, PODHasher> m_handle_to_id_map;
		std::unordered_map<CellAddress, RenderableId, PODHasher> m_cell_addr_to_id_map;
		std::unordered_map<std::string, RenderableId> m_widget_group_name_to_id_map;

		std::unordered_map<RenderableId, AssetType> m_asset_type_map;
		std::unordered_map<RenderableId, RenderableBufferMetadata> m_id_to_buffer_metadata_map;
};



#if 0
class AssetManagerOpenGL{
	/*
	YAGNI: This allocates renderable ids by incrementing a 64-bit number. Theoretically this wastes
		space compared to a smaller id (32, 16?) with a smarter allocation system. 
	*/

	public:
		AssetManagerOpenGL();
		~AssetManagerOpenGL();

		RenderableId initNewRenderable();
		bool eraseRenderable(RenderableId id);
		bool setRenderableData(RenderableId id, const WidgetMesh& widget_mesh);
		bool setRenderableData(RenderableId id, const ChunkVoxelMesh& voxel_mesh);
		// bool setRenderableData(RenderableId id, RESOURCE_TYPE_A);
		// bool setRenderableData(RenderableId id, RESOURCE_TYPE_B);
		// bool setRenderableData(RenderableId id, RESOURCE_TYPE_C);
		RenderableBufferMetadata getMetadata(RenderableId id);

		// A single resource can have multiple renderables.
		bool eraseResource(ResourceId id);
		bool linkRenderableToResource(RenderableId renderable_id, ResourceId resource_id);
		std::vector<RenderableId> getAllLinkedRenderables(ResourceId id);

		// A single chunk can have multiple renderables
		bool eraseCell(CellAddress addr);
		bool linkRenderableToCell(RenderableId renderable_id, CellAddress addr);
		std::vector<RenderableId> getAllLinkedRenderables(CellAddress addr);
		void printSummaryStatsDebugging() const;
		
	private:
		RenderableBufferMetadata initWidgetMesh(const WidgetMeshOpengl& mesh);
		RenderableBufferMetadata initChunkVoxelMesh(const ChunkVoxelMeshOpengl& mesh);
		void clearRenderableBuffer(RenderableId id, bool should_retain_entry);
		RenderableId getNextValidId();
		bool isRenderableInitialized(RenderableId id);

	
	private:
		std::unordered_map<ResourceId, std::vector<RenderableId>> m_resource_to_renderable;
		std::unordered_map<CellAddress, std::vector<RenderableId>, PODHasher> m_addr_to_renderable;
		std::unordered_map<RenderableId, RenderableBufferMetadata> m_renderables;
		std::unordered_set<RenderableId> m_active_renderable_ids;
		RenderableId m_next_available_id;
};
#endif
