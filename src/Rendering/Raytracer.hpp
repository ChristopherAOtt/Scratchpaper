#pragma once

#include "RayTracing.hpp"
#include "SimCache.hpp"
#include "WorldState.hpp"
#include "Image.hpp"
#include "SystemMessages.hpp"
#include "Colors.hpp"
#include "Debug.hpp"
#include "Window.hpp"
#include "QuadRenderer.hpp"

#include <thread>
#include <string.h>  // For memset
#include <memory>

struct RandomGen{
	// Normally distributed, slow
	std::default_random_engine random_engine;
	std::normal_distribution<float> normal_distribution;

	// Linearly distributed, fast, and wrong
	MathUtils::Random::SebVignaSplitmix64 splitmix{1};
	inline double nextLinearUnitDouble(){
		return MathUtils::Random::doubleFromUint64(splitmix.next());
	}
};

inline FVec3 randomUnitNormal(RandomGen& gen){
	/*
	ATTRIBUTION: Box-Muller Transform explanation
		https://towardsdatascience.com/the-best-way-to-pick-a-unit-vector-7bd0cc54f9b
	*/

	FVec3 output;
	for(int i = 0; i < 3; ++i){
		output[i] = 2 * gen.nextLinearUnitDouble() - 1;
		//output[i] = gen.normal_distribution(gen.random_engine);
	}
	return output.normal();
}

inline FVec3 randomDirAroundNormal(RandomGen& gen, FVec3 normal){
	/*
	ATTRIBUTION: Random dir picking technique
	https://raytracing.github.io/books/RayTracingInOneWeekend.html
		#diffusematerials/asimplediffusematerial
	*/
	return (normal + randomUnitNormal(gen) * 0.999).normal();
}

inline FVec3 reflectDir(FVec3 ray_dir, FVec3 normal){
	/*
	Generate a direction that is reflected over the normal
	*/
	return ray_dir.reflection(normal);
}

inline FVec3 bounceDir(RandomGen& gen, Ray ray, FVec3 normal, float roughness){
	/*
	Blend of bounce and reflect directions based on a roughness value
	*/
	FVec3 random_dir = randomDirAroundNormal(gen, normal);
	FVec3 reflect_dir = reflectDir(ray.dir, normal);
	return lerp(reflect_dir, random_dir, roughness);
}

class Raytracer{
	/*

	*/

	public:
		typedef Int32 MaterialPaletteIndex;

		struct RenderSettings{
			Rendering::ImageConfig image_config;

			Int32 num_render_threads;
			Int32 num_rays_per_pixel;
			Int32 max_path_len;

			FVec3 sky_brightness;
			FVec3 sun_brightness;
			FVec3 sun_direction;
		};

	private:
		struct PathVertex{
			/*
			Represents a single collision point along a ray's path.
			TODO: Figure out how to get texture info in here without 
				blowing up the size or looking up the color in the path code.
				Possibly UV coords + MaterialIndex in a union?

				union{
					struct{
						Stuff
					}world_hit;

					struct{
						Stuff
					}light_hit;
				};
			*/

			IntersectionType type;
			MaterialPaletteIndex material_index;
			
			// TODO: Remove these
			Ray source_ray;
			FVec3 hit_normal;
			float t_hit;
		};

		struct PathResult{
			/*
			Metadata about a path. 
			*/

			bool is_terminated_at_light;
			Uint8 num_filled;
		};

		struct PathBuffer{
			/*
			Allocated once per ray batch. Used to avoid large numbers of
			repetitive memory allocations in the path tracing section.
			*/

			PathVertex* vertices;
			PathResult* results;
			Int32 result_capacity;
			Int32 result_count;
			Int32 max_path_len;
			bool should_compress_failed_paths;

			static PathBuffer init(Int32 max_path_len, Int32 max_paths, bool should_compress_failed_paths);
			Int32 vertexCapacity();
			void freeMemory();
		};

		struct TileJob{
			/*
			Packages info for a thread
			*/

			struct{
				/*
				Stays the same for the entire image
				*/

				// General metadata
				const SimCache* simcache_ptr;
				RenderSettings settings;
				Image::PixelRGB* pixel_buffer;

				// Position info used to orient rays
				FVec3 plane_world_pos;
				FVec3 camera_pos;
				FVec3 image_x, image_y;
				FVec2 half_dims;
			} image_info;

			struct{
				/*
				Specific to each tile job.
				*/

				Rendering::ImageTile tile;
				RandomGen gen;
				bool use_prior_data;  // If the buffer already has valid info
			} tile_info;
		};

	public:
		enum ImageFormat{
			FORMAT_INVALID = 0,

			FORMAT_PPM,
			FORMAT_BMP,
			FORMAT_JPG,
			FORMAT_PNG,
			FORMAT_QOI,
		};

		enum RaytracingType{
			TRACE_INVALID = 0,

			TRACE_CHUNK,
			TRACE_VKDTREE,
			TRACE_MESH,
			TRACE_RAY_MANIPULATOR,
		};

	public:
		Raytracer();
		~Raytracer();
		
		void sendInstruction(SystemInstruction instruction);
		void setOutputFilepath(std::string filepath);
		void setWindowPtr(std::shared_ptr<Window> window_ptr);
		void renderImage(const SimCache& cache, Camera camera, RenderSettings settings);
		void visualizePaths(const SimCache& cache, std::vector<Ray> rays);
		void renderPreview(const SimCache& cache, Camera camera, Rendering::ImageConfig config);

	private:
		void runTileJob(TileJob job);
		void tracePaths(const SimCache* cache_ptr, const std::vector<Ray>& rays, 
			const PathBuffer& buffer, RandomGen& random_gen) const;
		std::vector<FVec3> determineColors(const SimCache* cache, const PathBuffer& buffer,
			RenderSettings settings) const;

	private:
		ImageFormat m_format;
		std::vector<Bytes1> m_random_buffer;
		std::vector<FVec3> m_image_color_buffer;
		std::vector<Image::PixelRGB> m_pixel_buffer;

		std::shared_ptr<Window> m_window_ptr;

		RandomGen m_default_random;

	public:  // TODO: Better method of setting these values
		bool m_should_compress_failed_paths;
		Int32 m_num_render_threads{2};
};

