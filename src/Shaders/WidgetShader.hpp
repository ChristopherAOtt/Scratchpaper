#pragma once

#include "BaseShader.hpp"

class WidgetShader : public BaseShader{
	public:
		WidgetShader();
		~WidgetShader();

		void bindAttributes();
		void saveUniformLocations();

	private:

};
