#include "Raytracer.hpp"



//-------------------------------------------------------------------------------------------------
// PathBuffer
//-------------------------------------------------------------------------------------------------
Raytracer::PathBuffer Raytracer::PathBuffer::init(Int32 max_path_len, Int32 max_paths, 
	bool should_compress_failed_paths){
	/*
	Allocates enough memory for every single ray to take the longest
	possible path and end at a light at the last bounce.
	*/

	PathBuffer new_buffer;
	new_buffer.vertices = (PathVertex*) malloc(sizeof(PathVertex) * 
		max_paths * max_path_len);
	new_buffer.results =  (PathResult*) malloc(sizeof(PathResult) * 
		max_paths);
	new_buffer.result_capacity = max_paths;
	new_buffer.result_count = 0;
	new_buffer.max_path_len = max_path_len;
	new_buffer.should_compress_failed_paths = should_compress_failed_paths;
	return new_buffer;
}

Int32 Raytracer::PathBuffer::vertexCapacity(){
	return result_capacity * max_path_len;
}

void Raytracer::PathBuffer::freeMemory(){
	free(vertices);
	free(results);
};

//-------------------------------------------------------------------------------------------------
// Raytracer
//-------------------------------------------------------------------------------------------------
Raytracer::Raytracer(){
	m_should_compress_failed_paths = false;
	m_default_random.splitmix.state = 314159265;
}

Raytracer::~Raytracer(){

}

void Raytracer::sendInstruction(SystemInstruction instruction){
	/*
	NOTE: This is where the raytracer is notified that it needs to update
	its internal representation.
	*/

}

void Raytracer::setOutputFilepath(std::string filepath){
	/*
	Sets the output filepath. Also uses the file extension to determine the
	image format.
	*/
}


void Raytracer::setWindowPtr(std::shared_ptr<Window> window_ptr){
	m_window_ptr = window_ptr;
}


Image::PixelRGB pixelFromColor(FVec3 color){
	return {(Uint8)color.x, (Uint8)color.y, (Uint8)color.z};
}

float clamp(float input){
	return min(max(0.0f, input), 1.0f);	
}

FVec3 clamp(FVec3 input){
	FVec3 output;
	for(int i = 0; i < 3; ++i){
		output[i] = clamp(input[i]);
	}
	return output;
}

float pow(float input, int power){
	float result = 1.0f;
	for(int i = 0; i < power; ++i){
		result *= input;
	}
	return result;
}

