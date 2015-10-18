#include <sstream>

#include "position.h"
#include "shapes.h"
#include "scene.h"
#include "world_clock.h"

const char* POSITION_UNIT_NAMES[4] = {"m", "km", "AU", "LY"};
const char* VELOCITY_UNIT_NAMES[3] = {"km/h", "m/s", "c"};

PositionUnit buildPositionUnitFromXMLNode(XMLNode& node)
{
	const std::string str = buildStringFromXMLNode(node);
	for (int i = 0; i < sizeof(POSITION_UNIT_NAMES)/sizeof(const char*); ++i)
		if (str == POSITION_UNIT_NAMES[i])
			return (PositionUnit)i;
	raiseXMLException(node, "Invalid PositionUnit");
	return (PositionUnit)0; // Keep compiler happy
}

VelocityUnit buildVelocityUnitFromXMLNode(XMLNode& node)
{
	const std::string str = buildStringFromXMLNode(node);
	for (int i = 0; i < sizeof(VELOCITY_UNIT_NAMES)/sizeof(const char*); ++i)
		if (str == VELOCITY_UNIT_NAMES[i])
			return (VelocityUnit)i;
	raiseXMLException(node, "Invalid VelocityUnit");
	return (VelocityUnit)0; // Keep compiler happy
}

/////////////////////////////////////////////////

SceneResolveable::SceneResolveable(bool resolved) : m_resolved(resolved) 
{
	SCENE_RESOLVEABLES.push_back(this);
}

/////////////////////////////////////////////////

void Position::update(const WorldClock& worldClock)
{
	if (worldClock.getT() != m_updatedTime)
	{
		_update(worldClock);
		m_updatedTime = worldClock.getT();
	}
}

Position* Position::buildFromXMLNode(XMLNode& node)
{
	const std::string type = getXMLTypeAttribute(node);

	if (type == "Absolute")
		return AbsolutePosition::buildFromXMLNode(node);
	else if (type == "Relative")
		return RelativePosition::buildFromXMLNode(node);
	else if (type == "Rotation")
		return Rotation::buildFromXMLNode(node);
	else if (type == "CircularOrbit")
		return CircularOrbit::buildFromXMLNode(node);
	else if (type == "Origin")
		return new AbsolutePosition(glm::dvec3(0.0));

	raiseXMLException(node, std::string("Invalid type: ") + type);
	return nullptr; // Keep compiler happy
}

/////////////////////////////////////////////////

AbsolutePosition::AbsolutePosition(const glm::dvec3& value) : Position(true), m_value(value)
{
	m_matrix = glm::translate(glm::dmat4(), m_value);
}

void AbsolutePosition::_update(const WorldClock& worldClock)
{
}

AbsolutePosition* AbsolutePosition::buildFromXMLNode(XMLNode& node)
{
	XMLChildFinder finder(node);

	return new AbsolutePosition(finder.required("Value", buildDVec3FromXMLNode));
}

/////////////////////////////////////////////////

RelativePosition::RelativePosition(const glm::dvec3& value, const std::string& parentName) :
	Position(false), SceneResolveable(false), m_value(value), m_parentName(parentName), m_parent(nullptr)
{
}

RelativePosition::RelativePosition(const glm::dvec3& value, Position* parent) :
	Position(false), SceneResolveable(true), m_value(value), m_parent(parent)
{
}

void RelativePosition::_resolve(Scene* scene)
{
	Shape* const parent = scene->findShapeByName(m_parentName);
	if (parent)
		m_parent = parent->m_position;
	else
		throw std::exception((std::string("Can't find Shape \"") + m_parentName + "\"").c_str());
}

void RelativePosition::_update(const WorldClock& worldClock)
{
	m_parent->update(worldClock);
	m_matrix = glm::translate(m_parent->getMatrix(), m_value);
}

void RelativePosition::changeParent(Position* parent)
{
	if (parent == m_parent)
		return;

	
}

