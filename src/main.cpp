// OpenGL voxel raytracer projet
// Renders a voxel scene using raytracing
// CPU side : voxel scene generation, raytracing, image saving
// Only show a fullscreen rectangle, the voxel scene is rendered by a shader
// GPU side : voxel scene rendering, fullscreen rectangle rendering

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "utils.h"

constexpr auto dimension = 48;
bool generateCells = false;
bool updateBuffer = true;

#include "Structs.h"

shader_data s_data = { dimension, dimension, dimension };
bool willDie[dimension * dimension * dimension];
bool willGrow[dimension * dimension * dimension];

#include <random>

// Link opengl32.lib, glfw3.lib and glew32.lib
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glfw3.lib")
#pragma comment(lib, "glew32.lib")

#if 0
// The standard Conway rules ( https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life#Rules )
bool rule3D = false;
int ruleCellDiesFewerThan = 2;
int ruleCellLivesFewerThan = 4;
int ruleCellDiesMoreThan = 3;
// Gives a range for when a cell grows
int ruleCellGrowsMoreThan = 2;
int ruleCellGrowsFewerThan = 4;
#else
bool rule3D = true;
int ruleCellDiesFewerThan = 5;
int ruleCellLivesFewerThan = 7;
int ruleCellDiesMoreThan = 7;
// Gives a range for when a cell grows
int ruleCellGrowsMoreThan = 5;
int ruleCellGrowsFewerThan = 7;
#endif


int width = 800;
int height = 800;

float degtorad = 3.141592f / 180.0f;

Camera camera;

int frameSinceLastReset = 0;

bool paused = false;

void cameraMouseCallback(GLFWwindow *window, const double posX, const double posY) {

	if(paused)
		return;

	frameSinceLastReset = 0;

	const float offset_x = posX - camera.lastX;
	const float offset_y = posY - camera.lastY;

	camera.lastX = width / 2;
	camera.lastY = height / 2;

	const float sensitivity = 0.1f;

	camera.rotationY += offset_x * sensitivity;
	camera.rotationX -= offset_y * sensitivity;

	if (camera.rotationX > 89.0f)
		camera.rotationX = 89.0f;
	if (camera.rotationX < -89.0f)
		camera.rotationX = -89.0f;


	camera.direction.x = cos(camera.rotationY * degtorad) * cos(camera.rotationX * degtorad);
	camera.direction.y = sin(camera.rotationX * degtorad);
	camera.direction.z = sin(camera.rotationY * degtorad) * cos(camera.rotationX * degtorad);
	camera.front = glm::normalize(camera.direction);
	camera.right = glm::normalize(glm::cross(camera.front, camera.worldUp));
	camera.up = glm::normalize(glm::cross(camera.right, camera.front));

	glfwSetCursorPos(window, width / 2, height / 2);
}

bool useFresnel = false;
AppState* appStatePtr;

void keyPressedCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_P && action == GLFW_PRESS)
	{
		appStatePtr->useSRGB = !appStatePtr->useSRGB;
	}
	if (key == GLFW_KEY_O && action == GLFW_PRESS)
	{
		appStatePtr->useACES = !appStatePtr->useACES;
	}

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		//hide or show cursor
		if (paused) {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			paused = false;
		}
		else {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			paused = true;
		}
	}

	if(paused)
		return;

	if (key == GLFW_KEY_G && action == GLFW_PRESS)
	{
		generateCells = true;
	}

	if (key == GLFW_KEY_1 && action == GLFW_PRESS)
	{
		// Step out from the camera to try to find the last non-empty block, for a maximum sensible distance.
		int x = -1;
		int y = 0;
		int z = 0;

		for (float dist = 0.0f; dist <= 6.0f; dist += 0.5f)
		{
			glm::vec3 wantPos = camera.position + (camera.front * dist);
			wantPos.x -= 1.0f;
			wantPos.x += (float)(dimension / 4);
			if (wantPos.x >= 0.0f && wantPos.y >= 0.0f && wantPos.z >= 0.0f &&
				wantPos.x < float(dimension) && wantPos.y < float(dimension) && wantPos.z < float(dimension))
			{
				int tx = int(wantPos.x);
				int ty = int(wantPos.y);
				int tz = int(wantPos.z);

				int cell = s_data.getCell(tx, ty, tz);
				if (!cell)
				{
					x = tx;
					y = ty;
					z = tz;
				}
				else
				{
					break;
				}
			}
		}

		if (x >= 0)
		{
			static int col = 1;
			col += 1;
			col = (col % 9) + 1;
			s_data.setCell(col, x, y, z);
			updateBuffer = true;
		}
	}

	if (key == GLFW_KEY_0 && action == GLFW_PRESS)
	{
		// Step out from the camera to try to find the first non-empty block, for a maximum sensible distance.
		for (float dist = 0.0f; dist <= 6.0f; dist += 0.5f)
		{
			glm::vec3 wantPos = camera.position + (camera.front * dist);
			wantPos.x -= 1.0f;
			wantPos.x += (float)(dimension / 4);
			if (wantPos.x >= 0.0f && wantPos.y >= 0.0f && wantPos.z >= 0.0f &&
				wantPos.x < float(dimension) && wantPos.y < float(dimension) && wantPos.z < float(dimension))
			{
				int tx = int(wantPos.x);
				int ty = int(wantPos.y);
				int tz = int(wantPos.z);

				int cell = s_data.getCell(tx, ty, tz);
				if (cell)
				{
					s_data.setCell(0, tx, ty, tz);
					updateBuffer = true;
					break;
				}
			}
		}
	}

	frameSinceLastReset = 0;

	if (key == GLFW_KEY_F && action == GLFW_PRESS) {
		useFresnel = !useFresnel;
	}

	if (action == GLFW_PRESS) {
		if (key == GLFW_KEY_W)
			camera.keys.w = true;
		if (key == GLFW_KEY_A)
			camera.keys.a = true;
		if (key == GLFW_KEY_S)
			camera.keys.s = true;
		if (key == GLFW_KEY_D)
			camera.keys.d = true;
		if (key == GLFW_KEY_SPACE)
			camera.keys.space = true;
		if (key == GLFW_KEY_LEFT_CONTROL)
			camera.keys.left_control = true;
	}
	if (action == GLFW_RELEASE) {
		if (key == GLFW_KEY_W)
			camera.keys.w = false;
		if (key == GLFW_KEY_A)
			camera.keys.a = false;
		if (key == GLFW_KEY_S)
			camera.keys.s = false;
		if (key == GLFW_KEY_D)
			camera.keys.d = false;
		if (key == GLFW_KEY_SPACE)
			camera.keys.space = false;
		if (key == GLFW_KEY_LEFT_CONTROL)
			camera.keys.left_control = false;
	}
}


void initCamera() {
	camera.position = glm::vec3(0.0f, 0.5f, -50.0f);
	camera.direction = glm::vec3(0.0f, 0.0f, 1.0f);
	camera.rotationX = 0.0f;
	camera.rotationY = 90.0f;
	camera.worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
	camera.front = glm::normalize(camera.direction);
	camera.right = glm::normalize(glm::cross(camera.front, camera.worldUp));
	camera.up = glm::normalize(glm::cross(camera.right, camera.front));
	camera.lastX = width / 2;
	camera.lastY = height / 2;
	camera.projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 100.0f);
	camera.view = glm::lookAt(camera.position, camera.position + camera.front, camera.up);
	camera.keys = { };
}

std::string loadFileDirect(std::string filename) {
	std::ifstream t(filename);
	std::stringstream buffer;
	buffer << t.rdbuf();
	return buffer.str();
}

// Tries to find a file by searching backwards in the path
std::string loadFile(std::string filename) {
	int tries = 0;
	while (tries++ < 5)
	{
		std::string ret;
		ret = loadFileDirect(filename);
		if (!ret.empty())
		{
			return ret;
		}
		filename = "../" + filename;
	}
	return "";
}

