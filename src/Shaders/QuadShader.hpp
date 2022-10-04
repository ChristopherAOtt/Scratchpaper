#pragma once

#include "BaseShader.hpp"

class QuadShader : public BaseShader{
	public:
		QuadShader();
		~QuadShader();

		void bindAttributes();
		void saveUniformLocations();

		Int32 colorLocation();
		Int32 depthLocation();
		void shouldFlipVertical(bool value);

	private:
		// TODO: Move these into a map of to avoid hardcoding
		Int32 m_loc_color_tex;
		Int32 m_loc_depth_tex;
		Int32 m_loc_should_flip_vertical;
};