RelativePosition* RelativePosition::buildFromXMLNode(XMLNode& node)
{
	XMLChildFinder finder(node);

	if (nodeHasChild(node, "Parent"))
		return new RelativePosition(
			finder.required("Value", buildDVec3FromXMLNode), 
			finder.required("Parent", Position::buildFromXMLNode)
		);
	else
		return new RelativePosition(
			finder.required("Value", buildDVec3FromXMLNode), 
			finder.required("ParentName", buildStringFromXMLNode)
		);
}

/////////////////////////////////////////////////

Rotation::Rotation(double angularVelocity, const glm::dvec3& axis, const std::string& parentName) :
	Position(false), SceneResolveable(false), m_angularVelocity(angularVelocity), m_axis(axis), m_parentName(parentName), m_parent(nullptr)
{
}

Rotation::Rotation(double angularVelocity, const glm::dvec3& axis, Position* parent) :
	Position(false), SceneResolveable(true), m_angularVelocity(angularVelocity), m_axis(axis), m_parent(parent)
{
}

void Rotation::_resolve(Scene* scene)
{
	Shape* const parent = scene->findShapeByName(m_parentName);
	if (parent)
		m_parent = parent->m_position;
	else
		throw std::exception((std::string("Can't find Shape \"") + m_parentName + "\"").c_str());
}

void Rotation::_update(const WorldClock& worldClock)
{
	m_parent->update(worldClock);
	m_matrix = glm::rotate(m_parent->getMatrix(), m_angularVelocity * worldClock.getT(), m_axis);
}

Rotation* Rotation::buildFromXMLNode(XMLNode& node)
{
	XMLChildFinder finder(node);

	if (nodeHasChild(node, "Parent"))
	{
		return new Rotation(
			finder.required("AngularVelocity", buildDoubleFromXMLNode), 
			finder.required("Axis", buildDVec3FromXMLNode),
			finder.required("Parent", Position::buildFromXMLNode)
		);
	}
	else
		return new Rotation(
			finder.required("AngularVelocity", buildDoubleFromXMLNode),
			finder.required("Axis", buildDVec3FromXMLNode),
			finder.required("ParentName", buildStringFromXMLNode)
		);
}

/////////////////////////////////////////////////

CircularOrbit::CircularOrbit(double radius, double angularVelocity, const std::string& parentName) :
	Position(false), SceneResolveable(false), m_radius(radius), m_angularVelocity(angularVelocity), m_parentName(parentName), m_parent(0)
{
}

CircularOrbit::CircularOrbit(double radius, double angularVelocity, Position* parent) :
	Position(false), SceneResolveable(true), m_radius(radius), m_angularVelocity(angularVelocity), m_parent(parent)
{
}

void CircularOrbit::_resolve(Scene* scene)
{
	Shape* const parent = scene->findShapeByName(m_parentName);
	if (parent)
		m_parent = parent->m_position;
	else
		throw std::exception((std::string("Can't find Shape \"") + m_parentName + "\"").c_str());
}

void CircularOrbit::_update(const WorldClock& worldClock)
{
	m_parent->update(worldClock);
	m_matrix = glm::translate(
		m_parent->getMatrix(), 
		glm::dvec3(
			cos(m_angularVelocity * worldClock.getT()), 
			0.0, 
			sin(m_angularVelocity * worldClock.getT())
		) * m_radius
	);
}

CircularOrbit* CircularOrbit::buildFromXMLNode(XMLNode& node)
{
	XMLChildFinder finder(node);

	if (nodeHasChild(node, "Parent"))
	{
		return new CircularOrbit(
			finder.required("Radius", buildFloatFromXMLNode), 
			finder.required("AngularVelocity", buildFloatFromXMLNode), 
			finder.required("Parent", Position::buildFromXMLNode)
		);
	}
	else
		return new CircularOrbit(
			finder.required("Radius", buildFloatFromXMLNode), 
			finder.required("AngularVelocity", buildFloatFromXMLNode), 
			finder.required("ParentName", buildStringFromXMLNode)
		);
}
