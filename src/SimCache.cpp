#include "SimCache.hpp"

//-------------------------------------------------------------------------------------------------
// SimCache
//-------------------------------------------------------------------------------------------------
SimCache::SimCache(){
	m_reference_world = NULL;
	m_resource_manager = NULL;
	m_kd_tree_ptr = NULL;
}

SimCache::~SimCache(){

}

void SimCache::generateAccelerationStructures(VoxelKDTree::BuildSettings settings){
	/*

	*/

	// WARNING: Hack used for testing. Should be "Generate at runtime" vs "load from file"
	if(settings.max_depth > 0){
		// Generate the tree from the chunk table
		const ChunkTable* table_ptr = &m_reference_world->m_chunk_table;
		freeTreeData(m_kd_tree_ptr);
		m_kd_tree_ptr = VoxelKDTree::TreeBuilder::buildTree(*table_ptr, settings);
	}else{
		// Have a default tree loaded from file
		freeTreeData(m_kd_tree_ptr);
		m_kd_tree_ptr = VoxelKDTree::loadTreeFromFile("VKDT.binary");
		m_kd_tree_ptr->bounds.origin += {CHUNK_LEN, CHUNK_LEN, CHUNK_LEN};
	}

	bool visualize_tree = true;
	if(visualize_tree){
		const std::string LEAFVIZ_NAME = "TreeLeaves";
		const std::string PLANEVIZ_NAME = "TreePlanes";
		std::string name = LEAFVIZ_NAME;
		Debug::DebugData leaf_viz = Debug::visualizeTree(m_kd_tree_ptr);
		m_reference_world->addWidgetData(name, leaf_viz.widget_groups[name], false);
	}
}

RayIntersection SimCache::traceRay(Ray ray){
	Debug::DebugData debug;
	
	// Visualize intersection data
	//const ChunkTable* table_ptr = &m_reference_world->m_chunk_table;
	//auto intersection = Intersection::intersectChunks(ray, table_ptr);
	auto intersection = Intersection::intersectTree(ray, m_kd_tree_ptr);
	return intersection;
}

#if 0
void SimCache::bulkRayIntersections(const std::vector<Ray>& rays){
	/*
	Used to test performance and visualize results
	*/

	std::vector<RayIntersection> intersections;
	intersections.reserve(rays.size());
	for(Ray ray : rays){
		intersections.push_back(Intersection::intersectTree(ray, m_kd_tree_ptr));
	}

	// Find the maximum t value so depth views have a dynamic range
	float max_t = 0;
	for(RayIntersection result : intersections){
		bool is_hit = result.type == INTERSECT_HIT_CHUNK_VOXEL;
		max_t = std::max(max_t, result.t_hit * is_hit);
	}
	
	std::vector<Widget> ray_widgets;
	ray_widgets.reserve(rays.size());
	for(Uint64 i = 0; i < rays.size(); ++i){
		bool is_hit = intersections[i].type == INTERSECT_HIT_CHUNK_VOXEL;
		float t_hit = is_hit ? intersections[i].t_hit : max_t;
		
		t_hit = 1.0;
		if(is_hit){
			t_hit = intersections[i].t_hit;
			ray_widgets.push_back(Debug::widgetFromRay(rays[i], {1,1,1}, t_hit));
		}
	}

	m_reference_world->addWidgetData("RayWidgets", ray_widgets, false);
}
#endif

#if 0
void SimCache::traceImage(Camera& camera, Rendering::ImageConfig config){
	/*
	TODO: Move into the raytracer.
	*/

	Image::Settings settings = {
		Image::PIXELTYPE_MONOCHROME,
		//Image::PIXELTYPE_RGB,
		config.num_pixels,
		.initial_clear=true
	};

	std::vector<Ray> image_rays = Rendering::allRays(camera, config);
	assert(image_rays.size() == (Uint64)(config.num_pixels.x * config.num_pixels.y));
	
	float hit_extremes[] = {LARGE_FLOAT, -LARGE_FLOAT};
	std::vector<RayIntersection> hit_results;
	hit_results.reserve(image_rays.size());
	for(Uint64 i = 0; i < image_rays.size(); ++i){
		auto hit = Intersection::intersectTree(image_rays[i], m_kd_tree_ptr);
		if(isValid(hit)){
			if(hit.t_hit < hit_extremes[INDEX_VALUE_MIN]){
				hit_extremes[INDEX_VALUE_MIN] = hit.t_hit;
			}

			if(hit.t_hit > hit_extremes[INDEX_VALUE_MAX]){
				hit_extremes[INDEX_VALUE_MAX] = hit.t_hit;
			}
		}
		hit_results.push_back(hit);
	}

	Image image(settings);
	float t_range = hit_extremes[INDEX_VALUE_MAX] - hit_extremes[INDEX_VALUE_MIN];
	for(Uint64 i = 0; i < hit_results.size(); ++i){
		auto& hit = hit_results[i];
		float depth = (hit.t_hit - hit_extremes[INDEX_VALUE_MIN]) / t_range * isValid(hit);
		
		bool is_hit = hit.type == INTERSECT_HIT_CHUNK_VOXEL || requiresLookup(hit.type);
		float material_multiplier = 
			is_hit * VOXEL_COLOR_BY_TYPE[hit.voxel_hit.palette_index].length() / 3 +
			!is_hit;
		float color = (1 - depth) * material_multiplier;

		image.pixelMonochrome(i).colors[0] = color * 255;
	}

	saveToPPM(image, "TestOutput.ppm");
}
#endif

