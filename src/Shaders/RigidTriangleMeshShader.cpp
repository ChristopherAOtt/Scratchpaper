#include "RigidTriangleMeshShader.hpp"

RigidTriangleMeshShader::RigidTriangleMeshShader(){

}

RigidTriangleMeshShader::~RigidTriangleMeshShader(){

}

//-----------------------------------------------------------------------------

void RigidTriangleMeshShader::bindAttributes(){
	bindAttribute(0, "in_pos");
	bindAttribute(1, "in_uv");
	bindAttribute(2, "in_normal");
	//bindAttribute(3, "in_tangent");
	//bindAttribute(4, "in_bitangent");
}

void RigidTriangleMeshShader::saveUniformLocations(){
	m_loc_time = BaseShader::getUniformLocation("time");
	m_loc_model_matrix = BaseShader::getUniformLocation("model_matrix");
	m_loc_view_matrix = BaseShader::getUniformLocation("view_matrix");
	m_loc_projection_matrix = BaseShader::getUniformLocation(
		"projection_matrix"
	);
	m_loc_clip_plane = BaseShader::getUniformLocation("clip_plane");
	m_loc_fog_color = BaseShader::getUniformLocation("fog_color");
	m_loc_camera_pos = BaseShader::getUniformLocation("camera_pos");

	// Texture maps
	this -> use();
	glUniform1i(glGetUniformLocation(m_shader_id, "map_diffuse"), 0);
	//glUniform1i(glGetUniformLocation(m_shader_id, "map_normal"), 1);
	this -> stop();
}

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
