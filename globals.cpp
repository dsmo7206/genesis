#include "globals.h"
#include "overlay.h"
#include "agent.h"
#include "scene.h"
#include "camera.h"

void TW_CALL antSetInputToOverlay(const void* value, void* clientData)
{
	assert(!(*(const bool*)value));
	GLOBALS.setInputToOverlay(false);
	TwEventMouseButtonGLFW(GLFW_MOUSE_BUTTON_LEFT, GLFW_RELEASE);
}
void TW_CALL antGetInputToOverlay(void* value, void* clientData)
{
	*(bool*)value = GLOBALS.getInputToOverlay();
}

void Globals::setWindowTitle(const std::string& windowTitle)
{
	m_windowTitle = windowTitle;
	glfwSetWindowTitle(appWindow, m_windowTitle.c_str());
}

void Globals::setWindowSize(GLFWwindow* window, int windowWidth, int windowHeight)
{
	m_windowWidth = windowWidth; 
	m_windowHeight = windowHeight;

	for (auto & sceneIt : SCENE_MAP)
		for (auto & cameraIt : sceneIt.second->m_cameras)
			cameraIt.second->refreshProjectionMatrix();

	glViewport(0, 0, m_windowWidth, m_windowHeight);
	TwWindowSize(m_windowWidth, m_windowHeight);
}

void Globals::setInputToOverlay(bool inputToOverlay)
{
	if (inputToOverlay)
		sendInputToOverlay();
	else
		sendInputToAgent();

	m_inputToOverlay = inputToOverlay;
	glfwSetCursorPos(appWindow, getWindowWidth() / 2, getWindowHeight() / 2);
}

void loadGlobalsFromXMLNode(XMLNode& node)
{
	XMLChildFinder finder(node);

	GLOBALS.m_windowTitle = finder.required("Name", buildStringFromXMLNode);
	GLOBALS.m_fullScreen = finder.required("FullScreen", buildBoolFromXMLNode);
	GLOBALS.m_windowWidth = finder.required("ResX", buildIntFromXMLNode);
	GLOBALS.m_windowHeight = finder.required("ResY", buildIntFromXMLNode);
	GLOBALS.m_clearColour = finder.required("ClearColour", buildFVec3FromXMLNode);
	GLOBALS.m_desiredFPS = finder.required("DesiredFPS", buildIntFromXMLNode);
	GLOBALS.m_maxPlanetPatchLevel = finder.required("MaxPlanetPatchLevel", buildIntFromXMLNode);
	GLOBALS.m_planetLevel1Distance = finder.required("PlanetLevel1Distance", buildFloatFromXMLNode);
}

void Globals::initialise()
{
	setWindowTitle(m_windowTitle);
	setWindowSize(appWindow, m_windowWidth, m_windowHeight);

	m_diagnosticsRefreshTime = 1.0;
	m_framesPerSecond = 1.0;
	m_millisPerFrame = 1000.0;
	m_frameNumber = 0;
	setInputToOverlay(false);
	m_shuttingDown = false;

	m_overlay_bar = TwNewBar("Settings");
	TwSetParam(m_overlay_bar, NULL, "refresh", TW_PARAM_CSTRING, 1, "0.1");

	TwAddVarCB(m_overlay_bar, "Input To Overlay", TW_TYPE_BOOLCPP, antSetInputToOverlay, antGetInputToOverlay, nullptr, nullptr);
	TwAddVarRW(m_overlay_bar, "Diag Refresh Time", TW_TYPE_DOUBLE, &m_diagnosticsRefreshTime, "min=0.01 step=0.01 group=Globals ");
	TwAddVarRO(m_overlay_bar, "Frames Per Second", TW_TYPE_DOUBLE, &m_framesPerSecond, " group=Globals ");
	TwAddVarRO(m_overlay_bar, "Millis Per Frame", TW_TYPE_DOUBLE, &m_millisPerFrame, " group=Globals ");
	TwAddVarRW(m_overlay_bar, "Desired FPS", TW_TYPE_INT32, &m_desiredFPS, " group=Globals min=1 step=1");
	TwAddVarRO(m_overlay_bar, "Window Width", TW_TYPE_INT32, &m_windowWidth, " group=Globals ");
	TwAddVarRO(m_overlay_bar, "Window Height", TW_TYPE_INT32, &m_windowHeight, " group=Globals ");
	TwAddVarRW(m_overlay_bar, "Max Patch Level", TW_TYPE_INT32, &m_maxPlanetPatchLevel, "min=0 max=27 group=Planet ");
	TwAddVarRW(m_overlay_bar, "Level 1 Distance", TW_TYPE_FLOAT, &m_planetLevel1Distance, "min=0.1 step=0.1 group=Planet ");
}

Globals GLOBALS;
