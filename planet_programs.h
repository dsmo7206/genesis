#pragma once

#include <cstdint>

#include "glstuff.h"
#include "shader_program.h"
#include "xml.h"
#include "overlay.h"

class WorldClock;

struct PlanetUniforms
{
	glm::mat4 m4_terrainMV;
	glm::mat4 m4_terrainMVP;
	glm::mat4 m4_skyMV;
	glm::mat4 m4_skyMVP;

	glm::vec3 v3_cameraFromCenter_VS;	uint32_t _pad0;
	glm::vec3 v3_invWavelength;			uint32_t _pad1;
	glm::vec3 v3_lightCol;				uint32_t _pad2;
	glm::vec3 v3_lightDir_VS;			uint32_t _pad3;
	glm::vec3 v3_lightPos_VS;			uint32_t _pad4;
	glm::vec3 v3_planetPos_VS;

	float f_cameraHeight;		// The camera's current height
	float f_cameraHeight2;		// f_cameraHeight^2
	float f_depthCoef;
	float f_g;
	float f_g2;
	float f_innerRadius;		// The inner (planetary) radius
	float f_innerRadius2;		// f_innerRadius^2
	float f_krESun;				// Kr * ESun
	float f_kmESun;				// Km * ESun
	float f_kr4PI;				// Kr * 4 * PI
	float f_km4PI;				// Km * 4 * PI
	float f_outerRadius;		// The outer (atmosphere) radius
	float f_outerRadius2;		// f_outerRadius^2
	float f_scale;				// 1 / (f_outerRadius - f_innerRadius)
	float f_scaleDepth;			// The scale depth (i.e. the altitude at which the atmosphere's average density is found)
	float f_scaleOverScaleDepth;// f_scale / f_scaleDepth

	int i_samples;
};

extern const char* PLANET_UNIFORMS_SHADER_DECL;
static const char* PLANET_UNIFORMS_NAME = "PlanetUniforms";
static const GLuint PLANET_UNIFORMS_BINDING_POINT = 1;

struct TerrainDrawProgram
{
	ShaderProgram* const m_program;

	TerrainDrawProgram(ShaderProgram* program);
	~TerrainDrawProgram();
};

struct SkyDrawProgram
{
	ShaderProgram* const m_program;

	SkyDrawProgram(ShaderProgram* program);
	~SkyDrawProgram();
};

struct AtmosphereConstants
{
	int m_patchSize;
	glm::vec3 m_waveLength;
	float m_innerRadius, m_outerRadius;
	float m_kr, m_km, m_eSun, m_g, m_scaleDepth; 
	int m_samples;

	AtmosphereConstants(
		int patchSize,
		const glm::vec3& waveLength,
		float innerRadius, float outerRadius,
		float kr, float km, float eSun,
		float g, float scaleDepth, int samples
	);

	static AtmosphereConstants* buildFromXMLNode(XMLNode& node);
};

class Water
{
	public:

	ShaderProgram* const m_program;

	Water(ShaderProgram* program) : m_program(program) {}

	virtual void addToOverlayBar(TwBar* bar) = 0;
	virtual void update(const WorldClock& worldClock) = 0;

	static Water* buildFromXMLNode(XMLNode& node);
};
