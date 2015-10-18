#include "skybox.h"
#include "shader_program.h"
#include "camera.h"

static const float SKYBOX_SIZE = 100.0f; // Doesn't matter as long as it's positive!
static const float SQRT_3_3 = 0.5773502691896258f;

static VertexArray* skyBoxVertexArray;
static VertexBuffer* skyBoxVertexBuffer;
static VertexBuffer* skyBoxIndexBuffer;
static ShaderProgram* skyBoxProgram = nullptr;
static GLuint skyBoxProgram_locId_m4_MVP;

void initialiseSkyBox()
{
	// Create program
	skyBoxProgram = new ShaderProgram({ShaderStages::Vertex::skyBox, ShaderStages::Fragment::skyBox});
	skyBoxProgram_locId_m4_MVP = skyBoxProgram->getUniformLocationId("m4_MVP");

	skyBoxVertexArray = new VertexArray();
	glBindVertexArray(skyBoxVertexArray->m_id);

	// Create vertex buffer
	skyBoxVertexBuffer = new VertexBuffer();
	glBindBuffer(GL_ARRAY_BUFFER, skyBoxVertexBuffer->m_id);
	float vertices[24] = {	// the idea is that all vectors have a length of 1 so that position can also be used as cubemap texture coords
		-SQRT_3_3, -SQRT_3_3, -SQRT_3_3,
		 SQRT_3_3, -SQRT_3_3, -SQRT_3_3,
		-SQRT_3_3,  SQRT_3_3, -SQRT_3_3,
		 SQRT_3_3,  SQRT_3_3, -SQRT_3_3,
		-SQRT_3_3, -SQRT_3_3,  SQRT_3_3,
		 SQRT_3_3, -SQRT_3_3,  SQRT_3_3,
		-SQRT_3_3,  SQRT_3_3,  SQRT_3_3,
		 SQRT_3_3,  SQRT_3_3,  SQRT_3_3
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

	// Create index buffer
	skyBoxIndexBuffer = new VertexBuffer();
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skyBoxIndexBuffer->m_id);
	GLubyte indices[36] = {
		6, 4, 0, 0, 2, 6, // neg X
		3, 1, 5, 5, 7, 3, // pos X
		0, 4, 5, 5, 1, 0, // neg Y
		6, 2, 3, 3, 7, 6, // pos Y
		2, 0, 1, 1, 3, 2, // neg Z
		7, 5, 4, 4, 6, 7  // pos Z
	};
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glBindVertexArray(0);
}

SkyBox::SkyBox()
{
}

void SkyBox::draw(const Camera* const camera) const
{
	glDisable(GL_DEPTH_TEST); // SkyBox should be drawn behind everything else
	glBindVertexArray(skyBoxVertexArray->m_id);
	glUseProgram(skyBoxProgram->m_id);
	setUniformMatrixm4f(skyBoxProgram_locId_m4_MVP, camera->getZeroViewProjectionMatrix());
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_BYTE, 0);
	glEnable(GL_DEPTH_TEST);
}
