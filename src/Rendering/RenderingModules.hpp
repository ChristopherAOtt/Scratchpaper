#pragma once

#include "SimCache.hpp"
#include "Camera.hpp"


//-------------------------------------------------------------------------------------------------
// Rendering Module Base Class
//-------------------------------------------------------------------------------------------------
struct RendererSettings{
	/*
	Packages 
	*/
	float placeholder;
};

class RenderingModule{
	public:
		RenderingModule();
		~RenderingModule();

		// TODO: Make this virtual once I can figure out the linker error.
		void render(const SimCache& state, const Camera& camera, 
			const RendererSettings& settings);

	private:

};