void Raytracer::renderImage(const SimCache& cache, Camera camera, RenderSettings settings){
	/*
	Renders the image and saves the output to a file.

	REFACTOR: Break this up into separate functions
	*/

	// TODO: Support the option to refine a previous render.
	Rendering::ImageConfig& config = settings.image_config;
	bool should_reuse_color_data = false;
	m_scratch_image.resize(config.num_pixels);

	// Init image tiles
	std::vector<Rendering::ImageTile> tiles = Rendering::tiles(camera, config);

	Int32 num_tiles = (Int32) tiles.size();
	constexpr Uint64 ARBITRARY_MAX_TILE_COUNT = 1 << 25; // Keep it somewhere under max Int32.
	assert(tiles.size() < ARBITRARY_MAX_TILE_COUNT);

	// TODO: Put this in its own function to declutter
	settings.sun_direction = settings.sun_direction.normal();
	TileJob job_template;  // Values that stay the same for all jobs
	{
		/*
		float film_distance = 1.0f;  // Arbitrary

		// Convert coordinate system where +X is right, +Y is down, and -Z is the view direction.
		// NOTE: The image_y is reversed so the image isn't upside-down
		FVec3 plane_world_pos = camera.pos + camera.basis.v1 * film_distance;
		FVec3 image_plane_normal = camera.basis.v1.normal();
		FVec3 image_x = image_plane_normal.cross(camera.basis.v2).normal();
		FVec3 image_y = -image_x.cross(image_plane_normal).normal();
		//FVec3 image_z = image_x.cross(image_y).normal();

		// Scaling factor
		// Set the film ratio of the largest dimension relative to the smallest
		FVec2 film_ratio = {1.0, 1.0};
		FVec2 float_image_dims = toFloatVector(config.num_pixels);
		if(float_image_dims.x > float_image_dims.y){
			film_ratio.x = float_image_dims.x / float_image_dims.y;
		}else{
			film_ratio.y = float_image_dims.y / float_image_dims.x;
		}
		FVec2 half_dims = {film_ratio.x * 0.5f, film_ratio.y * 0.5f};
		*/

		Rendering::CameraRayGenerator ray_generator(camera, config.num_pixels);

		// Save the values
		job_template = {  
			.image_info={
				// General metadata
				.simcache_ptr=&cache,
				.settings=settings,
				.pixel_buffer=(Image::PixelRGB*)m_scratch_image.dataPtr(),
				.ray_generator=ray_generator

				/*
				// Ray positioning info
				.plane_world_pos=plane_world_pos,
				.camera_pos=camera.pos,
				.image_x=image_x,
				.image_y=image_y,
				.half_dims=half_dims
				*/
			},
		};
	}

	// Init a job object for each tile
	RandomGen tile_random_gen;
	Int32 pixels_per_tile = config.tile_dims.x * config.tile_dims.y;
	std::vector<TileJob> tile_jobs;
	tile_jobs.reserve(num_tiles);
	for(Int32 i = 0; i < num_tiles; ++i){
		TileJob filled_job_struct = job_template;
		filled_job_struct.tile_info = {
			.tile=tiles[i],
			.gen=tile_random_gen,
			.use_prior_data=should_reuse_color_data
		};

		for(Int32 x = 0; x < pixels_per_tile; ++x){
			tile_random_gen.splitmix.next();
		}

		tile_jobs.push_back(filled_job_struct);
	}

	// This renders a preview first so that progress updates are made
	// over the preview image instead of a black background.
	// TODO: Option to disable preview for headless renders
	if(true){
		renderPreview(cache, camera, config, false);	
	}else{
		Uint64 num_image_bytes = config.tile_dims.x * config.tile_dims.y * sizeof(Image::PixelRGB);
		memset(m_scratch_image.dataPtr(), 0, num_image_bytes);
	}
	
	// Run the jobs in batches
	// TODO: Figure out how to get these constructed in place without this nonsense
	// extra layer of indirection. With a thread array I can't figure out how to stop
	// it from stupidly calling the destructor on uninitialized memory.
	Int32 num_threads = settings.num_render_threads;
	printf("%i tiles, %i worker slots\n", num_tiles, num_threads);
	Int32 job_index = 0;
	std::thread** thread_ptr_arr = (std::thread**) malloc(num_threads * sizeof(std::thread));
	bool should_abort_render = false;
	while(job_index < num_tiles && !should_abort_render){
		Int32 num_to_launch = min(num_threads, num_tiles - job_index);
		printf("Running new batch of %i tiles (%i/%i complete)\n", 
			num_to_launch, job_index, num_tiles);
		for(Int32 i = 0; i < num_to_launch; ++i){
			thread_ptr_arr[i] = new std::thread(&Raytracer::runTileJob, this, tile_jobs[job_index]);
			++job_index;
		}

		// Wait for the batch to finish
		for(Int32 i = 0; i < num_to_launch; ++i){
			thread_ptr_arr[i]->join();
			delete thread_ptr_arr[i];
		
			// Render a preview
			//----------------------------------------
			renderImageToQuad(m_scratch_image, false);

			m_window_ptr->pollEvents();
			should_abort_render = m_window_ptr->isKeyInState(
				KeyEventType::KEY_PRESSED, KEY_BACKSPACE);
		}
	}
	free(thread_ptr_arr);
	
	if(should_abort_render){
		printf("Render aborted by user\n");
	}else{
		printf("Render complete. Press ENTER to save or anything else to discard.\a\n");
		renderImageToQuad(m_scratch_image, true);

		m_window_ptr->pollEvents();
		bool should_save_output = m_window_ptr->isKeyInState(KeyEventType::KEY_PRESSED, KEY_ENTER);
		if(should_save_output){
			std::string filepath = "TestOutput.ppm";
			printf("Saved image to file '%s'\n", filepath.c_str());
			saveToPPM(m_scratch_image, filepath);
		}else{
			printf("User opted to discard the image.\n");
		}
	}
}

