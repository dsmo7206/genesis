#pragma once

#define GLFW_CDECL
#include <AntTweakBar.h>

void initialiseOverlay(); // Creates UI - only call once
void drawOverlay(); // Call every frame
void sendInputToOverlay(); // Sends mouse/keyboard events to UI
void killOverlay(); // Call at end of program
/*
class OverlayGroup;

class OverlayEntity
{
	OverlayEntity* m_parent;
	std::string m_ident;

protected:

	void setParent(OverlayEntity* parent)
	{
		assert(parent && !m_parent);
		m_parent = parent;
		m_ident = m_parent->m_name + m_name;
	}

public:

	const std::string m_name;

	OverlayEntity(const std::string& name) : m_parent(nullptr), m_name(name) {}

	virtual TwBar* getTwBar() = 0;

	void addGroup(OverlayGroup* group);
};

class OverlayGroup : public OverlayEntity
{

};

class OverlayBar : public OverlayEntity
{
	TwBar* m_twBar;

public:

	TwBar* getTwBar() override { return m_twBar; }
};
*/