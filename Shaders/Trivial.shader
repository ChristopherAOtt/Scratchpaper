#DEFINE VERTEX_SHADER
#version 330 core
layout (location = 0) in vec3 in_position;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

void main(){
	gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(in_position, 1.0f);
}

#DEFINE FRAGMENT_SHADER
#version 330 core
out vec4 FragColor;

void main(){
	FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}
