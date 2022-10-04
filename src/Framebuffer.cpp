#include "Framebuffer.hpp"

Framebuffer::Framebuffer(IVec2 dimensions){
	m_dimensions = dimensions;
	
	m_framebuffer_object_id = 0;
	m_colorbuffer_id = 0;
	m_depthbuffer_id = 0;
}

Framebuffer::~Framebuffer(){
	glDeleteFramebuffers(1, &m_framebuffer_object_id);
	glDeleteTextures(1, &m_colorbuffer_id);
	glDeleteTextures(1, &m_depthbuffer_id);
}

void Framebuffer::generateBuffer(bool should_generate_depthbuffer){
	/*
	ATTRIBUTION: Heavy reference made to
		https://learnopengl.com/Advanced-OpenGL/Framebuffers
	*/

	// Create the framebuffer
	Uint32 framebuffer;
	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	m_framebuffer_object_id = framebuffer;

	// Get a texture ready
	Uint32 color_buffer;
	glGenTextures(1, &color_buffer);
	glBindTexture(GL_TEXTURE_2D, color_buffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_dimensions.x, m_dimensions.y, 0, 
		GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); //s
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); //s
	glBindTexture(GL_TEXTURE_2D, 0);

	// Attach said texture to currently bound framebuffer object
	glFramebufferTexture2D(
		GL_FRAMEBUFFER, 
		GL_COLOR_ATTACHMENT0, 
		GL_TEXTURE_2D, 
		color_buffer, 
		0
	);
	m_colorbuffer_id = color_buffer;

	// The process could complete right now, or we could add additional buffers
	if(should_generate_depthbuffer){
		// Need this in order to do depth testing
		Uint32 depth_buffer;
		glGenTextures(1, &depth_buffer);
		glBindTexture(GL_TEXTURE_2D, depth_buffer);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, m_dimensions.x, 
			m_dimensions.y, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D, 0);

		glFramebufferTexture2D(
			GL_FRAMEBUFFER, 
			GL_DEPTH_ATTACHMENT, 
			GL_TEXTURE_2D,
			depth_buffer, 
			0
		);

		m_depthbuffer_id = depth_buffer;
	}

	/*
	if(should_generate_another_buffer_type){
		More buffer generation code
	}
	*/

	assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::bindCurrentBuffer(){
	glViewport(0, 0, m_dimensions.x, m_dimensions.y);
	glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer_object_id);

	// Clear the buffers so we can render the image properly
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Framebuffer::unbindCurrentBuffer(){
	// Binds the default framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

IVec2 Framebuffer::getDimensions() const{
	return m_dimensions;
}

Uint32 Framebuffer::getFramebufferId() const{
	return m_framebuffer_object_id;
}

Uint32 Framebuffer::getColorbufferId() const{
	return m_colorbuffer_id;
}

Uint32 Framebuffer::getDepthbufferId() const{
	return m_depthbuffer_id;
}

