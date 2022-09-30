#include "BaseShader.hpp"

//-------------------------------------------------------------------------------------------------
// Helper functions
//-------------------------------------------------------------------------------------------------
glm::mat4 toGlmMatrix(FMat4 matrix){
	glm::mat4 output;
	// glm::mat4 x = glm::transpose(x);
	memcpy(glm::value_ptr(output), &matrix, sizeof(float) * 16);
	return output;
}

//-------------------------------------------------------------------------------------------------
// Base Shader
//-------------------------------------------------------------------------------------------------
BaseShader::BaseShader(){

}

BaseShader::~BaseShader(){
	// TODO: ensure that nothing gets double-deleted. Make destroy() private?
	this -> destroy();
}

//-----------------------------------------------------------------------------
void BaseShader::compileShaders(std::string vert_text, std::string frag_text){
	/*
	ATTRIBUTION: This function makes heavy reference to:
	https://learnopengl.com/Getting-started/Hello-Triangle
	*/
	m_is_valid = true; // Gets set to false in validate() if something fails
	m_shader_id = glCreateProgram();
	unsigned int vert_handle, frag_handle;
	const char* v_cstring = vert_text.c_str();
	const char* f_cstring = frag_text.c_str();

	// 
	bindAttributes();

	// Vertex shader
	vert_handle = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vert_handle, 1, &v_cstring, NULL);
	glCompileShader(vert_handle);
	validate(vert_handle, "Vertex Shader", vert_text);
	
	// Fragment shader
	frag_handle = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(frag_handle, 1, &f_cstring, NULL);
	glCompileShader(frag_handle);
	validate(frag_handle, "Fragment Shader", frag_text);
	
	// Link the shaders into the final program
	glAttachShader(m_shader_id, vert_handle);
	glAttachShader(m_shader_id, frag_handle);
	glLinkProgram(m_shader_id);
	validate(m_shader_id, "Shader Program", vert_text + "\n\n" + frag_text);
	
	// Now that we have a program we can store the uniform locations
	saveUniformLocations();

	// Clean up this mess
	// This works since the final program contains the information from
	// both shaders. We don't need to keep these anymore
	glDetachShader(m_shader_id, vert_handle);
	glDetachShader(m_shader_id, frag_handle);
	glDeleteShader(vert_handle);
	glDeleteShader(frag_handle);
}

// This is useful for the derived classes
GLuint BaseShader::getUniformLocation(std::string uniform_name){
	return glGetUniformLocation(m_shader_id, uniform_name.c_str());
}

// This is useful for the derived classes
// Allows the derived to enforce a consistent layout across shaders
void BaseShader::bindAttribute(int attribute_index, std::string var_name){
	glBindAttribLocation(m_shader_id, attribute_index, var_name.c_str());
}

// Virtual functions
void BaseShader::saveUniformLocations(){}  // Changes depending on the shader
void BaseShader::bindAttributes(){}  // Changes depending on the shader

//-----------------------------------------------------------------------------

void BaseShader::setMat4(int location, FMat4 matrix) const{
	glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(toGlmMatrix(matrix)));
}

void BaseShader::setInt(int location, int value) const{
	glUniform1i(location, value);
}

void BaseShader::setFloat(int location, float value) const{
	glUniform1f(location, value);
}

//-----------------------------------------------------------------------------

bool BaseShader::isValid(){
	return m_is_valid;
}

void BaseShader::use(){
	glUseProgram(m_shader_id);
}

void BaseShader::stop(){
	glUseProgram(0);
}

void BaseShader::destroy(){
	this -> stop();
	glDeleteProgram(m_shader_id);
}

//-----------------------------------------------------------------------------
void BaseShader::validate(unsigned int id, std::string type, std::string code){
	/*
	Checks for errors in the compilation of a shader
	https://learnopengl.com/Getting-started/Hello-Triangle
	*/
	int success; // Not bool due to the C interface's use of ints
	int buffer_size = 512;
	char infolog_buffer[buffer_size];
	
	if(type != "Shader Program"){
		// Fragment, vertex, or geometry shader issue
		glGetShaderiv(id, GL_COMPILE_STATUS, &success);
		if(!success){
			glGetShaderInfoLog(id, buffer_size, NULL, infolog_buffer);
			std::cout << "ERROR [" << type << "] COMPILATION ERROR\n" << 
				infolog_buffer << std::endl;
			std::cout << "SHADER CODE:\n-----------------------\n" << 
				code << "\n-----------------------\n";
		}
	}else{
		// General shader program issue
		glGetProgramiv(id, GL_LINK_STATUS, &success);
		if (!success){
			glGetProgramInfoLog(id, buffer_size, NULL, infolog_buffer);
			std::cout << "ERROR [" << type << "] COMPILATION ERROR\n" << 
				infolog_buffer << std::endl;
		}
	}

	// TODO: Potentially add a general error message when this fails
	m_is_valid &= success;
	std::cout << "m_is_valid : " << m_is_valid << std::endl;
}

//-----------------------------------------------------------------------------
void BaseShader::setCameraPos(FVec3 pos){
	glUniform3fv(m_loc_camera_pos, 1, &pos[0]);
}

void BaseShader::setTime(float time){
	setFloat(m_loc_time, time);
}

void BaseShader::setFogColor(FVec3 color){
	glUniform3fv(m_loc_fog_color, 1, &color[0]);
}

void BaseShader::setModelMatrix(FMat4 model_matrix){
	setMat4(m_loc_model_matrix, model_matrix);
}

void BaseShader::setViewMatrix(FMat4 view_matrix){
	setMat4(m_loc_view_matrix, view_matrix);
}

void BaseShader::setProjectionMatrix(FMat4 projection_matrix){
	setMat4(m_loc_projection_matrix, projection_matrix);
}

void BaseShader::setClipPlane(FVec4 plane_eq){
	glUniform4fv(m_loc_clip_plane, 1, (float*)&plane_eq);
}