void Raytracer::visualizePaths(const SimCache& cache, std::vector<Ray> rays){
	/*
	Given a list of vectors to start from, trace their paths and init widget
	meshes to represent them
	*/

	constexpr Int32 ARBITRARY_MAX_PATH_LEN = 20;

	Int32 num_rays = rays.size();
	PathBuffer buffer = PathBuffer::init(ARBITRARY_MAX_PATH_LEN, num_rays, false);
	buffer.result_count = num_rays;
	tracePaths(&cache, rays, buffer, m_default_random);

	std::vector<Widget> widgets;
	Int32 vertex_read_start = 0;
	for(Int32 path_index = 0; path_index < buffer.result_count; ++path_index){
		// Color the path differently depending on how it terminated
		FVec3 path_color = {1.0, 0.5, 0.5};
		PathResult& result = buffer.results[path_index];
		PathVertex* curr_path = &buffer.vertices[vertex_read_start];
		
		// Calculate the light color based on what happened at the end vertex.
		if(result.is_terminated_at_light){
			PathVertex& end_vertex = curr_path[result.num_filled - 1];
			if(end_vertex.type == INTERSECT_HIT_CHUNK_VOXEL){
				path_color = {1.0, 1.0, 1.0};
			}else if(end_vertex.type == INTERSECT_MISS){
				path_color = {0.7, 0.7, 1.0};
			}else{
				path_color = {1.0, 0.0, 0.0};
			}
		}

		// Now go vertex-to-vertex creating line widgets. Since rays are
		// able to teleport, both the start and end positions are saved in 
		// each vertex.
		for(Int32 v = 0; v < result.num_filled; ++v){
			PathVertex& curr_vertex = curr_path[v];

			Ray ray = curr_vertex.source_ray;
			float t_hit = curr_vertex.t_hit;
			if((v == result.num_filled - 1) && result.is_terminated_at_light){
				t_hit = 1000;
			}

			widgets.push_back(Debug::widgetFromLine(
				ray.origin, posFromT(ray, t_hit), 
				path_color));
			widgets.push_back(Debug::widgetFromPoint(
				ray.origin, 
				{1,0,0}));
			widgets.push_back(Debug::widgetFromPoint(
				posFromT(ray, t_hit), 
				{0,0,1}));
		}

		vertex_read_start += result.num_filled;
	}

	cache.m_reference_world->addWidgetData("RayWidgets", widgets, true);
}

void Raytracer::renderPreview(const SimCache& cache, Camera camera, Rendering::ImageConfig config){
	/*
	Wrapper function to make calls to the renderPreview function easier to work with
	*/

	renderPreview(cache, camera, config, true);
}

