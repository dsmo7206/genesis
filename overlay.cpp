#include "overlay.h"
#include "globals.h"
#include "glstuff.h"
#include <cassert>

void initialiseOverlay()
{
	TwInit(TW_OPENGL_CORE, NULL);
	TwWindowSize(GLOBALS.getWindowWidth(), GLOBALS.getWindowHeight());
}

// Wrapper functions for tweak bar - tweak bar expects GLFW(<3) format callbacks
void myTwEventMouseButtonGLFW(GLFWwindow* window, int button, int action, int mods)
{
	TwEventMouseButtonGLFW(button, action);
}
void myTwEventMousePosGLFW(GLFWwindow* window, double xpos, double ypos)
{
	TwEventMousePosGLFW((int)xpos, (int)ypos);
}
void myTwEventMouseWheelGLFW(GLFWwindow* window, double xoffset, double yoffset)
{
	TwEventMouseWheelGLFW((int)yoffset); // Is this correct??
}
void myTwEventKeyGLFW(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	TwEventKeyGLFW(key, action);
	//TwEventKeyGLFW(key, GLFW_KEY_DOWN);
}
void myTwEventCharGLFW(GLFWwindow* window, unsigned int character)
{
	TwEventCharGLFW((int)character, GLFW_PRESS);//GLFW_KEY_DOWN); // Is this correct??
}

void sendInputToOverlay()
{
	// Set GLFW event callbacks. I removed glfwSetWindowSizeCallback for conciseness
	glfwSetMouseButtonCallback(appWindow, myTwEventMouseButtonGLFW); // - Directly redirect GLFW mouse button events to AntTweakBar
	glfwSetCursorPosCallback(appWindow, myTwEventMousePosGLFW); // - Directly redirect GLFW mouse position events to AntTweakBar
	glfwSetScrollCallback(appWindow, myTwEventMouseWheelGLFW); // - Directly redirect GLFW mouse wheel events to AntTweakBar
	glfwSetKeyCallback(appWindow, myTwEventKeyGLFW); // - Directly redirect GLFW key events to AntTweakBar
	glfwSetCharCallback(appWindow, myTwEventCharGLFW); // - Directly redirect GLFW char events to AntTweakBar
	glfwSetInputMode(appWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void drawOverlay() { TwDraw(); }
void killOverlay() { TwTerminate(); }
