#pragma once

#include "glstuff.h"
#include "shader_program.h"
#include "xml.h"

class TerrainGenerator
{
	public:

	VertexArray m_vertexArray;
	ShaderProgram* m_program;
	
	GLint m_locId_patchDetails;

	TerrainGenerator(ShaderStage* stageCompute, int seed);
	virtual ~TerrainGenerator();
	virtual void addToOverlay(void* bar) = 0;

	static TerrainGenerator* buildFromXMLNode(XMLNode& node);
};

class RidgedMFGenerator : public TerrainGenerator
{
	public:

	GLuint m_permTextureId;
	GLuint m_simplexTextureId;
	GLuint m_gradTextureId;

	RidgedMFGenerator(
		int seed, float lacunarity, float gain, float offset,
		int octaves, float scale, float bias
	);

	void addToOverlay(void* bar);

	static RidgedMFGenerator* buildFromXMLNode(XMLNode& node);
};

class LibnoiseGenerator : public TerrainGenerator
{
	public:

	// Buffers
	GLuint m_permTextureId;
	GLuint m_simplexTextureId;
	GLuint m_gradTextureId;
	
	LibnoiseGenerator(int seed);
	void addToOverlay(void* bar);

	static LibnoiseGenerator* buildFromXMLNode(XMLNode& node);
};