void Raytracer::renderPreview(const SimCache& cache, Camera camera, Rendering::ImageConfig config, 
	bool is_interactive){
	/*
	Renders a quick preview of the scene
	*/

	constexpr Int64 MAX_UINT_16 = 65535;
	constexpr Int64 MAX_INT_32 = 1ULL << 31;
	assert(config.num_pixels.x <= MAX_UINT_16);
	assert(config.num_pixels.y <= MAX_UINT_16);

	std::vector<Ray> image_rays = Rendering::allRays(camera, config);
	Int32 num_rays = image_rays.size();
	assert(num_rays < MAX_INT_32);
	assert(num_rays == config.num_pixels.x * config.num_pixels.y);
	
	float hit_extremes[] = {LARGE_FLOAT, -LARGE_FLOAT};
	PathBuffer buffer = PathBuffer::init(1, num_rays, false);
	tracePaths(&cache, image_rays, buffer, m_default_random);
	buffer.result_count = num_rays;

	for(Int32 i = 0; i < num_rays; ++i){
		PathVertex& ray_path = buffer.vertices[i];

		if(!(ray_path.type == INTERSECT_MISS)){
			if(ray_path.t_hit < hit_extremes[INDEX_VALUE_MIN]){
				hit_extremes[INDEX_VALUE_MIN] = ray_path.t_hit;
			}

			if(ray_path.t_hit > hit_extremes[INDEX_VALUE_MAX]){
				hit_extremes[INDEX_VALUE_MAX] = ray_path.t_hit;
			}
		}
	}

	m_scratch_image.resize(config.num_pixels);

	float t_range = hit_extremes[INDEX_VALUE_MAX] - hit_extremes[INDEX_VALUE_MIN];
	for(Int32 i = 0; i < num_rays; ++i){
		PathVertex& ray_path = buffer.vertices[i];
		bool is_hit = 
			requiresLookup(ray_path.type) ||
			(ray_path.type == INTERSECT_HIT_CHUNK_VOXEL) ||
			(ray_path.type == INTERSECT_HIT_COLLIDER);
		float depth = (ray_path.t_hit - hit_extremes[INDEX_VALUE_MIN]) / t_range;
		
		FVec3 material_color = {0, 0, 0};
		if(is_hit){
			if(ray_path.material_index == 128){
				material_color = LIGHT_EMITTING_VOXEL_COLOR;	
			}else{
				material_color = VOXEL_COLOR_BY_TYPE[ray_path.material_index];
			}

			if(ray_path.type == INTERSECT_HIT_COLLIDER){
				material_color = {1, 0, 0};
			}
		}
		FVec3 color = (1 - depth) * material_color;

		for(int x = 0; x < 3; ++x){
			m_scratch_image.pixelRGB(i).colors[x] = clamp((int) (color[x] * 255), 0, 255);
		}
	}

	//----------------------------------------
	// Render a preview
	//----------------------------------------
	renderImageToQuad(m_scratch_image, is_interactive);
	//saveToPPM(m_scratch_image, "TestOutput.ppm");
}

void Raytracer::renderImageToQuad(Image& image, bool should_wait_for_input){
	/*
	Renders an image to the screen. 
	
	If should_wait_for_input is True then the image will remain until 
	a key is pressed.
	*/

	constexpr Int32 num_microseconds = 0.050 * 1000000;

	IVec2 image_dims = image.dimensions();
	Uint32 preview_texture_id = textureFromColorBuffer(image.dataPtr(), image_dims);
	m_quad_renderer.render(preview_texture_id, image_dims);
	glDeleteTextures(1, &preview_texture_id);

	m_window_ptr->swapBuffers();
	while(should_wait_for_input){
		usleep(num_microseconds);

		m_window_ptr->pollEvents();
		auto inputs = m_window_ptr->getInputEvents();
		should_wait_for_input = (inputs.size() == 0);
	}
}

