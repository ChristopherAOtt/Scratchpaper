#pragma once

#include <string>
#include <iostream>

#include "Primitives.hpp"
#include "GLIncludes.hpp"

// Base class other shaders will inherit from
class BaseShader{
	public:
		BaseShader();
		~BaseShader();
		BaseShader(const BaseShader& shader) = delete;

		// Construction functions. Can't call from the constructor directly
		// due to some of these methods being virtual. Have to make a call
		// to compileShaders() after the constructor is done.
		void compileShaders(std::string vert_text, std::string frag_text);
		GLuint getUniformLocation(std::string uniform_name);
		void bindAttribute(int attribute_index, std::string var_name);
		void virtual saveUniformLocations() = 0;
		void virtual bindAttributes() = 0;

		// Functions to set uniform values. The subclass will know the
		// locations of these uniforms.
		void setMat4(int location, FMat4 matrix) const;
		void setInt(int location, int value) const;
		void setFloat(int location, float value) const;

		// General convenience functions
		bool isValid();
		void use();
		void stop();
		void destroy();

		// General functions that will be useful for any spatial rendering
		void setTime(float time);
		void setCameraPos(FVec3 pos);
		void setModelMatrix(FMat4 model_matrix);
		void setViewMatrix(FMat4 view_matrix);
		void setProjectionMatrix(FMat4 projection_matrix);
		void setClipPlane(FVec4 plane_eq);
		void setFogColor(FVec3 color);

	protected:
		GLuint m_shader_id;
		bool m_is_valid;

		void validate(unsigned int id, std::string type, std::string code);

		// Uniform location variables
		GLuint m_loc_time;
		GLuint m_loc_model_matrix;
		GLuint m_loc_view_matrix;
		GLuint m_loc_projection_matrix;
		GLuint m_loc_clip_plane;
		GLuint m_loc_fog_color;
		GLuint m_loc_camera_pos;
};
