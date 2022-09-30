#include "Camera.hpp"

Camera initDefaultCamera(){
	Camera camera;

	camera.pos = {0, 0, 0};
	camera.basis = {
		{1, 0, 0},
		{0, 1, 0},
		{0, 0, 1}
	};
	camera.fov = 90.0;
	camera.aspect_ratio = 4 / 4;  // 4/3
	camera.near_plane = 0.001;
	camera.far_plane = 1000.0;

	return camera;
}