void Raytracer::runTileJob(TileJob job){
	/*
	TODO: Allocate the color buffer in the calling function and pass the
		ptr in through the job struct.

	ATTRIBUTION: https://www.scratchapixel.com/lessons/3d-basic-rendering/
		ray-tracing-generating-camera-rays/generating-camera-rays

		and

		https://www.youtube.com/watch?v=pq7dV4sR7lg
	*/

	Rendering::ImageTile& tile = job.tile_info.tile;

	// Get aliases ready and allocate buffers
	RenderSettings& settings = job.image_info.settings;
	Rendering::ImageConfig& config = settings.image_config;
	float sample_contribution = 1.0f / settings.num_rays_per_pixel;
	Int32 num_pixels = tile.range_x.extent * tile.range_y.extent;
	
	PathBuffer path_buffer = PathBuffer::init(settings.max_path_len, num_pixels, 
		m_should_compress_failed_paths);
	path_buffer.result_count = num_pixels;
	std::vector<FVec3> color_buffer(num_pixels, COLOR_BLACK);
	std::vector<Ray> ray_buffer(num_pixels, Ray{});

	// STEP: Trace paths, calculate colors, and update pixel buffer.
	for(Int32 r = 0; r < settings.num_rays_per_pixel; ++r){
		// Fill the ray buffer with new rays
		Int32 ray_index = 0;
		for(Int32 y = 0; y < tile.range_y.extent; ++y){
			for(Int32 x = 0; x < tile.range_x.extent; ++x){
				IVec2 pixel_coord = {tile.range_x.origin + x, tile.range_y.origin + y};
				
				// Init a "perfect" ray, then add random jitter to the direction vector.
				Ray ray = job.image_info.ray_generator.rayFromPixelCoord(pixel_coord);
				FVec3 random_jitter = randomUnitNormal(job.tile_info.gen) * 0.003;
				ray.dir = (ray.dir + random_jitter).normal();

				ray_buffer[ray_index++] = ray;
			}
		}

		// Trace the paths
		tracePaths(job.image_info.simcache_ptr, ray_buffer, path_buffer, job.tile_info.gen);
		auto batch_colors = determineColors(job.image_info.simcache_ptr, path_buffer, settings);

		// Update the color buffer
		assert(
			batch_colors.size() == color_buffer.size() && 
			(Int32) color_buffer.size() == num_pixels);
		for(Int32 i = 0; i < num_pixels; ++i){
			color_buffer[i] += batch_colors[i] * sample_contribution;
		}
	}

	// STEP: All color information has been updated. Write it out to the image buffer.
	Int64 image_index_linear = tile.range_x.origin + config.num_pixels.x * tile.range_y.origin;
	Int64 image_step_amount = config.num_pixels.x - tile.range_x.extent;
	Int32 tile_index_linear = 0;
	for(Int32 y = 0; y < tile.range_y.extent; ++y){
		for(Int32 x = 0; x < tile.range_x.extent; ++x){
			FVec3 output_color;
			FVec3 raw_color = color_buffer[tile_index_linear++];
			
			// Clamp to range and gamma correct
			for(int i = 0; i < 3; ++i){
				output_color[i] = sqrt(clamp(raw_color[i]));
			}

			Image::PixelRGB pixel = pixelFromColor(output_color * 255);
			job.image_info.pixel_buffer[image_index_linear++] = pixel;
		}
		image_index_linear += image_step_amount;
	}
}

bool isLightTerminated(RayIntersection hit){
	bool is_light_voxel = 
		(hit.type == INTERSECT_HIT_CHUNK_VOXEL) && 
		(hit.voxel_hit.palette_index == LightEmitter);

	return (hit.type == INTERSECT_MISS) || is_light_voxel || requiresLookup(hit.type);
}

