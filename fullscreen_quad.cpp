#include "fullscreen_quad.h"
#include "utils.h"

ShaderStage* makeVertexShaderStage()
{
	const std::string source = R"(

const vec2 data[4] = vec2[]
(
	vec2(-1.0, 1.0),
	vec2(-1.0, -1.0),
	vec2(1.0, 1.0),
	vec2(1.0, -1.0)
);

void main()
{
	gl_Position = vec4(data[gl_VertexID], 0.0, 1.0);
}

)";

	return new ShaderStage(GL_VERTEX_SHADER, source, 430);
}

const VertexArray& FullscreenQuad::getVertexArray()
{
	static VertexArray vertexArray;
	return vertexArray;
}

const ShaderStage& FullscreenQuad::getVertexShaderStage()
{
	static ShaderStage* stage = makeVertexShaderStage();
	return *stage;
}

void FullscreenQuad::draw()
{
	glBindVertexArray(getVertexArray().m_id);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
