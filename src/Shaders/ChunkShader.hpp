#pragma once

#include "BaseShader.hpp"

class ChunkShader : public BaseShader{
	public:
		ChunkShader();
		~ChunkShader();

		void bindAttributes();
		void saveUniformLocations();

	private:

};
