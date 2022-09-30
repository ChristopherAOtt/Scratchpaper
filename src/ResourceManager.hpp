#pragma once

#include "Types.hpp"
#include "Primitives.hpp"
#include "Meshes.hpp"
#include "MathUtils.hpp"

#include <unordered_set>
#include <unordered_map>


struct TempTexture{
	int placeholder_type;
};

class ResourceManager{
	private:
		struct ResourceMetadata{
			ResourceHandle handle;
			std::string name;
		};

		static constexpr Uint64 ARBITRARY_MAX_ACTIVE_IDS = 50000;
		static constexpr ResourceHandle INVALID_RESOURCE_HANDLE = {
			RESOURCE_INVALID, INVALID_RESOURCE_ID
		};

	public:
		ResourceManager();
		~ResourceManager();

		ResourceHandle handleByName(std::string name) const;

		ResourceHandle addResource(std::string name, TriangleMesh data);
		ResourceHandle addResource(std::string name, TempTexture data);

		TriangleMesh getDataTriangleMesh(ResourceHandle handle);
		TempTexture getDataTexture(ResourceHandle handle);

	private:
		ResourceId generateNewId();

	private:
		// Different resource type maps
		std::unordered_map<ResourceId, TriangleMesh> m_rigid_triangle_mesh_map;
		std::unordered_map<ResourceId, TempTexture> m_texture_map;
		
		// Id and metadata handling
		std::unordered_set<ResourceId> m_active_ids;
		std::unordered_map<ResourceHandle, ResourceMetadata, PODHasher> m_metadata_map;
		MathUtils::Random::SebVignaSplitmix64 m_id_generator;
};
