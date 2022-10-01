#pragma once

#include "BaseShader.hpp"

class RigidTriangleMeshShader : public BaseShader{
	public:
		RigidTriangleMeshShader();
		~RigidTriangleMeshShader();

		void bindAttributes();
		void saveUniformLocations();

	private:

};
