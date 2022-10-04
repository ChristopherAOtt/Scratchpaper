#pragma once

#include "Types.hpp"
#include "Primitives.hpp"
#include "GLIncludes.hpp"

class Framebuffer{
	public:
		Framebuffer(IVec2 dimensions);
		~Framebuffer();

		void generateBuffer(bool should_generate_depthbuffer);
		void bindCurrentBuffer();
		void unbindCurrentBuffer();

		IVec2 getDimensions() const;
		Uint32 getFramebufferId() const;
		Uint32 getColorbufferId() const;
		Uint32 getDepthbufferId() const;

	private:
		IVec2  m_dimensions;
		Uint32 m_framebuffer_object_id;
		Uint32 m_colorbuffer_id;
		Uint32 m_depthbuffer_id;
};
