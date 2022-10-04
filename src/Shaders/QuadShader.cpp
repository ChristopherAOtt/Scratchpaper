#include "QuadShader.hpp"

QuadShader::QuadShader(){
	m_loc_color_tex = -1;
	m_loc_depth_tex = -1;
	m_loc_should_flip_vertical = -1;
}

QuadShader::~QuadShader(){

}

//------------------------------------------------------------------------------
void QuadShader::bindAttributes(){
	bindAttribute(0, "in_pos");
	bindAttribute(1, "in_uv");
}

void QuadShader::saveUniformLocations(){
	m_loc_color_tex = BaseShader::getUniformLocation("in_texture");
	m_loc_depth_tex = BaseShader::getUniformLocation("in_depth");
	m_loc_should_flip_vertical = BaseShader::getUniformLocation(
		"should_flip_vertical");
}

//------------------------------------------------------------------------------
Int32 QuadShader::colorLocation(){
	return m_loc_color_tex;
}

Int32 QuadShader::depthLocation(){
	return m_loc_depth_tex;
}

void QuadShader::shouldFlipVertical(bool value){
	setInt(m_loc_should_flip_vertical, value);
}
