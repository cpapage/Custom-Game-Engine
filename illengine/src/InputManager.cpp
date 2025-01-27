#include "InputManager.h"
#include <stdlib.h>
#include "GLFW/glfw3.h"

InputManager::InputManager(GraphicsManager* g)
{
	graphics = g;
};

void InputManager::Startup()
{

}

void InputManager::Shutdown()
{

}

void InputManager::Update()
{
	glfwPollEvents();
}

bool InputManager::KeyIsPressed(int keyName)
{
	bool keyPressed = (glfwGetKey(graphics->window, keyName) == GLFW_PRESS);
	return keyPressed;
}