void Raytracer::tracePaths(const SimCache* cache_ptr, const std::vector<Ray>& rays, 
	const PathBuffer& buffer, RandomGen& random_gen) const{
	/*
	TODO: Add ability to set which resource types are involved in a trace.
	*/

	constexpr float TINY_FLOAT = 0.00001;
	constexpr float BOUNCE_OFFSET = 0.001;
	constexpr float SURFACE_ROUGHNESS[] = {
		0,
		0,

		0.00, // Air,
		0.80, // Grass,
		0.80, // Dirt,
		0.85, // Stone,
		0.95, // Concrete,
		0.02, // Metal,
	};

	const ChunkTable* table_ptr = &cache_ptr->m_reference_world->m_chunk_table;

	Intersection::Utils::VKDTStack stack = Intersection::Utils::VKDTStack::init(
		cache_ptr->m_kd_tree_ptr->curr_max_depth);
	Int32 vertex_write_index = 0;
	Int32 num_rays = rays.size();
	for(Int32 ray_index = 0; ray_index < num_rays; ++ray_index){
		// STEP 1: Trace the path of the ray as it bounces through the scene
		bool is_terminated_at_light = false;
		Ray curr_ray = rays[ray_index];
		
		Int32 path_len = 0;
		while(path_len < buffer.max_path_len){
			// STEP: Intersection tests
			PathVertex curr_vertex;
			curr_vertex.source_ray = curr_ray;
			curr_vertex.material_index = 0;

			// VKDTree Intersection
			RayIntersection curr_hit{INTERSECT_MISS};
			curr_hit.t_hit = 0;
			auto hit = Intersection::intersectTree(curr_ray, 
				cache_ptr->m_kd_tree_ptr, stack);
			if(hit.type == INTERSECT_HIT_CHUNK_VOXEL){
				curr_hit = hit;
			}else if(curr_hit.type == INTERSECT_POSSIBLE_CHUNK_VOXEL){
				// TODO: Follow up with chunk trace if this happens
				// NOTE: For now I'm just making the offending areas glow bright red.
			}

			hit = Intersection::intersectChunks(curr_ray, table_ptr);
			if(hit.type == INTERSECT_HIT_CHUNK_VOXEL && hit.t_hit < curr_hit.t_hit){
				curr_hit = hit;
			}

			// MKDTree Intersection
			
			
			// Portal intersection
			// TODO: Move to function
			if(true){
				Int32 curr_site_index = -1;
				Portal& portal = cache_ptr->m_reference_world->m_temp_portal;
				Intersection::DetailedSphereIntersection best_hit{.is_valid=false};
				for(Int32 p = 0; p < 2; ++p){
					FSphere site = {portal.locations[p], portal.radius};
					auto site_hit = Intersection::intersectColliderDetailed(curr_ray, site);
					if(site_hit.is_valid){
						if(!best_hit.is_valid ||
							(site_hit.t_bounds[INDEX_VALUE_MIN] < best_hit.t_bounds[INDEX_VALUE_MIN])){
							
							// Got a first hit or better hit
							curr_site_index = p;
							best_hit = site_hit;
						}
					}
				}

				// Relocate ray if an intersection happened.
				bool should_update = curr_hit.type == INTERSECT_MISS ||
					best_hit.t_bounds[INDEX_VALUE_MIN] < curr_hit.t_hit;
				if(curr_site_index > -1 && should_update){
					Int32 other_site_index = !curr_site_index;
					FVec3 source = portal.locations[curr_site_index];
					FVec3 target = portal.locations[other_site_index];

					FVec3 offset_to_other = target - source;
					FVec3 local_enter = posFromT(curr_ray, best_hit.t_bounds[INDEX_VALUE_MIN]);
					FVec3 local_exit =  posFromT(curr_ray, best_hit.t_bounds[INDEX_VALUE_MAX]);
					Ray new_ray = {local_exit + offset_to_other, curr_ray.dir};

					curr_vertex.type = INTERSECT_HIT_COLLIDER;
					curr_vertex.t_hit = best_hit.t_bounds[INDEX_VALUE_MIN];
					curr_vertex.hit_normal = (local_enter - source).normal();
					buffer.vertices[vertex_write_index++] = curr_vertex;
					curr_ray = new_ray;
					++path_len;
					continue;
				}
			}

			// NOTE: Any other ray intersection tests go here.

			// STEP: Set vertex data and end if this is light terminated
			curr_vertex.type = curr_hit.type;
			curr_vertex.t_hit = curr_hit.t_hit;
			if(curr_hit.type == INTERSECT_HIT_CHUNK_VOXEL){
				curr_vertex.material_index = curr_hit.voxel_hit.palette_index;
			}
			if(isLightTerminated(curr_hit)){
				is_terminated_at_light = true;
				buffer.vertices[vertex_write_index++] = curr_vertex;
				++path_len;
				break;
			}else if(curr_hit.t_hit < TINY_FLOAT){
				// Ray terminated in a wall.
				break;
			}
			
			// STEP: Based on the surface properties of the hit location, 
			// generate a new outgoing ray.
			FVec3 hit_normal = NORMALS_BY_FACE_INDEX[curr_hit.voxel_hit.face_index];
			float roughness = SURFACE_ROUGHNESS[curr_hit.voxel_hit.palette_index];
			FVec3 new_dir = bounceDir(random_gen, curr_ray, hit_normal, roughness).normal();
			
			// WARNING: Assumes that any ambiguous hits were followed up on until
			// a material was obtained. 
			curr_vertex.hit_normal = hit_normal;
			buffer.vertices[vertex_write_index++] = curr_vertex;

			// Change to the new ray and continue to the next iteration
			FVec3 hit_pos = posFromT(curr_ray, curr_hit.t_hit);
			Ray new_ray = {hit_pos + hit_normal * BOUNCE_OFFSET, new_dir};
			curr_ray = new_ray;
			++path_len;
		}

		// Rewinds write index so it ends up at the start of the failed path.
		// The next path will overwrite the useless data.
		bool should_rewind = !is_terminated_at_light && buffer.should_compress_failed_paths;
		int rewind_amount = path_len * should_rewind;
		vertex_write_index -= rewind_amount;

		PathResult result;
		result.num_filled = path_len * !should_rewind;
		result.is_terminated_at_light = is_terminated_at_light;
		buffer.results[ray_index] = result;
	}
	stack.freeMemory();
}

