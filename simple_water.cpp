#include "simple_water.h"
#include "utils.h"
#include "overlay.h"

SimpleWater::SimpleWater(const glm::vec3& seaColour) :
	Water(new ShaderProgram({ ShaderStages::Vertex::simpleWater, ShaderStages::Fragment::simpleWater })), 
	m_seaColour(seaColour)
{
	m_program->setUniformBlockBinding(PLANET_UNIFORMS_NAME, PLANET_UNIFORMS_BINDING_POINT);
}

void SimpleWater::addToOverlayBar(TwBar* bar)
{
}

void SimpleWater::update(const WorldClock& worldClock)
{
}

SimpleWater* SimpleWater::buildFromXMLNode(XMLNode& node)
{
	XMLChildFinder finder(node);

	return new SimpleWater(finder.required("Colour", buildFVec3FromXMLNode));
}
