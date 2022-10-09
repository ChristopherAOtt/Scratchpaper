#DEFINE VERTEX_SHADER
#version 410 core
layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec2 in_uv;
layout (location = 2) in uint in_blocktype;
layout (location = 3) in uint in_normal_index;

out vec3 world_coord;
out vec2 uv;
flat out uint blocktype;
flat out uint normal_index;

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
	blocktype = in_blocktype;
	normal_index = in_normal_index;
}

#DEFINE FRAGMENT_SHADER
#version 410 core

out vec4 FragColor;

in vec3 world_coord;
in vec2 uv;
flat in uint blocktype;
flat in uint normal_index;

uniform float time;
uniform vec3 fog_color;
uniform vec3 camera_pos;

// Environment lighting
uniform vec3 sun_dir;
uniform vec3 sun_color;
uniform vec3 ambient_color;

const float PI = 3.141592;
const float TAU = PI * 2;
const int NUM_BLOCK_TYPES = 6;
vec3 block_colors[] = vec3[](
	vec3(1.0, 0.0, 0.0), // Empty
	vec3(1.0, 0.0, 0.0), // Lookup
	vec3(1.0, 0.0, 0.0), // Air
	vec3(0.10, 0.95, 0.00), //Grass
	vec3(0.50, 0.25, 0.05), //Dirt
	vec3(0.30, 0.30, 0.30) //Stone
);

float far_plane = 250;

void main(){
	vec3 output_color;
	
	/*
	For normal block rendering
	*/
	vec3 block_color = block_colors[blocktype % NUM_BLOCK_TYPES];
	float distance = pow(length(world_coord - camera_pos) / far_plane, 3.5);
	float fog_blend = max(min(distance, 1), 0);
	output_color = mix(block_color, fog_color, fog_blend);

	/*
	For scaled voxel blocks
	*/
	/*
	float size = 1;
	float scale = 1;
	vec2 uv = world_coord.xy / vec2(scale);
    vec2 tiled_uv = fract(uv * size);
    vec2 square = abs(tiled_uv * 2.0 - 1.0);
    vec2 sharp_square = step(0.8, square);
    float result = sharp_square.x + sharp_square.y;
    output_color = vec4(1.0).xyz * result;
    
    float size = 1;
	float scale = 4;
	vec3 uv = world_coord / vec3(scale);
    vec3 tiled_uv = fract(uv * size);
    vec3 square = abs(tiled_uv * 2.0 - 1.0);
    vec3 sharp_square = step(0.90, square);
    float result = min(max(sharp_square.x, sharp_square.y), sharp_square.z);
    output_color = vec4(1.0).xyz * result;

    vec2 v0 = step(0.9, abs(tiled_uv.yz * 2 - 1));
    vec2 v1 = step(0.9, abs(tiled_uv.xz * 2 - 1));
    vec2 v2 = step(0.9, abs(tiled_uv.xy * 2 - 1));
    float intensity = length(v0 + v1 + v2) / 3.0;
    //intensity = step(cos(time / 5) * 2, intensity);
    output_color = vec3(length(abs(v0 - v1) - v2));
    */

	float line_intensity = 0.0;
	for(int i = 0; i < 3; ++i){
		line_intensity += pow(abs(-0.5 + fract(world_coord[i] / 4)), 5);
	}
	line_intensity += 0.90;
	line_intensity = 1 - pow(line_intensity, 50);
	//output_color = vec3(line_intensity);
	output_color += vec3((1 - line_intensity) + 0.15);


	/*
	Gamma correction
	*/
	/*
	float gamma = 2.2;
	output_color = pow(output_color, vec3(1.0 / gamma));
	vec3 ramp = vec3(1, 1, 1);
	vec3 adjusted = vec3(
		pow(output_color.x, ramp.x),
		pow(output_color.y, ramp.y),
		pow(output_color.z, ramp.z)
	);
	*/

	FragColor = vec4(output_color, 1.0);
}
