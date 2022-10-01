#pragma once

#include "WorldState.hpp"
#include "MultiresGrid.hpp"
#include "ResourceManager.hpp"
#include "KDTree.hpp"
#include "RayTracing.hpp"
#include "MeshLoader.hpp"

#include "FileIO.hpp"

class SimCache{
	/*
	Wrapper around the WorldState used by the engine to cache temporary, runtime
	data and structures. Anything meant to persist after shutdown is stored in
	the WorldState structure.
	*/

	public:
		SimCache();
		~SimCache();

		void generateAccelerationStructures(VoxelKDTree::BuildSettings settings);

		//RayIntersection traceRay(Ray ray);
		//void traceImage(Camera& camera, Rendering::ImageConfig config);
		//void findIntersectionDiscrepancies(int num_iterations);

	public:
		WorldState* m_reference_world;
		ResourceManager* m_resource_manager;
		VoxelKDTree::TreeData* m_kd_tree_ptr;

		MultiresGrid m_chunk_lod_meshes;
};
