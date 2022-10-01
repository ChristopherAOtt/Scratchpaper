#include "Renderer.hpp"

Renderer::Renderer(){
	m_current_settings = {0.0};
}

Renderer::~Renderer(){

}

void Renderer::initOpenGL(){
	/*
	Initialization for OpenGL specific functionality
	*/
	
	m_opengl_module.initShadersFromSourceFiles();
}

void Renderer::sendInstruction(SystemInstruction instruction){
	/*
	TODO: Handle instructions related to renderer settings
	*/

	m_opengl_module.sendInstruction(instruction);
}

void Renderer::render(const SimCache& state, Camera camera){
	m_opengl_module.render(state, camera, m_current_settings);
}
