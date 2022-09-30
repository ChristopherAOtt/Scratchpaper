#include "WidgetShader.hpp"

WidgetShader::WidgetShader(){

}

WidgetShader::~WidgetShader(){

}

//-----------------------------------------------------------------------------

void WidgetShader::bindAttributes(){
	bindAttribute(0, "in_pos");
	bindAttribute(1, "in_uv");
	bindAttribute(2, "in_color");
}

void WidgetShader::saveUniformLocations(){
	m_loc_time = BaseShader::getUniformLocation("time");
	m_loc_model_matrix = BaseShader::getUniformLocation("matrix_model");
	m_loc_view_matrix = BaseShader::getUniformLocation("matrix_view");
	m_loc_projection_matrix = BaseShader::getUniformLocation("matrix_projection");
	m_loc_camera_pos = BaseShader::getUniformLocation("pos_camera");
	m_loc_fog_color = BaseShader::getUniformLocation("fog_color");

	// Texture maps
	this -> use();
	//glUniform1i(glGetUniformLocation(m_shader_id, "map_diffuse"), 0);
	//glUniform1i(glGetUniformLocation(m_shader_id, "map_normal"), 1);
	this -> stop();
}

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------