#if 0
void SimCache::findIntersectionDiscrepancies(int num_iterations){
	/*
	Brute force. Generates rays at random positions and orientations, then collides
	using KDTree and Grid Iteration against both. Returns a list of rays that don't 
	return the same result.
	*/

	//printf("Setting up test...\n");
	Debug::DebugData debug;
	const ChunkTable* table_ptr = &m_reference_world->m_chunk_table;
	ICuboid chunkspace_bounds = table_ptr->boundingVolumeChunkspace();

	//freeTreeData(m_kd_tree_ptr);
	//m_kd_tree_ptr = VoxelKDTree::TreeBuilder::buildTree(*table_ptr, settings);

	//printf("Running discrepancy test...\n");
	std::vector<Ray> random_rays;
	random_rays.reserve(num_iterations);
	IVec3 voxel_origin = chunkspace_bounds.origin * CHUNK_LEN;
	IVec3 voxel_extent = chunkspace_bounds.extent * CHUNK_LEN;
	for(int i = 0; i < num_iterations; ++i){
		Ray new_ray;
		for(int x = 0; x < 3; ++x){
			new_ray.origin[x] = rand() % voxel_extent[x] + voxel_origin[x];
			new_ray.dir[x] = rand() % voxel_extent[x] - (voxel_extent[x] / 2);
		}
		new_ray.dir = new_ray.dir.normal();
		random_rays.push_back(new_ray);
	}

	constexpr float THRESHOLD = 0.01;
	constexpr int TYPE_MISMATCH = 0;
	constexpr int T_MISMATCH = 1;
	constexpr int MATCH = 2;
	std::unordered_map<int, float> mismatch_map;
	std::unordered_map<int, std::unordered_set<int>> index_types;
	for(int i = 0; i < num_iterations; ++i){
		auto hit1 = Intersection::intersectTree(random_rays[i], m_kd_tree_ptr);
		auto hit2 = Intersection::intersectChunks(random_rays[i], table_ptr);
		if(!isValid(hit1) && !isValid(hit2)){
			index_types[MATCH].insert(i);
			continue;
		}else if(hit1.type != hit2.type){
			index_types[TYPE_MISMATCH].insert(i);
			continue;
		}

		float t_diff = std::abs(hit1.t_hit - hit2.t_hit);
		if(t_diff > THRESHOLD){
			mismatch_map[i] = t_diff;
			index_types[T_MISMATCH].insert(i);
			continue;
		}

		index_types[MATCH].insert(i);
	}

	Int64 num_mismatched = random_rays.size() - index_types[MATCH].size();
	float percent = (float)num_mismatched / (float)random_rays.size() * 100;
	printf("%li/%li bad rays (%f%%)\n", num_mismatched, random_rays.size(), percent);

	FVec3 TYPE_COLORS[] = {
		{1.0, 0.2, 0.2},
		{0.2, 1.0, 0.2},
		{0.2, 0.2, 1.0},
	};
	std::vector<Widget> ray_widgets;
	ray_widgets.reserve(random_rays.size() * 2);
	for(Uint64 i = 0; i < random_rays.size(); ++i){
		int ray_type = 0;
		for(auto iter = index_types.begin(); iter != index_types.end(); ++iter){
			if(iter->second.find(i) != iter->second.end()){
				ray_type = iter->first;
				break;
			}
		}

		float t_diff = 1;
		if(ray_type == T_MISMATCH){
			t_diff = mismatch_map[i];
		}

		FVec3 color = TYPE_COLORS[ray_type];
		ray_widgets.push_back(Debug::widgetFromRay(random_rays[i], color));
		if(ray_type != MATCH){
			ray_widgets.push_back(Debug::widgetFromPoint(random_rays[i].origin, color / t_diff));	
		}
	}
	
	m_reference_world->addWidgetData("RayWidgets", ray_widgets, false);
}
#endif
