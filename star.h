#pragma once

#include "shapes.h"
#include "lightsource.h"
#include "glstuff.h"
#include "xml.h"

class Star : public Shape, public LightSource
{
	public:

	const glm::vec3 m_colour;

	Star(const std::string& name, Position* position, const glm::vec3& colour) :
		Shape(name, position), m_colour(colour)
	{}

	// Shape functions
	void updateGeneral(const WorldClock& worldClock) override;
	void updateForCamera(const Camera* camera) override;

	FloatPair getMinMaxDrawDist() const override;
	
	void draw(const Scene* scene, const Camera* camera) override {}

	// LightSource functions
	const Position* getLightSourcePosition() override { return m_position; }
	const glm::vec3& getLightSourceColour() override { return m_colour; }

	static Star* buildFromXMLNode(XMLNode& node);
};
