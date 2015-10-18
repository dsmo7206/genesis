#pragma once

#include "glstuff.h"
#include "position.h"

class LightSource
{
	public:

	glm::vec3 m_v3f_pos_VS;

	virtual const Position* getLightSourcePosition() = 0;
	virtual const glm::vec3& getLightSourceColour() = 0;
};
