#include "agent.h"
#include "glstuff.h"
#include "globals.h"
#include "compute_queue.h"

void mouseButtonFunction(GLFWwindow* window, int button, int action, int mods)
{
	//printf("mouseButtonFunction(button=%d, action=%d, mods=%d)\n", button, action, mods);
}

void mousePosFunction(int x, int y)
{
	//printf("mousePosFunction(x=%d, y=%d)\n", x, y);
}

void mouseWheelFunction(GLFWwindow* window, double xoffset, double yoffset)
{
	//printf("mouseWheelFunction(xoffset=%lf, yoffset=%lf)\n", xoffset, yoffset);
}

void keyFunction(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	//printf("keyFunction(key=%d, action=%d)\n", key, action);
	if (action == GLFW_PRESS)
	{
		switch (key)
		{
			case 'm':
			case 'M':
				GLOBALS.setInputToOverlay(true);
				break;
			case 'x':
			case 'X':
				ComputeQueue::get().runAll();
				break;
			default:
				break;
		}
	}
}

void charFunction(GLFWwindow* window, unsigned int codepoint)
{
	//printf("charFunction(codepoint=%d)\n", codepoint);
}

void sendInputToAgent()
{
	glfwSetInputMode(appWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	// Set GLFW event callbacks. I removed glfwSetWindowSizeCallback for conciseness
	glfwSetMouseButtonCallback(appWindow, mouseButtonFunction);
	glfwSetCursorPosCallback(appWindow, (GLFWcursorposfun)0);
	glfwSetScrollCallback(appWindow, mouseWheelFunction);
	glfwSetKeyCallback(appWindow, keyFunction);
	glfwSetCharCallback(appWindow, charFunction); 
}
