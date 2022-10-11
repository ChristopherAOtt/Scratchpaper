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

const int CHUNK_LEN = 32;
const int VOXELS_PER_METER = 4;

const float PI = 3.141592;
const float TAU = PI * 2;
const int NUM_BLOCK_TYPES = 8;
vec3 block_colors[] = vec3[](
	// Error values. 
	vec3(1.0, 0.0, 0.0), // Empty
	vec3(1.0, 0.0, 0.0), // Lookup
	vec3(1.0, 0.0, 0.0), // Air

	// Actual, valid block types
	vec3(0.3608, 0.9529, 0.3608),  // Grass
	vec3(0.5294, 0.2431, 0.1373),  // Dirt
	vec3(0.4824, 0.4784, 0.4745),  // Stone
	vec3(0.7804, 0.7804, 0.7804),  // Concrete
	vec3(0.2784, 0.2863, 0.3201)   // Metal
);

float far_plane = 500;

void main(){
	vec3 output_color;
	
	/*
	For normal block rendering
	*/
	if(true){
		vec3 block_color = block_colors[blocktype];
		float distance = pow(length(world_coord - camera_pos) / far_plane, 3.5);
		float fog_blend = max(min(distance, 1), 0);
		output_color = mix(block_color, fog_color, fog_blend);
	}

	/*
	Grid lines
	*/
	if(true){
		float line_intensity = 0.0;
		for(int i = 0; i < 3; ++i){
			// Individual Voxels
			line_intensity += pow(abs(-0.5 + fract(world_coord[i])), 7);
			
			// 1 Meter Grid (4 voxels per meter)
			line_intensity += pow(abs(-0.5 + fract(world_coord[i] / VOXELS_PER_METER)), 8);
			
			// Chunk Boundaries
			line_intensity += pow(abs(-0.5 + fract(world_coord[i] / CHUNK_LEN)), 7);
		}
		line_intensity = clamp(line_intensity + 0.95, 0.0, 1.0);

		line_intensity = pow(line_intensity, 50);
		output_color = mix(output_color, vec3(0.0), line_intensity);
	}

	/*
	Gamma correction
	*/
	if(false){
		float gamma = 2.0;
		output_color = pow(output_color, vec3(1.0 / gamma));
		vec3 ramp = vec3(1.0, 1.0, 1.0);
		vec3 adjusted = vec3(
			pow(output_color.x, ramp.x),
			pow(output_color.y, ramp.y),
			pow(output_color.z, ramp.z)
		);
		output_color = adjusted;
	}

	FragColor = vec4(output_color, 1.0);
}
