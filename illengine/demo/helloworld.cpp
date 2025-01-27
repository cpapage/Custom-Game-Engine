#include <iostream>
#include "Engine.h"
#include "GraphicsManager.h"
#include "GLFW/glfw3.h"

void printSomething()
{
	printf("Alpha and Violet!\n");
}

int main(int argc, const char* argv[])
{
	Engine *e = new Engine(1920, 1080, "Game", false);
	e->Startup();
	e->RunGameLoop(printSomething);
	e->Shutdown();
	delete e;
	return 0;
}