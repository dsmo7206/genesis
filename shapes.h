#pragma once

#include <utility>
#include "glstuff.h"
#include "xml.h"
#include "position.h"

class Scene;
class Camera;
class WorldClock;

typedef std::pair<float, float> FloatPair;

class Shape
{
	public:

	const std::string m_name;
	Position* const m_position;
	
	Shape(const std::string& name, Position* position) :
		m_name(name), m_position(position)
	{}

	// This function should update members, excluding m_position
	// (this is done by the Scene), that are not camera-specific.
	virtual void updateGeneral(const WorldClock& worldClock) = 0;

	// This function should cache any View Space specific members.
	virtual void updateForCamera(const Camera* camera) = 0;
	
	// Used for sorting by closeness when drawing Systems - because
	// closer transparent atmospheres need to be drawn after distant planets
	virtual FloatPair getMinMaxDrawDist() const = 0;
	
	virtual void draw(const Scene* scene, const Camera* camera) = 0;

	static Shape* buildFromXMLNode(XMLNode& node);
};