std::vector<FVec3> Raytracer::determineColors(const SimCache* cache, const PathBuffer& buffer,
	RenderSettings settings) const{
	/*
	The transport process is complete. Gather color info from a filled buffer.
	Since this step includes additional complications like texture mapping and
	potentially normal mapping, it has been separated from the path tracing step.
	
	WARNING: This assumes that hits with INTERSECT_POSSIBLE_CHUNK_VOXEL were
		followed up on and either yielded a hit with a palette index or a 
		path terminating light.
	*/
	
	std::vector<FVec3> output_colors;
	output_colors.reserve(buffer.result_count);

	// NOTE: I'm allowing lights to be outside the 0-1 range for lighting
	// calculations as a poor-man's HDR as long as the final pixel color is clamped
	// before being written to a file. That is not the responsibility of this function.
	const FVec3& sky_color = settings.sky_brightness;
	const FVec3& sun_color = settings.sun_brightness;
	const FVec3& to_sun_dir = settings.sun_direction;

	Int32 vertex_read_start = 0;
	for(Int32 path_index = 0; path_index < buffer.result_count; ++path_index){
		FVec3 path_color = COLOR_BLACK;

		PathResult& result = buffer.results[path_index];
		if(result.is_terminated_at_light){
			PathVertex* curr_path = &buffer.vertices[vertex_read_start];
			
			// Calculate the light color based on what happened at the end vertex.
			PathVertex& end_vertex = curr_path[result.num_filled - 1];
			if(end_vertex.type == INTERSECT_HIT_CHUNK_VOXEL){
				// Hit a light-emitting voxel
				path_color = LIGHT_EMITTING_VOXEL_COLOR;
			}else if(end_vertex.type == INTERSECT_MISS){
				// Hit the skylight	
				FVec3 to_sky = end_vertex.source_ray.dir;
				float alignment = clamp(to_sky.dot(to_sun_dir));
				path_color = sky_color + sun_color * pow(alignment, 128);
			}else{
				// Hit an unknown type that wasn't followed up on in the path tracing step.
				path_color = {1000.0f, 0, 0};
			}

			// One less operation than the number of intersection tests, 
			// since the last one is always the light
			for(Int32 v = 0; v < result.num_filled - 1; ++v){
				PathVertex& curr_vertex = curr_path[v];
				if(curr_vertex.type == INTERSECT_HIT_COLLIDER){
					// Colliders don't have materials. They cause the ray to
					// be modified in some way, but don't directly change
					// the color.
					continue;
				}

				// TODO: Index into a Raytracer material palette instead of
				// 	the voxel color palette.
				MaterialPaletteIndex index = curr_vertex.material_index;	
				FVec3 hit_color = VOXEL_COLOR_BY_TYPE[index];
				path_color = hadamard(hit_color, path_color);
			}
		}

		vertex_read_start += result.num_filled;
		output_colors.push_back(path_color);
	}

	return output_colors;
}

