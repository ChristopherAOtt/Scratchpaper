#DEFINE VERTEX_SHADER
#version 410 core
layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec2 in_uv;
layout (location = 2) in vec3 in_normal;

out vec3 world_coord;
out vec2 uv;
out vec3 normal;

uniform mat4 model_matrix;
uniform mat4 view_matrix;
uniform mat4 projection_matrix;

uniform float time;

void main(){
	vec3 pos = in_pos;
	vec4 world_pos = model_matrix * vec4(pos, 1.0);
	gl_Position = projection_matrix * view_matrix * world_pos;

	world_coord = world_pos.xyz;
	uv = in_uv;
	normal = in_normal;
}

#DEFINE FRAGMENT_SHADER
#version 410 core

out vec4 FragColor;

in vec3 world_coord;
in vec2 uv;
in vec3 normal;

uniform float time;
uniform vec3 fog_color;
uniform vec3 camera_pos;

// Environment lighting
uniform vec3 sun_dir;
uniform vec3 sun_color;
uniform vec3 ambient_color;

float far_plane = 250;

void main(){
	vec3 output_color = sin(world_coord);
	
	/*
	Gamma correction
	*/
	float gamma = 2.2;
	output_color = pow(output_color, vec3(1.0 / gamma));
	vec3 ramp = vec3(1, 1, 1);
	vec3 adjusted = vec3(
		pow(output_color.x, ramp.x),
		pow(output_color.y, ramp.y),
		pow(output_color.z, ramp.z)
	);
	output_color = adjusted;

	FragColor = vec4(output_color, 1.0);
}