void updateCamera(float deltaTime) {
	const float cameraSpeed = 5.0f * deltaTime;
	if (camera.keys.w)
		camera.position += cameraSpeed * camera.front;
	if (camera.keys.s)
		camera.position -= cameraSpeed * camera.front;
	if (camera.keys.a)
		camera.position -= camera.right * cameraSpeed;
	if (camera.keys.d)
		camera.position += camera.right * cameraSpeed;
	if (camera.keys.space)
		camera.position += cameraSpeed * camera.worldUp;
	if (camera.keys.left_control)
		camera.position -= cameraSpeed * camera.worldUp;

	if (camera.keys.w || camera.keys.a || camera.keys.s || camera.keys.d || camera.keys.space || camera.keys.left_control) {
		frameSinceLastReset = 0;
	}
}

bool initOpenGL(AppState *appState) {
	if (!glfwInit()) {
		std::cerr << "Failed to initialize GLFW" << std::endl;
		return 0;
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Create a windowed mode window and its OpenGL context
	char title[128];
	sprintf(title, "Conway-Piper/%d/%d/%d/%d/%d/%d", rule3D?3:2, ruleCellDiesFewerThan, ruleCellLivesFewerThan , ruleCellDiesMoreThan, ruleCellGrowsMoreThan, ruleCellGrowsFewerThan);
	appState->window = glfwCreateWindow(width, height, title, NULL, NULL);
	if (!appState->window) {
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return 0;
	}
	// Make the window's context current
	glfwMakeContextCurrent(appState->window);
	glfwSwapInterval(0);

	// Initialize glad
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "Failed to initialize GLAD" << std::endl;
		return 0;
	}

	return 1;
}

void initFrameBuffer(Framebuffer& buff) {
	buff.width = width;
	buff.height = height;
	glGenFramebuffers(1, &buff.fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, buff.fbo);

	glGenTextures(1, &buff.colorTexture);
	glBindTexture(GL_TEXTURE_2D, buff.colorTexture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, buff.width, buff.height, 0, GL_RGBA, GL_FLOAT, nullptr);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, buff.colorTexture, 0);

	glGenTextures(1, &buff.bloomTexture);
	glBindTexture(GL_TEXTURE_2D, buff.bloomTexture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, buff.width, buff.height, 0, GL_RGBA, GL_FLOAT, nullptr);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, buff.bloomTexture, 0);

	GLenum drawBuffers[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, drawBuffers);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cerr << "Failed to create framebuffer 1" << std::endl;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void setWillDie(bool flag, int w, int h, int d)
{
	willDie[w + (s_data.mapw * h) + (s_data.mapw * s_data.maph * d)] = flag;
}

void setWillGrow(bool flag, int w, int h, int d)
{
	willGrow[w + (s_data.mapw * h) + (s_data.mapw * s_data.maph * d)] = flag;
}

