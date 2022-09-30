#pragma once

#include <stdio.h>
#include <vector>

#include "GLIncludes.hpp"
#include "Primitives.hpp"
#include "InputEvents.hpp"

constexpr static int DEFAULT_WIN_WIDTH = 1000; // 1280
constexpr static int DEFAULT_WIN_HEIGHT = 1000; // 800
class Window{
	/*
	Passthrough functions to avoid letting GLFW functionality
	leak out into the rest of the codebase. 
	*/
	public:
		Window();
		~Window();

		// Give option to manually init and terminate
		bool initWindow();
		void terminateWindow();

		// Query info about the window state
		bool isValid();
		bool shouldClose();
		IVec2 getWindowDimensions();

		// Query for different input info
		bool isKeyInState(KeyEventType type, int keycode);
		//FVec2 currentMousePosition();
		//bool isJoystickInState(InputEventType state);

		// Passthrough functions
		void setFullscreenState(bool is_fullscreen);
		void swapBuffers();
		void pollEvents();  // WARNING: Needs to run or the window never opens
		void setClearColor(FVec3 color);
		void clear();

	private:
		GLFWwindow* m_window;
		bool m_is_valid;
		IVec2 m_window_dims;
		IVec2 m_fullscreen_resolution;
};
