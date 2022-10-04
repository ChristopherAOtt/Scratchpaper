#pragma once

#include "Primitives.hpp"
#include "Framebuffer.hpp"
#include "QuadShader.hpp"
#include "FileIO.hpp"  // loadShadersFromFile()

#include <string>
#include <memory>

class QuadRenderer{
	private:
		struct RenderData{
			/*
			Packages info necessary for rendering.
			*/

			IVec2 dims;

			Uint32 fbo_id;
			Uint32 color_id;
			Uint32 depth_id;
		};

	public:
		QuadRenderer();
		~QuadRenderer();

		void initShader(std::string filepath);
		void setViewportDims(IVec2 dimensions);

		void render(const Framebuffer& buffer);
		void render(Uint32 texture_id, IVec2 dimensions);
		
	private:
		void initQuadData();
		void render(RenderData data);

	private:
		bool m_use_texture_dims;
		IVec2 m_viewport_dims;
		Uint32 m_quad_vao_id;

		std::unique_ptr<QuadShader> m_shader_ptr;
};

inline Uint32 textureFromColorBuffer(void* data, IVec2 dims){
	/*
	Given an image size and an array of color data, create an opengl texture
	and return its id.

	TODO: Move this into the resource manager
	*/

	// Declare new texture resource
	Uint32 texture_id;
	glGenTextures(1, &texture_id);
	glBindTexture(GL_TEXTURE_2D, texture_id);

	// Wrapping and magnification options
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 
		GL_NEAREST);
		//GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, 
		GL_NEAREST);
		//GL_LINEAR);  // Mipmapping not meaningful when magnifying
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, dims.x, dims.y, 
		0, GL_RGB, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);

	return texture_id;
}
