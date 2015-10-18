#pragma once

#include "glstuff.h"
#include "shader_program.h"

class FullscreenQuad
{
public:

	static const VertexArray& getVertexArray();
	static const ShaderStage& getVertexShaderStage();

	static void draw();
};