int main()
{
	srand(time(NULL));

	AppState appState;
	appStatePtr = &appState;

	if (!initOpenGL(&appState))
		return -1;


	// Create the quad to render the voxel scene
	float vertices[] = {
		-1.0f, -1.0f, 0.0f,
		 1.0f, -1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f,
		 1.0f, -1.0f, 0.0f,
		 1.0f,  1.0f, 0.0f
	};

	glGenVertexArrays(1, &appState.vao);
	glGenBuffers(1, &appState.vbo);

	glBindVertexArray(appState.vao);
	glBindBuffer(GL_ARRAY_BUFFER, appState.vbo);

	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// Load the shader from a file
	appState.shader = glCreateProgram();
	appState.quad_shader = glCreateProgram();
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	GLuint quad_fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);


	// Loading the shaders and checking for compilation errors

	std::string vertexShaderSource = loadFile("src/vertex.glsl");
	std::string fragmentShaderSource = loadFile("src/fragment.glsl");
	std::string quadFragShaderSource = loadFile("src/quad_frag.glsl");

	// TODO: 

	const char* vertexShaderSourceC = vertexShaderSource.c_str();
	const char* fragmentShaderSourceC = fragmentShaderSource.c_str();
	const char* quadFragShaderSourceC = quadFragShaderSource.c_str();

	glShaderSource(vertexShader, 1, &vertexShaderSourceC, NULL);
	glCompileShader(vertexShader);
	int success;
	char infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cerr << "Failed to compile vertex shader: " << infoLog << std::endl;
	}

	glShaderSource(fragmentShader, 1, &fragmentShaderSourceC, NULL);
	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cerr << "Failed to compile fragment shader: " << infoLog << std::endl;
	}

	glShaderSource(quad_fragmentShader, 1, &quadFragShaderSourceC, NULL);
	glCompileShader(quad_fragmentShader);
	glGetShaderiv(quad_fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cerr << "Failed to compile quad fragment shader: " << infoLog << std::endl;
	}

	glAttachShader(appState.shader, vertexShader);
	glAttachShader(appState.shader, fragmentShader);
	glLinkProgram(appState.shader);
	glGetProgramiv(appState.shader, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(appState.shader, 512, NULL, infoLog);
		std::cerr << "Failed to link shader program: " << infoLog << std::endl;
	}

	glAttachShader(appState.quad_shader, vertexShader);
	glAttachShader(appState.quad_shader, quad_fragmentShader);
	glLinkProgram(appState.quad_shader);
	glGetProgramiv(appState.quad_shader, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(appState.shader, 512, NULL, infoLog);
		std::cerr << "Failed to link quad shader program: " << infoLog << std::endl;
	}

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	glDeleteShader(quad_fragmentShader);

	// Init shader storage buffer

	for (int i = 0; i < s_data.mapw; i++) {
		for (int j = 0; j < s_data.maph; j++) {
			for (int k = 0; k < s_data.mapd; k++) {
#if 0
				s_data.data[i + s_data.mapw * j + s_data.mapw * s_data.maph * k] = (rand() % 9 + 1) * ((i==0||i== s_data.mapw-1||j==0||j== s_data.maph-1||k==0||k== s_data.mapd-1) || (rand() % 10 == 0));
#else
				s_data.data[i + s_data.mapw * j + s_data.mapw * s_data.maph * k] = 0;
#endif
			}
		}
	}

	for (int i = 0; i < 10; i++) {
		s_data.palette[i] = glm::vec3(rand() / (float)RAND_MAX, rand() / (float)RAND_MAX, rand() / (float)RAND_MAX);
	}

	glGenBuffers(1, &appState.ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, appState.ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(shader_data), &s_data, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, appState.ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	// Init the frame buffers

	Framebuffer* fb1 = &appState.fb1;
	initFrameBuffer(appState.fb1);

	Framebuffer* fb2 = &appState.fb2;
	initFrameBuffer(appState.fb2);

	// Init the camera

	bool s_data_changed = false;

	int spp = 1;
	int bounces = 30;

	initCamera();

	float lastChangeTime = glfwGetTime();

	// Callbacks

	glfwSetCursorPosCallback(appState.window, cameraMouseCallback);
	glfwSetKeyCallback(appState.window, keyPressedCallback);

	float lastTime = glfwGetTime();
	float lastTimeFPS = glfwGetTime();
	int frameCount = 0;
	int frame = 0;

	// Set starting state
	// https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life#Pattern_taxonomy
	// 2D
#if 0
	// Blinker
	int x = dimension / 2;
	int y = dimension / 2;
	s_data.setCell(1, 1 + x, y, 0);
	s_data.setCell(1, 2 + x, y, 0);
	s_data.setCell(1, 3 + x, y, 0);
#endif

#if 0
	// Toad
	int x = dimension / 2;
	int y = dimension / 2;
	s_data.setCell(1, 2 + x, 1 + y, 0);
	s_data.setCell(1, 3 + x, 1 + y, 0);
	s_data.setCell(1, 4 + x, 1 + y, 0);
	s_data.setCell(1, 1 + x, 2 + y, 0);
	s_data.setCell(1, 2 + x, 2 + y, 0);
	s_data.setCell(1, 3 + x, 2 + y, 0);
#endif

#if 0
	// Glider
	int x = dimension / 2;
	int y = dimension / 2;
	s_data.setCell(1, 2 + x, 3 + y, 0);
	s_data.setCell(1, 1 + x, 2 + y, 0);
	s_data.setCell(1, 3 + x, 1 + y, 0);
	s_data.setCell(1, 2 + x, 1 + y, 0);
	s_data.setCell(1, 1 + x, 1 + y, 0);
#endif

	// 3D
#if 1
	// Glider two depth
	int x = dimension / 2;
	int y = dimension / 2;
	for (int z = 8; z <= 9; z++)
	{
		s_data.setCell(z-5, 2 + x, 3 + y, z);
		s_data.setCell(z-5, 1 + x, 2 + y, z);
		s_data.setCell(z-5, 3 + x, 1 + y, z);
		s_data.setCell(z-5, 2 + x, 1 + y, z);
		s_data.setCell(z-5, 1 + x, 1 + y, z);
}
#endif

#if 0
	// Larger longer ship two depth
	int x = dimension / 2;
	int y = dimension / 2;
	for (int z = 8; z <= 9; z++)
	{
		s_data.setCell(z-5, x - 1, y + 0, z);
		s_data.setCell(z-5, x - 0, y + 1, z);
		s_data.setCell(z-5, x - 1, y + 1, z);
		s_data.setCell(z-5, x - 2, y + 1, z);
		s_data.setCell(z-5, x - 2, y + 2, z);
		s_data.setCell(z-5, x - 0, y + 3, z);
		s_data.setCell(z-5, x - 1, y + 3, z);
		s_data.setCell(z-5, x - 0, y + 4, z);
	}
#endif

#if 0
	// Bracket glider
	ruleCellDiesFewerThan = 4;
	ruleCellLivesFewerThan = 5;
	ruleCellDiesMoreThan = 5;
	ruleCellGrowsMoreThan = 4;
	ruleCellGrowsFewerThan = 6;

	int x = dimension / 2;
	int y = dimension - 5;
	s_data.setCell(1, x + 0, y + 2, 8);
	s_data.setCell(1, x + 3, y + 2, 8);
	s_data.setCell(1, x + 0, y + 1, 8);
	s_data.setCell(1, x + 3, y + 1, 8);
	s_data.setCell(1, x + 1, y + 0, 8);
	s_data.setCell(1, x + 2, y + 0, 8);
	//
	s_data.setCell(2, x + 1, y + 2, 9);
	s_data.setCell(2, x + 2, y + 2, 9);
	s_data.setCell(2, x + 1, y + 1, 9);
	s_data.setCell(2, x + 2, y + 1, 9);
#endif

#if 0
	// Pulsar
	ruleCellDiesFewerThan = 5;
	ruleCellLivesFewerThan = 6;
	ruleCellDiesMoreThan = 5;
	ruleCellGrowsMoreThan = 3;
	ruleCellGrowsFewerThan = 6;

	int x = dimension / 2;
	int y = dimension / 2;
	int z = dimension / 2;
	s_data.setCell(1, x, y, z);
	s_data.setCell(2, x - 1, y, z);
	s_data.setCell(3, x + 1, y, z);
	s_data.setCell(4, x, y - 1, z);
	s_data.setCell(5, x, y + 1, z);
	s_data.setCell(6, x, y, z - 1);
	s_data.setCell(7, x, y, z + 1);
#endif


	// Main rendering / event loop
	while (!glfwWindowShouldClose(appState.window)) {

		// Update the camera
		float currentTime = glfwGetTime();
		float deltaTime = currentTime - lastTime;
		lastTime = currentTime;

		frameCount++;
		if (currentTime > lastTimeFPS + 1) {
			std::cout << "FPS : " << frameCount << std::endl << "SPP : " << spp << std::endl << std::endl;
			frameCount = 0;
			lastTimeFPS = currentTime;
		}

		if (abs(1.0f/deltaTime - 60) > 2) {
			float sampleTime = deltaTime / spp;
			float newSPP = 1.0f / (sampleTime * 60);
			spp = (int)(spp + (newSPP - spp) * 0.1f);
			if(spp < 1) spp = 1;
			if (spp > 500) spp = 500;
		}

		updateCamera(deltaTime);

		fb1 = (frame % 2 == 0) ? &appState.fb1 : &appState.fb2;
		fb2 = (frame % 2 == 1) ? &appState.fb1 : &appState.fb2;

		glBindFramebuffer(GL_FRAMEBUFFER, fb1->fbo);

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glBindVertexArray(appState.vao);

		glViewport(0, 0, fb1->width, fb1->height);

		glUseProgram(appState.shader);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, appState.ssbo);

		if (s_data_changed) {

			glBindBuffer(GL_SHADER_STORAGE_BUFFER, appState.ssbo);
			GLvoid* p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
			memcpy(p, &s_data, sizeof(shader_data));
			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

			s_data_changed = false;
		}

		camera.projection = glm::perspective(glm::radians(70.0f ), (float)width / (float)height, 0.1f, 100.0f);
		camera.view = glm::lookAt(camera.position, camera.position + camera.front, camera.up);

		// First Pass
		// TODO: Create the quad shader, make second and first pass, edit the fragment shader to do framebuffer.

		// Draw the main quad

		setUniformV2(appState.shader, "u_Resolution", glm::vec2(fb1->width, fb1->height));
		setUniformF(appState.shader, "u_Time", glfwGetTime());

		setUniformM4(appState.shader, "u_InverseView", glm::inverse(camera.view));
		setUniformM4(appState.shader, "u_InverseProjection", glm::inverse(camera.projection));

		setUniformInt(appState.shader, "useFresnel", useFresnel);
		
		setUniformInt(appState.shader, "u_SPP", spp);
		setUniformInt(appState.shader, "u_Bounces", bounces);

		setUniformInt(appState.shader, "u_FrameSinceLastReset", frameSinceLastReset);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, fb2->colorTexture);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, fb2->bloomTexture);

		setUniformInt(appState.shader, "u_LastColors", 0);
		setUniformInt(appState.shader, "u_LastBloom", 1);

		glClear(GL_COLOR_BUFFER_BIT);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		//glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		//  Second pass

		glClear(GL_COLOR_BUFFER_BIT);
		glViewport(0, 0, width, height);
		
		glBindVertexArray(appState.vao);
		glUseProgram(appState.quad_shader);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, fb1->colorTexture);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, fb1->bloomTexture);

		setUniformInt(appState.quad_shader, "u_Texture", 0);
		setUniformInt(appState.quad_shader, "u_BloomTexture", 1);
		setUniformInt(appState.quad_shader, "useSRGB", appState.useSRGB);
		setUniformInt(appState.quad_shader, "useACES", appState.useACES);

		setUniformInt(appState.quad_shader, "u_FrameSinceLastReset", frameSinceLastReset);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glBindVertexArray(0);

		// Update screen

		glfwSwapBuffers(appState.window);
		glfwPollEvents();

		frame++;
		frameSinceLastReset++;

		if (generateCells)
		{
			updateBuffer = true;
			generateCells = false;

			// Process any state
			int depth = s_data.mapd;
			if (!rule3D)
			{
				depth = 1;
			}

			// Calculate what to do next
			for (int i = 1; i < s_data.mapw - 1; i++)
			{
				for (int j = 1; j < s_data.maph - 1; j++)
				{
					if (!rule3D)
					{
						// 2D rules processing
						int neighbours = 0;
						for (int di = -1; di <= 1; di++)
						{
							for (int dj = -1; dj <= 1; dj++)
							{
								if (di == 0 && dj == 0)
								{
									continue;
								}

								if (s_data.data[(i + di) + s_data.mapw * (j + dj)] > 0)
								{
									neighbours++;
								}
							}
						}
						if (s_data.data[i + s_data.mapw * j] > 0)
						{
							// Any live cell...
							if (neighbours < ruleCellDiesFewerThan)
							{
								setWillDie(true, i, j, 0);
							}
							else if (neighbours < ruleCellLivesFewerThan)
							{
								// Do nothing
							}
							else if (neighbours > ruleCellDiesMoreThan)
							{
								setWillDie(true, i, j, 0);
							}
						}
						else
						{
							// Any dead cell...
							if (neighbours > ruleCellGrowsMoreThan && neighbours < ruleCellGrowsFewerThan)
							{
								setWillGrow(true, i, j, 0);
							}
						}
					}
					else
					{
						// 3D rules processing
						for (int k = 1; k < depth - 1; k++)
						{
							int neighbours = 0;
							for (int di = -1; di <= 1; di++)
							{
								for (int dj = -1; dj <= 1; dj++)
								{
									for (int dk = -1; dk <= 1; dk++)
									{
										if (di == 0 && dj == 0 && dk == 0)
										{
											continue;
										}

										if (s_data.data[(i + di) + s_data.mapw * (j + dj) + s_data.mapw * s_data.maph * (k + dk)] > 0)
										{
											neighbours++;
										}
									}
								}
							}
							if (s_data.data[i + s_data.mapw * j + s_data.mapw * s_data.maph * k] > 0)
							{
								// Any live cell...
								if (neighbours < ruleCellDiesFewerThan)
								{
									setWillDie(true, i, j, k);
								}
								else if (neighbours < ruleCellLivesFewerThan)
								{
									// Do nothing
								}
								else if (neighbours > ruleCellDiesMoreThan)
								{
									setWillDie(true, i, j, k);
								}
							}
							else
							{
								// Any dead cell...
								if (neighbours > ruleCellGrowsMoreThan && neighbours < ruleCellGrowsFewerThan)
								{
									setWillGrow(true, i, j, k);
								}
							}
						}
					}
				}
			}

			for (int i = 0; i < s_data.mapw; i++)
			{
				for (int j = 0; j < s_data.maph; j++)
				{
					for (int k = 0; k < depth; k++)
					{
						if (willGrow[i + s_data.mapw * j + s_data.mapw * s_data.maph * k])
						{
							s_data.data[i + s_data.mapw * j + s_data.mapw * s_data.maph * k] = 2;
							//							s_data.data[i + s_data.mapw * j + s_data.mapw * s_data.maph * k] = rand() % 9 + 1;
							willGrow[i + s_data.mapw * j + s_data.mapw * s_data.maph * k] = false;
						}
						else if (willDie[i + s_data.mapw * j + s_data.mapw * s_data.maph * k])
						{
							s_data.data[i + s_data.mapw * j + s_data.mapw * s_data.maph * k] = 0;
							willDie[i + s_data.mapw * j + s_data.mapw * s_data.maph * k] = false;
						}
						else if (s_data.data[i + s_data.mapw * j + s_data.mapw * s_data.maph * k] > 0)
						{
							s_data.data[i + s_data.mapw * j + s_data.mapw * s_data.maph * k] = s_data.data[i + s_data.mapw * j + s_data.mapw * s_data.maph * k] + 1;
							if (s_data.data[i + s_data.mapw * j + s_data.mapw * s_data.maph * k] >= 9)
							{
								//								s_data.data[i + s_data.mapw * j + s_data.mapw * s_data.maph * k] = rand() % 9 + 1;
								s_data.data[i + s_data.mapw * j + s_data.mapw * s_data.maph * k] = 1;
							}
						}
					}
				}
			}
		}

		if (updateBuffer)
		{
			updateBuffer = false;
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(shader_data), &s_data, GL_DYNAMIC_DRAW);
		}
	}

	// Cleanup
	glDeleteVertexArrays(1, &appState.vao);
	glDeleteBuffers(1, &appState.vbo);
	glDeleteProgram(appState.shader);
	glDeleteBuffers(1, &appState.ssbo);
	glDeleteFramebuffers(1, &fb1->fbo);
	glDeleteFramebuffers(1, &fb2->fbo);
	glDeleteTextures(1, &fb1->colorTexture);
	glDeleteTextures(1, &fb2->colorTexture);

	glfwTerminate();

	return 0;
}