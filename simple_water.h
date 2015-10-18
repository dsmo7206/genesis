#pragma once

#include "planet_programs.h"
#include "glstuff.h"

class SimpleWater : public Water
{
	const glm::vec3 m_seaColour;

	public:

	SimpleWater(const glm::vec3& seaColour);

	void addToOverlayBar(TwBar* bar) override;
	void update(const WorldClock& worldClock) override;

	static SimpleWater* buildFromXMLNode(XMLNode& node);
};
