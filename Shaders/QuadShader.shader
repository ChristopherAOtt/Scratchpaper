#DEFINE VERTEX_SHADER
#version 410 core
layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec2 in_uv;

out vec2 uv;
uniform int should_flip_vertical;

void main(){
	gl_Position = vec4(in_pos, 1.0); 
	uv = in_uv;
	uv.y = abs(should_flip_vertical - in_uv.y);
}

#DEFINE FRAGMENT_SHADER
#version 410 core

out vec4 FragColor;

in vec2 uv;

uniform sampler2D in_texture;
uniform sampler2D in_depth;

void main(){
	FragColor = vec4(texture(in_texture, uv).xyz, 1.0);
}
