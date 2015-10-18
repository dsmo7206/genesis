#pragma once

#include "glstuff.h"
#include "xml.h"
#include "utils.h"
#include "scene_resolveable.h"

class Scene;
class WorldClock;

enum class PositionUnit { METERS = 0, KILOMETERS = 1, AU = 2, LIGHT_YEARS = 3 };
enum class VelocityUnit { METERS_PER_SECOND = 0, KILOMETERS_PER_HOUR = 1, FRACTION_LIGHT_SPEED = 2 };
PositionUnit buildPositionUnitFromXMLNode(XMLNode& node);
VelocityUnit buildVelocityUnitFromXMLNode(XMLNode& node);

const glm::vec4 POS_MULTIPLY_F(0.0, 0.0, 0.0, 1.0);
const glm::dvec4 POS_MULTIPLY_D(0.0, 0.0, 0.0, 1.0);

inline glm::vec3 matrixPosition(const glm::mat4& m) { return glm::vec3(m * POS_MULTIPLY_F); }
inline glm::dvec3 matrixPosition(const glm::dmat4& m) { return glm::dvec3(m * POS_MULTIPLY_D); }

class Position
{
	double m_updatedTime;
	virtual void _update(const WorldClock& worldClock) = 0;

	protected:

	glm::dmat4 m_matrix;

	public:

	Position(bool updated) : m_updatedTime(-1.0) {}
	void update(const WorldClock& worldClock);
	const glm::dmat4& getMatrix() const { return m_matrix; }

	static Position* buildFromXMLNode(XMLNode& node);
};

class AbsolutePosition : public Position
{
	void _update(const WorldClock& worldClock);

	public:

	const glm::dvec3 m_value;

	AbsolutePosition(const glm::dvec3& value);

	static AbsolutePosition* buildFromXMLNode(XMLNode& node);
};

class RelativePosition : public Position, public SceneResolveable
{
	void _resolve(Scene* scene);
	void _update(const WorldClock& worldClock);

	public:

	glm::dvec3 m_value;
	std::string m_parentName; Position* m_parent;

	RelativePosition(const glm::dvec3& value, const std::string& parentName);
	RelativePosition(const glm::dvec3& value, Position* parent);

	void changeParent(Position* parent);

	static RelativePosition* buildFromXMLNode(XMLNode& node);
};

class Rotation : public Position, public SceneResolveable
{
	void _resolve(Scene* scene);
	void _update(const WorldClock& worldClock);

	public:

	const double m_angularVelocity;
	const glm::dvec3 m_axis;
	std::string m_parentName; Position* m_parent;

	Rotation(double angularVelocity, const glm::dvec3& axis, const std::string& parentName);
	Rotation(double angularVelocity, const glm::dvec3& axis, Position* parent);

	static Rotation* buildFromXMLNode(XMLNode& node);
};

class CircularOrbit : public Position, public SceneResolveable
{
	void _resolve(Scene* scene);
	void _update(const WorldClock& worldClock);

	public:
	
	const double m_radius;
	const double m_angularVelocity;
	std::string m_parentName; Position* m_parent;

	CircularOrbit(double radius, double angularVelocity, const std::string& parentName);
	CircularOrbit(double radius, double angularVelocity, Position* parent);

	static CircularOrbit* buildFromXMLNode(XMLNode& node);
};
