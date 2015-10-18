#pragma once

#include "glstuff.h"

class Camera;

void initialiseSkyBox();

class SkyBox
{
	public:

	SkyBox();
	void draw(const Camera* const camera) const;
};
