#pragma once

#include "Primitives.hpp"

/*
TODO: Add ifdef blocks depending on OpengGL vs Vulkan libraries.
*/

//-------------------------------------------------------------------------------------------------
// RendererRaytracer
//-------------------------------------------------------------------------------------------------
class Raytracer{
	public:
		Raytracer();
		~Raytracer();

	private:
};

//-------------------------------------------------------------------------------------------------
// RendererRaster
//-------------------------------------------------------------------------------------------------
class Rasterizer{
	public:
		Rasterizer();
		~Rasterizer();

	private:
};


//-------------------------------------------------------------------------------------------------
// RendererOpenGL
//-------------------------------------------------------------------------------------------------
class RendererOpenGL{
	public:
		RendererOpenGL();
		~RendererOpenGL();

	private:
};


//-------------------------------------------------------------------------------------------------
// RendererVulkan
//-------------------------------------------------------------------------------------------------
class RendererVulkan{
	public:
		RendererVulkan();
		~RendererVulkan();

	private:
};

//-------------------------------------------------------------------------------------------------
// Renderer
// Manages the different sub-renderers. 
//-------------------------------------------------------------------------------------------------
class Renderer{
	public:
		Renderer();
		~Renderer();

	private:
		Raytracer m_raytracer;
		Rasterizer m_rasterizer;
		RendererOpenGL m_renderer_opengl;
		RendererVulkan m_renderer_vulkan;

};