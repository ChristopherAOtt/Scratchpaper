#include "Window.hpp"



//-------------------------------------------------------------------------------------------------
// Callbacks
// TODO: Figure out a way to use callbacks without globals
//-------------------------------------------------------------------------------------------------
// These indicate that something got changed and the window class should re-query info
bool global_window_resized = false;

void resizeCallback(GLFWwindow* window, int width, int height){
	glViewport(0, 0, width, height);
	global_window_resized = true;
}

void errorCallback(int error, const char* text) {
	printf("Error Callback: %i, '%s'\n", error, text);
}

void shouldCloseWindowCallback(GLFWwindow* window){
	// Unused for now
}

//-------------------------------------------------------------------------------------------------
// Window
//-------------------------------------------------------------------------------------------------
Window::Window(){
	m_is_valid = false;
	m_fullscreen_resolution = {0, 0};
	m_window_dims = {0, 0};
}

Window::~Window(){
	if(m_is_valid){
		terminateWindow();
	}
}

bool Window::initWindow(){
	glfwSetErrorCallback(errorCallback);
	if(!glfwInit()){
		printf("GLFW initialization failed!");
		m_is_valid = false;
		return false;
	}

	// Set window settings, then try to open a window. 
	// ATTRIBUTION: Reference implementation
	// https://www.glfw.org/faq.html
	constexpr bool should_fullscreen = false;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Prevent OSX crash
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	
	// Get the primary monitor's info and save it for later in case the
	// user decides to fullscreen the window.
	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(monitor);
	m_fullscreen_resolution = {mode->width, mode->height};
	if(!should_fullscreen){
		monitor = NULL;
	}

	// Actually create the window
	GLFWwindow* window = glfwCreateWindow(
		DEFAULT_WIN_WIDTH, DEFAULT_WIN_HEIGHT, 
		"The Spaghetti Monster", 
		monitor, NULL);
	if(window == NULL){
		printf("ERROR: Failed to create GLFW window");
		glfwTerminate();
		return false;
	}
	m_window_dims = {DEFAULT_WIN_WIDTH, DEFAULT_WIN_HEIGHT};

	// Now that the window is open, start applying settings
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, resizeCallback);
	//glfwSetWindowCloseCallback(window, shouldCloseWindowCallback);
	glViewport(0, 0, DEFAULT_WIN_WIDTH, DEFAULT_WIN_HEIGHT);
	if(glewInit() != GLEW_OK){
		printf("glewInit() failed!");
		m_is_valid = false;
		return false;
	}
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
	glEnable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);

	m_is_valid = (window != NULL);
	m_window = window;
	return true;
}

void Window::terminateWindow(){
	m_is_valid = false;
	glfwTerminate();
}

bool Window::isValid(){
	return m_is_valid;
}

bool Window::shouldClose(){
	return glfwWindowShouldClose(m_window);
}

IVec2 Window::getWindowDimensions(){
	/*
	Only re-query if the resize callback got called.
	*/
	if(global_window_resized){
		glfwGetWindowSize(m_window, &m_window_dims.x, &m_window_dims.x);
		global_window_resized = false;
	}
	
	return m_window_dims;
}

bool Window::isKeyInState(KeyEventType type, int keycode){
	// TODO: Limit to valid keys

	int key_value = glfwGetKey(m_window, keycode);
	if(type == KeyEventType::KEY_PRESSED){
		return key_value == GLFW_PRESS;
	}else if(type == KeyEventType::KEY_HELD){
		return key_value == GLFW_REPEAT;
	}else if(type == KeyEventType::KEY_RELEASED){
		return key_value == GLFW_RELEASE;
	}
	
	return false;
}

std::vector<InputEvent> Window::getInputEvents(){
	/*
	Gathers input from the GLFW window and returns a list of InputEvent structs
	*/

	std::vector<InputEvent> events;

	//-------------------------------------------------------
	// Keyboard Inputs
	//-------------------------------------------------------
	// YAGNI: More efficient query method
	KeyEventType queried_key_types[] = {
		KeyEventType::KEY_PRESSED,
		KeyEventType::KEY_HELD,

		// Seems to fire for any non-pressed key
		//KeyEventType::KEY_RELEASED  
	};
	for(KeyEventType keytype : queried_key_types){
		// 65 - 90 is ASCII range
		// NOTE: 31 and below are invalid
		for(int keycode = 32; keycode <= 348; ++keycode){ 
			if(isKeyInState(keytype, keycode)){
				InputEvent new_event;
				new_event.source_type = InputSourceType::DEVICE_KEYBOARD;
				new_event.keyboard.event_type = keytype;
				new_event.keyboard.keycode = keycode;

				events.push_back(new_event);
			}
		}
	}

	//-------------------------------------------------------
	// Mouse Inputs
	//-------------------------------------------------------
	// TODO: Actual mouse inputs

	//-------------------------------------------------------
	// Controller Type1 Inputs
	//-------------------------------------------------------
	// TODO: Actual controller inputs

	// Catch configuration issues before they go any further
	for(InputEvent event : events){
		assert(event.source_type != InputSourceType::DEVICE_INVALID);
		assert(event.source_type != InputSourceType::DEVICE_UNRECOGNIZED);
	}

	return events;
}

void Window::setFullscreenState(bool is_fullscreen){
	GLFWmonitor* monitor = NULL;
	int new_width = m_window_dims.x;
	int new_height = m_window_dims.y;
	if(is_fullscreen){
		monitor = glfwGetPrimaryMonitor();
		new_width = m_fullscreen_resolution.x;
		new_height = m_fullscreen_resolution.y;
	}
	int x_pos = 0;
	int y_pos = 0;
	int refresh_rate = GLFW_DONT_CARE;

	glfwSetWindowMonitor(m_window, monitor, 
		x_pos, y_pos,
		new_width, new_height,
		refresh_rate);
}

void Window::swapBuffers(){
	glfwSwapBuffers(m_window);
}

void Window::pollEvents(){
	glfwPollEvents();

	if(glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS){
		glfwSetWindowShouldClose(m_window, true);
		return;
	}
}

void Window::setClearColor(FVec3 color){
	glClearColor(color.x, color.y, color.z, 1);	
}

void Window::clear(){	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
