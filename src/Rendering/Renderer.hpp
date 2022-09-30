#pragma once

#include <memory>

#include "RenderingModules.hpp"
#include "OpenGLModule.hpp"
#include "SystemMessages.hpp"
#include "SimCache.hpp"

struct InitData{
	// 
};

class Renderer{
	public:
		Renderer();
		~Renderer();

		// OpenGL functionality
		void initOpenGL();
		void sendInstruction(SystemInstruction instruction);

		// Vulkan Functionality
		// TODO: Add content

		// Raytracer Functionality
		// TODO: Add content


		void render(const SimCache& state, Camera camera);

	private:
		RenderingModule m_current_module;
		OpenGLModule m_opengl_module;  // TODO: IFDEFs for different rendering APIs
		RendererSettings m_current_settings;
};
