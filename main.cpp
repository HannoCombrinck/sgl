#include <iostream>
#include <future>
#include "myglfw.h"
#include "Timer.h"

using namespace std;

// GPU jobs
/*
	Create VBO, IBO, VAO, shader, texture, framebufferobject etc..
*/

// Buffer containing draw call information 
/* Render something: (View: has a frustum and camera i.e view and projection matrices)
	Bind render target
	Bind shaders
	Upload uniforms
	Bind textures
	Submit draw call

	sorting key that represents all of the above 
*/

void update(double dt)
{

}

void render()
{

}

int doSomeWork(int iCount)
{
	int iTotal = 0;
	for (int i = 0; i < iCount; ++i)
	{
		iTotal += i % 10;
	}
	return iTotal;
}

int main(int argc, char** argv)
{
	GLWindow window("OpenGL Window", 800, 600, false);
	window.registerKeyPress(GLFW_KEY_A, []() { cout << doSomeWork(10) << endl; });


	auto dCurrentTime = glfwGetTime();
	double dPreviousTime = 0.0;
	double dt = 0.0;
	
	while (window.isRunning())
	{
		// Update thread
		dPreviousTime = dCurrentTime;
		dCurrentTime = glfwGetTime();
		dt = dCurrentTime - dPreviousTime;
		window.processEvents();
		update(dt);

		// Render thread
		render();
		window.swapBuffers();
	}

	return 0; 
}

