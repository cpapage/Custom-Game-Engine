#pragma once
#include "GLFW/glfw3.h"
#include "EntityComponentSystem.h"
#include "Types.h"
#include <vector>

class GraphicsManager
{
public:
	int window_width;
	int window_height;
	const char* window_name;
	bool window_fullscreen;
	EntityComponentSystem *ECS;
	GLFWwindow* window;

	GraphicsManager(int width = int(0), int height = int(0), const char* name = NULL, bool fullscreen = false, EntityComponentSystem *e = NULL);
	void Startup();
	void Shutdown();
	bool ShouldQuit();
	void SetShouldQuit(bool quit);
	bool LoadOneImage( const string& name, const string& path );
	void Draw();
};