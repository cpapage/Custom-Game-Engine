#pragma once
#include "GraphicsManager.h"
#include "GLFW/glfw3.h"

class InputManager
{
	private:
		GraphicsManager* graphics;
	public:
		InputManager(GraphicsManager* g = NULL);
		void Startup();
		void Shutdown();
		void Update();
		bool KeyIsPressed(int keyName);
};