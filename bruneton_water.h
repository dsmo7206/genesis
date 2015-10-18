#pragma once
/*
#include "glstuff.h"
#include "shader_program.h"
#include "overlay.h"
#include "planet_programs.h"

class InitProgram
{
	public:

	VertexArray m_vertexArray;
	VertexBuffer m_fullscreenQuadVertexBuffer;

	ShaderProgram* const m_program;

	GLint m_locId_spectrum_1_2_Sampler;
	GLint m_locId_spectrum_3_4_Sampler;
	GLint m_locId_fftSize;
	GLint m_locId_inverseGridSizes;
	GLint m_locId_t;

	InitProgram();
	void drawQuad();
};

class FFTProgram
{
	public:

	VertexArray m_vertexArray;
	VertexBuffer m_fullscreenQuadVertexBuffer;

	ShaderProgram* const m_program;

	GLint m_locId_butterflySampler;
	GLint m_locId_nLayers;
	GLint m_locId_pass;
	GLint m_locId_imgSampler;
	
	FFTProgram(bool isX);
	void drawQuad();
};

class VariancesProgram
{
	public:

	VertexArray m_vertexArray;
	VertexBuffer m_fullscreenQuadVertexBuffer;

	ShaderProgram* const m_program;

	GLint m_locId_nSlopeVariance;
	GLint m_locId_spectrum_1_2_Sampler;
	GLint m_locId_spectrum_3_4_Sampler;
	GLint m_locId_fftSize;
	GLint m_locId_gridSizes;
	GLint m_locId_slopeVarianceDelta;
	GLint m_locId_c;
	
	VariancesProgram();
	void drawQuad();
};

class RenderProgram
{
	public:

	ShaderProgram* const m_program;
	glm::vec4 m_seaColour;
	float m_stretch;
	float m_scale;

	GLint m_locId_M;
	GLint m_locId_MVP;
	GLint m_locId_worldCamera; // camera position in world space
	GLint m_locId_worldSunDir; // sun direction in world space
	GLint m_locId_gridSize;
	GLint m_locId_choppy;
	GLint m_locId_fftWavesSampler;
	GLint m_locId_GRID_SIZES;
	GLint m_locId_slopeVarianceSampler;
	GLint m_locId_seaColour; // sea bottom color
	GLint m_locId_f_depthCoef;
	GLint m_locId_stretch;
	GLint m_locId_scale;

	RenderProgram(
		ShaderProgram* program, 
		const glm::vec4& seaColour,
		float stretch,
		float scale
	);
};

class BrunetonWater : public Water
{
	public:

	FrameBuffer m_fftFbo1;
	FrameBuffer m_fftFbo2;
	FrameBuffer m_variancesFbo;
	FrameBuffer m_fbo;

	InitProgram m_initProgram;
	FFTProgram m_fftXProgram;
	FFTProgram m_fftYProgram;
	VariancesProgram m_variancesProgram;
	RenderProgram m_renderProgram;

	Texture m_irradianceTex;
	Texture m_inscatterTex;
	Texture m_transmittanceTex;
	Texture m_skyTex;
	Texture m_noiseTex;
	Texture m_spectrum12Tex;
	Texture m_spectrum34Tex;
	Texture m_slopeVarianceTex;
	Texture m_fftaTex;
	Texture m_fftbTex;
	Texture m_butterflyTex;

	void generateMesh();
	void computeSlopeVarianceTex();
	
	public:

	BrunetonWater(const glm::vec4& seaColour, float stretch, float scale);

	void addToOverlayBar(TwBar* bar);
	void update(const WorldClock& worldClock);

	static BrunetonWater* buildFromXMLNode(XMLNode& node);
};

*/
