#DEFINE VERTEX_SHADER
#version 330 core
layout (location = 0) in vec3 in_position;
layout (location = 1) in vec2 in_uv;
layout (location = 2) in vec3 in_color;

out vec3 vertex_color;
out vec3 world_coord;

uniform mat4 matrix_model;
uniform mat4 matrix_view;
uniform mat4 matrix_projection;

void main(){
	gl_Position = matrix_projection * matrix_view * matrix_model * vec4(in_position, 1.0f);
	vertex_color = in_color;
	vec4 world_pos = matrix_model * vec4(in_position, 1.0);
	world_coord = world_pos.xyz;
}

#DEFINE FRAGMENT_SHADER
#version 330 core

out vec4 FragColor;

in vec3 vertex_color;
in vec3 world_coord;

uniform float time;
uniform vec3 pos_camera;
uniform vec3 fog_color;

// Hardcoded. 
// TODO: Move to uniform.
float far_plane = 750;  // 250

void main(){
	float distance = pow(length(world_coord - pos_camera) / far_plane, 3.5);
	float fog_blend = max(min(distance, 1), 0);
	vec3 mixed_color = mix(vertex_color, fog_color, fog_blend);
	FragColor = vec4(mixed_color, 1.0);
}
