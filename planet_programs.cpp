#include "planet_programs.h"
#include "planet.h"
#include "utils.h"
#include "noise.h"
#include "globals.h"
#include "bruneton_water.h"
#include "simple_water.h"

const char* PLANET_UNIFORMS_SHADER_DECL = R"(
layout (std140) uniform PlanetUniforms
{
	mat4 m4_terrainMV;
	mat4 m4_terrainMVP;
	mat4 m4_skyMV;
	mat4 m4_skyMVP;
	
	vec3 v3_cameraFromCenter_VS; // The camera's current position
	vec3 v3_invWavelength;		// 1 / pow(wavelength, 4) for the red, green, and blue channels
	vec3 v3_lightCol;
	vec3 v3_lightDir_VS;		// The direction vector to the light source
	vec3 v3_lightPos_VS;
	vec3 v3_planetPos_VS;
	
	float f_cameraHeight;		// The camera's current height
	float f_cameraHeight2;		// f_cameraHeight^2
	float f_depthCoef;
	float g;
	float g2;
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
)";

AtmosphereConstants::AtmosphereConstants(
	int patchSize,
	const glm::vec3& waveLength,
	float innerRadius, float outerRadius,
	float kr, float km, float eSun,
	float g, float scaleDepth, int samples
) :
	m_patchSize(patchSize),
	m_waveLength(waveLength),
	m_innerRadius(innerRadius), m_outerRadius(outerRadius),
	m_kr(kr), m_km(km), m_eSun(eSun),
	m_g(g), m_scaleDepth(scaleDepth), m_samples(samples)
{
}

TerrainDrawProgram::TerrainDrawProgram(ShaderProgram* program) : m_program(program)
{
	m_program->setUniformBlockBinding(PLANET_UNIFORMS_NAME, PLANET_UNIFORMS_BINDING_POINT);
}

TerrainDrawProgram::~TerrainDrawProgram()
{
	delete m_program;
}
	
SkyDrawProgram::SkyDrawProgram(ShaderProgram* program) : m_program(program)
{
	m_program->setUniformBlockBinding(PLANET_UNIFORMS_NAME, PLANET_UNIFORMS_BINDING_POINT);
}

SkyDrawProgram::~SkyDrawProgram()
{
	delete m_program;
}

AtmosphereConstants* AtmosphereConstants::buildFromXMLNode(XMLNode& node)
{
	XMLChildFinder finder(node);

	return new AtmosphereConstants(
		finder.required("PatchSize", buildIntFromXMLNode),
		finder.required("WaveLength", buildFVec3FromXMLNode), 
		0.0, 
		0.0, 
		finder.required("Kr", buildFloatFromXMLNode), 
		finder.required("Km", buildFloatFromXMLNode),
		finder.required("ESun", buildFloatFromXMLNode),
		finder.required("G", buildFloatFromXMLNode),
		finder.required("ScaleDepth", buildFloatFromXMLNode),
		finder.required("Samples", buildIntFromXMLNode)
	);
}

Water* Water::buildFromXMLNode(XMLNode& node)
{
	const std::string type = getXMLTypeAttribute(node);

	if (type == "Bruneton")
		return nullptr;
		//return BrunetonWater::buildFromXMLNode(node);
	else if (type == "Simple")
		return SimpleWater::buildFromXMLNode(node);

	raiseXMLException(node, std::string("Invalid type: ") + type);
	return nullptr; // Keep compiler happy
}
