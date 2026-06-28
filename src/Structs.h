#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

struct keys {
	bool w = false;
	bool a = false;
	bool s = false;
	bool d = false;
	bool space = false;
	bool left_control = false;
};

struct Camera {
	glm::vec3 position;
	glm::vec3 direction;
	float rotationX;
	float rotationY;

	glm::vec3 worldUp;

	glm::vec3 front;
	glm::vec3 right;
	glm::vec3 up;

	float lastX;
	float lastY;

	glm::mat4 projection;
	glm::mat4 view;

	keys keys;
};

struct shader_data {
	int mapw;
	int maph;
	int mapd;

	glm::vec3 palette[10];
	
	int data[dimension* dimension * dimension];

	void setCell(int stage, int w, int h, int d)
	{
		data[w + (mapw * h) + (mapw * maph * d)] = stage;
	}

	int getCell(int w, int h, int d)
	{
		return data[w + (mapw * h) + (mapw * maph * d)];
	}
};

struct Framebuffer {
	GLuint fbo;
	GLuint colorTexture;
	GLuint bloomTexture;
	int width;
	int height;
};

struct AppState {
	GLFWwindow* window;
	GLuint vao, vbo;

	GLuint shader;
	GLuint quad_shader;

	shader_data s_data;
	GLuint ssbo;
	Framebuffer fb1;
	Framebuffer fb2;

	bool useSRGB = true;
	bool useACES = true;
};