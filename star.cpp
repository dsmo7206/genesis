#include "star.h"
#include "camera.h"

void Star::updateGeneral(const WorldClock& worldClock)
{
}

void Star::updateForCamera(const Camera* camera)
{
	m_v3f_pos_VS = matrixPosition(camera->getZeroViewMatrix() * glm::mat4(camera->getInvPosMatrix() * m_position->getMatrix()));
}

FloatPair Star::getMinMaxDrawDist() const
{
	return std::make_pair(glm::length(m_v3f_pos_VS), 0.0f);
}

Star* Star::buildFromXMLNode(XMLNode& node)
{
	XMLChildFinder finder(node);

	return new Star(
		finder.required("Name", buildStringFromXMLNode),
		finder.required("Position", Position::buildFromXMLNode),
		finder.required("Colour", buildFVec3FromXMLNode)
	);
}
