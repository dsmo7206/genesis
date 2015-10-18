#pragma once

#include <string>
#include <vector>

#include "glstuff.h"
#include "xml.h"
#include "overlay.h"

class Globals
{
	std::string m_windowTitle;
	int m_windowWidth;
	int m_windowHeight;
	bool m_inputToOverlay;

	public:

	bool m_fullScreen;
	bool m_shuttingDown;
	double m_diagnosticsRefreshTime;
	double m_framesPerSecond;
	double m_millisPerFrame;
	int m_desiredFPS;
	int m_frameNumber;
	glm::vec3 m_clearColour;
	int m_maxPlanetPatchLevel;
	float m_planetLevel1Distance;

	TwBar* m_overlay_bar;

	void initialise();

	inline const std::string& getWindowTitle() const { return m_windowTitle; }
	void setWindowTitle(const std::string& windowTitle);

	inline int getWindowWidth() const { return m_windowWidth; }
	inline int getWindowHeight() const { return m_windowHeight; }
	void setWindowSize(GLFWwindow* window, int windowWidth, int windowHeight);

	inline bool getInputToOverlay() const { return m_inputToOverlay; }
	void setInputToOverlay(bool inputToOverlay);

	friend void loadGlobalsFromXMLNode(XMLNode& node);
};

extern Globals GLOBALS;

inline void setGlobalsWindowSize(GLFWwindow* window, int windowWidth, int windowHeight)
{
	GLOBALS.setWindowSize(window, windowWidth, windowHeight);
}

void loadGlobalsFromXMLNode(XMLNode& node);
