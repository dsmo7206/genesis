#include "bruneton_atmosphere.h"
#include "fullscreen_quad.h"

namespace Bruneton {

ShaderStage* LAYER_GEOMETRY_STAGE;

const GLuint UNIFORM_BINDING_INDEX_RES = 0;
const GLuint UNIFORM_BINDING_INDEX_PLANET = 1;
const GLuint UNIFORM_BINDING_INDEX_ATMOSPHERE = 2;
const GLuint UNIFORM_BINDING_INDEX_DYNAMICDRAW = 3;
const GLuint UNIFORM_BINDING_INDEX_LAYER = 4;

const int TEXTURE_UNIT_REFLECTANCE = 0;
const int TEXTURE_UNIT_TRANSMITTANCE = 1;
const int TEXTURE_UNIT_IRRADIANCE = 2;
const int TEXTURE_UNIT_INSCATTER = 3;
const int TEXTURE_UNIT_DELTA_E = 4;
const int TEXTURE_UNIT_DELTA_SR = 5;
const int TEXTURE_UNIT_DELTA_SM = 6;
const int TEXTURE_UNIT_DELTA_J = 7;

class TransmittanceProgram : public ShaderProgram
{
public:

	TransmittanceProgram() :
		ShaderProgram({
		&FullscreenQuad::getVertexShaderStage(),
		std::make_unique<ShaderStage>(
		GL_FRAGMENT_SHADER,
		stringFromFile("D:/Code/PrecomputedAtmosphericScattering/transmittance.fs.glsl"),
		430
		).get()
	})
	{}
};

class Irradiance1Program : public ShaderProgram
{
	GLuint m_locId_transmittanceSampler;

public:

	Irradiance1Program() :
		ShaderProgram({
		&FullscreenQuad::getVertexShaderStage(),
		std::make_unique<ShaderStage>(
		GL_FRAGMENT_SHADER,
		stringFromFile("D:/Code/PrecomputedAtmosphericScattering/irradiance1.fs.glsl"),
		430
		).get()
	})
	{
		m_locId_transmittanceSampler = getUniformLocationId("transmittanceSampler");
		glUseProgram(m_id);
		glUniform1i(m_locId_transmittanceSampler, TEXTURE_UNIT_TRANSMITTANCE);
	}
};

class Inscatter1Program : public ShaderProgram
{
	GLuint m_locId_transmittanceSampler;

public:

	Inscatter1Program() :
		ShaderProgram({
		&FullscreenQuad::getVertexShaderStage(),
		LAYER_GEOMETRY_STAGE,
		std::make_unique<ShaderStage>(
		GL_FRAGMENT_SHADER,
		stringFromFile("D:/Code/PrecomputedAtmosphericScattering/inscatter1.fs.glsl"),
		430
		).get()
	})
	{
		m_locId_transmittanceSampler = getUniformLocationId("transmittanceSampler");
		glUseProgram(m_id);
		glUniform1i(m_locId_transmittanceSampler, TEXTURE_UNIT_TRANSMITTANCE);
	}
};

class CopyIrradianceProgram : public ShaderProgram
{
	GLuint m_locId_k;
	GLuint m_locId_deltaESampler;

public:

	CopyIrradianceProgram() :
		ShaderProgram({
		&FullscreenQuad::getVertexShaderStage(),
		std::make_unique<ShaderStage>(
		GL_FRAGMENT_SHADER,
		stringFromFile("D:/Code/PrecomputedAtmosphericScattering/copyIrradiance.fs.glsl"),
		430
		).get()
	})
	{
		m_locId_k = getUniformLocationId("k");
		m_locId_deltaESampler = getUniformLocationId("deltaESampler");
		glUseProgram(m_id);
		glUniform1i(m_locId_deltaESampler, TEXTURE_UNIT_DELTA_E);
	}

		void setK(float k)
		{
			glUseProgram(m_id);
			glUniform1f(m_locId_k, k);
		}
};

class CopyInscatter1Program : public ShaderProgram
{
	GLuint m_locId_deltaSRSampler;
	GLuint m_locId_deltaSMSampler;

public:

	CopyInscatter1Program() :
		ShaderProgram({
		&FullscreenQuad::getVertexShaderStage(),
		LAYER_GEOMETRY_STAGE,
		std::make_unique<ShaderStage>(
		GL_FRAGMENT_SHADER,
		stringFromFile("D:/Code/PrecomputedAtmosphericScattering/copyInscatter1.fs.glsl"),
		430
		).get()
	})
	{
		m_locId_deltaSRSampler = getUniformLocationId("deltaSRSampler");
		m_locId_deltaSMSampler = getUniformLocationId("deltaSMSampler");
		glUseProgram(m_id);
		glUniform1i(m_locId_deltaSRSampler, TEXTURE_UNIT_DELTA_SR);
		glUniform1i(m_locId_deltaSMSampler, TEXTURE_UNIT_DELTA_SM);
	}
};

class InscatterSProgram : public ShaderProgram
{
	GLuint m_locId_first;
	GLuint m_locId_transmittanceSampler;
	GLuint m_locId_deltaESampler;
	GLuint m_locId_deltaSRSampler;
	GLuint m_locId_deltaSMSampler;

public:

	InscatterSProgram() :
		ShaderProgram({
		&FullscreenQuad::getVertexShaderStage(),
		LAYER_GEOMETRY_STAGE,
		std::make_unique<ShaderStage>(
		GL_FRAGMENT_SHADER,
		stringFromFile("D:/Code/PrecomputedAtmosphericScattering/inscatterS.fs.glsl"),
		430
		).get()
	})
	{
		m_locId_first = getUniformLocationId("first");
		m_locId_transmittanceSampler = getUniformLocationId("transmittanceSampler");
		m_locId_deltaESampler = getUniformLocationId("deltaESampler");
		m_locId_deltaSRSampler = getUniformLocationId("deltaSRSampler");
		m_locId_deltaSMSampler = getUniformLocationId("deltaSMSampler");
		glUseProgram(m_id);
		glUniform1i(m_locId_transmittanceSampler, TEXTURE_UNIT_TRANSMITTANCE);
		glUniform1i(m_locId_deltaESampler, TEXTURE_UNIT_DELTA_E);
		glUniform1i(m_locId_deltaSRSampler, TEXTURE_UNIT_DELTA_SR);
		glUniform1i(m_locId_deltaSMSampler, TEXTURE_UNIT_DELTA_SM);
	}

		void setFirst(float first)
		{
			glUseProgram(m_id);
			glUniform1f(m_locId_first, first);
		}
};

class IrradianceNProgram : public ShaderProgram
{
	GLuint m_locId_first;
	GLuint m_locId_transmittanceSampler;
	GLuint m_locId_deltaSRSampler;
	GLuint m_locId_deltaSMSampler;

public:

	IrradianceNProgram() :
		ShaderProgram({
		&FullscreenQuad::getVertexShaderStage(),
		std::make_unique<ShaderStage>(
		GL_FRAGMENT_SHADER,
		stringFromFile("D:/Code/PrecomputedAtmosphericScattering/irradianceN.fs.glsl"),
		430
		).get()
	})
	{
		m_locId_first = getUniformLocationId("first");
		m_locId_transmittanceSampler = getUniformLocationId("transmittanceSampler");
		m_locId_deltaSRSampler = getUniformLocationId("deltaSRSampler");
		m_locId_deltaSMSampler = getUniformLocationId("deltaSMSampler");
		glUseProgram(m_id);
		glUniform1i(m_locId_transmittanceSampler, TEXTURE_UNIT_TRANSMITTANCE);
		glUniform1i(m_locId_deltaSRSampler, TEXTURE_UNIT_DELTA_SR);
		glUniform1i(m_locId_deltaSMSampler, TEXTURE_UNIT_DELTA_SM);
	}

		void setFirst(float first)
		{
			glUseProgram(m_id);
			glUniform1f(m_locId_first, first);
		}
};

class InscatterNProgram : public ShaderProgram
{
	GLuint m_locId_first;
	GLuint m_locId_transmittanceSampler;
	GLuint m_locId_deltaJSampler;

public:

	InscatterNProgram() :
		ShaderProgram({
		&FullscreenQuad::getVertexShaderStage(),
		LAYER_GEOMETRY_STAGE,
		std::make_unique<ShaderStage>(
		GL_FRAGMENT_SHADER,
		stringFromFile("D:/Code/PrecomputedAtmosphericScattering/inscatterN.fs.glsl"),
		430
		).get()
	})
	{
		m_locId_first = getUniformLocationId("first");
		m_locId_transmittanceSampler = getUniformLocationId("transmittanceSampler");
		m_locId_deltaJSampler = getUniformLocationId("deltaJSampler");
		glUseProgram(m_id);
		glUniform1i(m_locId_transmittanceSampler, TEXTURE_UNIT_TRANSMITTANCE);
		glUniform1i(m_locId_deltaJSampler, TEXTURE_UNIT_DELTA_J);
	}

		void setFirst(float first)
		{
			glUseProgram(m_id);
			glUniform1f(m_locId_first, first);
		}
};

class CopyInscatterNProgram : public ShaderProgram
{
	GLuint m_locId_deltaSSampler;

public:

	CopyInscatterNProgram() :
		ShaderProgram({
		&FullscreenQuad::getVertexShaderStage(),
		LAYER_GEOMETRY_STAGE,
		std::make_unique<ShaderStage>(
		GL_FRAGMENT_SHADER,
		stringFromFile("D:/Code/PrecomputedAtmosphericScattering/copyInscatterN.fs.glsl"),
		430
		).get()
	})
	{
		m_locId_deltaSSampler = getUniformLocationId("deltaSSampler");
		glUseProgram(m_id);
		glUniform1i(m_locId_deltaSSampler, TEXTURE_UNIT_DELTA_SR);
	}
};

class DrawProgram : public ShaderProgram
{
	GLuint m_locId_reflectanceSampler;
	GLuint m_locId_transmittanceSampler;
	GLuint m_locId_irradianceSampler;
	GLuint m_locId_inscatterSampler;

public:

	DrawProgram() :
		ShaderProgram(
	{ GL_VERTEX_SHADER, GL_FRAGMENT_SHADER },
	stringFromFile("D:/Code/PrecomputedAtmosphericScattering/earth.glsl"),
	430
	)
	{
		m_locId_reflectanceSampler = getUniformLocationId("reflectanceSampler");
		m_locId_transmittanceSampler = getUniformLocationId("transmittanceSampler");
		m_locId_irradianceSampler = getUniformLocationId("irradianceSampler");
		m_locId_inscatterSampler = getUniformLocationId("inscatterSampler");
		glUseProgram(m_id);
		glUniform1i(m_locId_reflectanceSampler, TEXTURE_UNIT_REFLECTANCE);
		glUniform1i(m_locId_transmittanceSampler, TEXTURE_UNIT_TRANSMITTANCE);
		glUniform1i(m_locId_irradianceSampler, TEXTURE_UNIT_IRRADIANCE);
		glUniform1i(m_locId_inscatterSampler, TEXTURE_UNIT_INSCATTER);
	}
};

TransmittanceProgram* TRANSMITTANCE_PROGRAM;
Irradiance1Program* IRRADIANCE_1_PROGRAM;
Inscatter1Program* INSCATTER_1_PROGRAM;
CopyIrradianceProgram* COPY_IRRADIANCE_PROGRAM;
CopyInscatter1Program* COPY_INSCATTER_1_PROGRAM;
InscatterSProgram* INSCATTER_S_PROGRAM;
IrradianceNProgram* IRRADIANCE_N_PROGRAM;
InscatterNProgram* INSCATTER_N_PROGRAM;
CopyInscatterNProgram* COPY_INSCATTER_N_PROGRAM;
DrawProgram* DRAW_PROGRAM;

/////////////// Uniform blocks ////////////////////

ResUniformBlock::ResUniformBlock() :
	UniformBlock(GL_STATIC_DRAW,
	{
	"RES_R", "RES_MU", "RES_MU_S", "RES_NU",
	"TRANSMITTANCE_W", "TRANSMITTANCE_H",
	"TRANSMITTANCE_INTEGRAL_SAMPLES", "IRRADIANCE_INTEGRAL_SAMPLES",
	"INSCATTER_INTEGRAL_SAMPLES", "INSCATTER_SPHERICAL_INTEGRAL_SAMPLES",
	"SKY_W", "SKY_H"
}
)
{
}

void ResUniformBlock::upload()
{
	glBindBuffer(GL_UNIFORM_BUFFER, m_bufferId);
	GLvoid* ptr = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
	copyScalar(ptr, m_offsets[0], &m_resR);
	copyScalar(ptr, m_offsets[1], &m_resMu);
	copyScalar(ptr, m_offsets[2], &m_resMuS);
	copyScalar(ptr, m_offsets[3], &m_resNu);
	copyScalar(ptr, m_offsets[4], &m_transmittance_w);
	copyScalar(ptr, m_offsets[5], &m_transmittance_h);
	copyScalar(ptr, m_offsets[6], &m_transmittance_integral_samples);
	copyScalar(ptr, m_offsets[7], &m_irradiance_integral_samples);
	copyScalar(ptr, m_offsets[8], &m_inscatter_integral_samples);
	copyScalar(ptr, m_offsets[9], &m_inscatter_spherical_integral_samples);
	copyScalar(ptr, m_offsets[10], &m_sky_w);
	copyScalar(ptr, m_offsets[11], &m_sky_h);
	glUnmapBuffer(GL_UNIFORM_BUFFER);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

PlanetUniformBlock::PlanetUniformBlock() :
	UniformBlock(GL_STATIC_DRAW, { "Rg", "Rt", "RL" })
{
}

void PlanetUniformBlock::upload()
{
	glBindBuffer(GL_UNIFORM_BUFFER, m_bufferId);
	GLvoid* ptr = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
	copyScalar(ptr, m_offsets[0], &m_rg);
	copyScalar(ptr, m_offsets[1], &m_rt);
	copyScalar(ptr, m_offsets[2], &m_rL);
	glUnmapBuffer(GL_UNIFORM_BUFFER);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

AtmosphereUniformBlock::AtmosphereUniformBlock() :
	UniformBlock(GL_STATIC_DRAW, { "betaR", "HR", "HM", "betaMSca", "betaMEx", "mieG" })
{
}

void AtmosphereUniformBlock::upload()
{
	glBindBuffer(GL_UNIFORM_BUFFER, m_bufferId);
	GLvoid* ptr = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
	copyArray(ptr, m_offsets[0], glm::value_ptr(m_betaR), 3);
	copyScalar(ptr, m_offsets[1], &m_hr);
	copyScalar(ptr, m_offsets[2], &m_hm);
	copyArray(ptr, m_offsets[3], glm::value_ptr(m_betaMSca), 3);
	copyArray(ptr, m_offsets[4], glm::value_ptr(m_betaMEx), 3);
	copyScalar(ptr, m_offsets[5], &m_mieG);
	glUnmapBuffer(GL_UNIFORM_BUFFER);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

struct LayerUniformBlock : public UniformBlock
{
	LayerUniformBlock() :
	UniformBlock(GL_DYNAMIC_DRAW, { "r", "dhdH", "layer" })
	{
	}

	void setValues(float r, float* dhdH, int layer)
	{
		glBindBuffer(GL_UNIFORM_BUFFER, m_bufferId);
		GLvoid* ptr = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
		copyScalar(ptr, m_offsets[0], &r);
		copyArray(ptr, m_offsets[1], dhdH, 4);
		copyScalar(ptr, m_offsets[2], &layer);
		glUnmapBuffer(GL_UNIFORM_BUFFER);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}
};

DynamicDrawUniformBlock::DynamicDrawUniformBlock() :
	UniformBlock(GL_DYNAMIC_DRAW, { "projInverse", "viewInverse", "cameraPos", "sunDirection", "exposure", "iSun" })
{
}

void DynamicDrawUniformBlock::setValues(
	const glm::mat4& invProj, const glm::mat4& invView,
	const glm::vec3& cameraPos, const glm::vec3& sunDirection,
	float exposure, float iSun
	)
{
	glBindBuffer(GL_UNIFORM_BUFFER, m_bufferId);
	GLvoid* ptr = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
	copyArray(ptr, m_offsets[0], glm::value_ptr(invProj), 16);
	copyArray(ptr, m_offsets[1], glm::value_ptr(invView), 16);
	copyArray(ptr, m_offsets[2], glm::value_ptr(cameraPos), 3);
	copyArray(ptr, m_offsets[3], glm::value_ptr(sunDirection), 3);
	copyScalar(ptr, m_offsets[4], &exposure);
	copyScalar(ptr, m_offsets[5], &iSun);
	glUnmapBuffer(GL_UNIFORM_BUFFER);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

///////////////////////////////////////////////////////////////

void Atmosphere::initialiseShaders()
{
	const std::string layerGeomCode = R"(

layout (shared) uniform LayerUniformBlock
{
float r;
vec4 dhdH;
int layer;
};

layout (triangles) in;
layout (triangle_strip, max_vertices=3) out;

void main() 
{
gl_Position = gl_in[0].gl_Position;
gl_Layer = layer;
EmitVertex();

gl_Position = gl_in[1].gl_Position;
gl_Layer = layer;
EmitVertex();

gl_Position = gl_in[2].gl_Position;
gl_Layer = layer;
EmitVertex();

EndPrimitive();
}

)";
	LAYER_GEOMETRY_STAGE = new ShaderStage(GL_GEOMETRY_SHADER, layerGeomCode, 430);

	// Create programs and set up static uniform index bindings - these never need to change.
	TRANSMITTANCE_PROGRAM = new TransmittanceProgram();
	TRANSMITTANCE_PROGRAM->setUniformBlockBinding("ResUniformBlock", UNIFORM_BINDING_INDEX_RES);
	TRANSMITTANCE_PROGRAM->setUniformBlockBinding("PlanetUniformBlock", UNIFORM_BINDING_INDEX_PLANET);
	TRANSMITTANCE_PROGRAM->setUniformBlockBinding("AtmosphereUniformBlock", UNIFORM_BINDING_INDEX_ATMOSPHERE);

	IRRADIANCE_1_PROGRAM = new Irradiance1Program();
	IRRADIANCE_1_PROGRAM->setUniformBlockBinding("ResUniformBlock", UNIFORM_BINDING_INDEX_RES);
	IRRADIANCE_1_PROGRAM->setUniformBlockBinding("PlanetUniformBlock", UNIFORM_BINDING_INDEX_PLANET);

	INSCATTER_1_PROGRAM = new Inscatter1Program();
	INSCATTER_1_PROGRAM->setUniformBlockBinding("ResUniformBlock", UNIFORM_BINDING_INDEX_RES);
	INSCATTER_1_PROGRAM->setUniformBlockBinding("PlanetUniformBlock", UNIFORM_BINDING_INDEX_PLANET);
	INSCATTER_1_PROGRAM->setUniformBlockBinding("AtmosphereUniformBlock", UNIFORM_BINDING_INDEX_ATMOSPHERE);
	INSCATTER_1_PROGRAM->setUniformBlockBinding("LayerUniformBlock", UNIFORM_BINDING_INDEX_LAYER);

	COPY_IRRADIANCE_PROGRAM = new CopyIrradianceProgram();
	COPY_IRRADIANCE_PROGRAM->setUniformBlockBinding("ResUniformBlock", UNIFORM_BINDING_INDEX_RES);

	COPY_INSCATTER_1_PROGRAM = new CopyInscatter1Program();
	COPY_INSCATTER_1_PROGRAM->setUniformBlockBinding("ResUniformBlock", UNIFORM_BINDING_INDEX_RES);
	COPY_INSCATTER_1_PROGRAM->setUniformBlockBinding("LayerUniformBlock", UNIFORM_BINDING_INDEX_LAYER);

	INSCATTER_S_PROGRAM = new InscatterSProgram();
	INSCATTER_S_PROGRAM->setUniformBlockBinding("ResUniformBlock", UNIFORM_BINDING_INDEX_RES);
	INSCATTER_S_PROGRAM->setUniformBlockBinding("PlanetUniformBlock", UNIFORM_BINDING_INDEX_PLANET);
	INSCATTER_S_PROGRAM->setUniformBlockBinding("AtmosphereUniformBlock", UNIFORM_BINDING_INDEX_ATMOSPHERE);
	INSCATTER_S_PROGRAM->setUniformBlockBinding("LayerUniformBlock", UNIFORM_BINDING_INDEX_LAYER);

	IRRADIANCE_N_PROGRAM = new IrradianceNProgram();
	IRRADIANCE_N_PROGRAM->setUniformBlockBinding("ResUniformBlock", UNIFORM_BINDING_INDEX_RES);
	IRRADIANCE_N_PROGRAM->setUniformBlockBinding("PlanetUniformBlock", UNIFORM_BINDING_INDEX_PLANET);
	IRRADIANCE_N_PROGRAM->setUniformBlockBinding("AtmosphereUniformBlock", UNIFORM_BINDING_INDEX_ATMOSPHERE);

	INSCATTER_N_PROGRAM = new InscatterNProgram();
	INSCATTER_N_PROGRAM->setUniformBlockBinding("ResUniformBlock", UNIFORM_BINDING_INDEX_RES);
	INSCATTER_N_PROGRAM->setUniformBlockBinding("PlanetUniformBlock", UNIFORM_BINDING_INDEX_PLANET);
	INSCATTER_N_PROGRAM->setUniformBlockBinding("LayerUniformBlock", UNIFORM_BINDING_INDEX_LAYER);

	COPY_INSCATTER_N_PROGRAM = new CopyInscatterNProgram();
	COPY_INSCATTER_N_PROGRAM->setUniformBlockBinding("ResUniformBlock", UNIFORM_BINDING_INDEX_RES);
	COPY_INSCATTER_N_PROGRAM->setUniformBlockBinding("PlanetUniformBlock", UNIFORM_BINDING_INDEX_PLANET);
	COPY_INSCATTER_N_PROGRAM->setUniformBlockBinding("LayerUniformBlock", UNIFORM_BINDING_INDEX_LAYER);

	DRAW_PROGRAM = new DrawProgram();
	DRAW_PROGRAM->setUniformBlockBinding("ResUniformBlock", UNIFORM_BINDING_INDEX_RES);
	DRAW_PROGRAM->setUniformBlockBinding("PlanetUniformBlock", UNIFORM_BINDING_INDEX_PLANET);
	DRAW_PROGRAM->setUniformBlockBinding("AtmosphereUniformBlock", UNIFORM_BINDING_INDEX_ATMOSPHERE);
	DRAW_PROGRAM->setUniformBlockBinding("DynamicDrawUniformBlock", UNIFORM_BINDING_INDEX_DYNAMICDRAW);
}

GLuint Atmosphere::makeEmptyTex2D(int samplerUnit, int width, int height)
{
	GLuint textureId;

	glActiveTexture(GL_TEXTURE0 + samplerUnit);
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D, textureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F_ARB, width, height, 0, GL_RGB, GL_FLOAT, NULL);

	return textureId;
}

GLuint Atmosphere::makeEmptyTex3D(int samplerUnit, int width, int height, int depth)
{
	GLuint textureId;

	glActiveTexture(GL_TEXTURE0 + samplerUnit);
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_3D, textureId);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA16F_ARB, width, height, depth, 0, GL_RGB, GL_FLOAT, NULL);

	return textureId;
}

////////////////////////////////////////////////////////////////

Atmosphere::Atmosphere(
	GLuint reflectanceTexture,
	const std::shared_ptr<ResUniformBlock>& resUniformBlock,
	const std::shared_ptr<PlanetUniformBlock>& planetUniformBlock,
	const std::shared_ptr<AtmosphereUniformBlock>& atmosphereUniformBlock,
	const std::shared_ptr<DynamicDrawUniformBlock>& dynamicDrawUniformBlock
	) :
	m_resUniformBlock(resUniformBlock),
	m_planetUniformBlock(planetUniformBlock),
	m_atmosphereUniformBlock(atmosphereUniformBlock),
	m_dynamicDrawUniformBlock(dynamicDrawUniformBlock)
{
	// We only need this uniform block during construction
	LayerUniformBlock layerUniformBlock; // TODO: Generate these dynamically
	glBindBufferBase(GL_UNIFORM_BUFFER, UNIFORM_BINDING_INDEX_LAYER, layerUniformBlock.m_bufferId);
	glBindBufferBase(GL_UNIFORM_BUFFER, UNIFORM_BINDING_INDEX_RES, m_resUniformBlock->m_bufferId);
	glBindBufferBase(GL_UNIFORM_BUFFER, UNIFORM_BINDING_INDEX_PLANET, m_planetUniformBlock->m_bufferId);
	glBindBufferBase(GL_UNIFORM_BUFFER, UNIFORM_BINDING_INDEX_ATMOSPHERE, m_atmosphereUniformBlock->m_bufferId);

	// Make permanent textures
	m_reflectanceTexture = reflectanceTexture;
	m_transmittanceTexture = makeEmptyTex2D(TEXTURE_UNIT_TRANSMITTANCE, resUniformBlock->m_transmittance_w, resUniformBlock->m_transmittance_h);
	m_irradianceTexture = makeEmptyTex2D(TEXTURE_UNIT_IRRADIANCE, resUniformBlock->m_sky_w, resUniformBlock->m_sky_h);
	m_inscatterTexture = makeEmptyTex3D(TEXTURE_UNIT_INSCATTER, resUniformBlock->m_resMuS * resUniformBlock->m_resNu, resUniformBlock->m_resMu, resUniformBlock->m_resR);

	// Make transient textures and bind them to the correct active textures
	const GLuint deltaETexture = makeEmptyTex2D(TEXTURE_UNIT_DELTA_E, resUniformBlock->m_sky_w, resUniformBlock->m_sky_h);
	const GLuint deltaSRTexture = makeEmptyTex3D(TEXTURE_UNIT_DELTA_SR, resUniformBlock->m_resMuS * resUniformBlock->m_resNu, resUniformBlock->m_resMu, resUniformBlock->m_resR);
	const GLuint deltaSMTexture = makeEmptyTex3D(TEXTURE_UNIT_DELTA_SM, resUniformBlock->m_resMuS * resUniformBlock->m_resNu, resUniformBlock->m_resMu, resUniformBlock->m_resR);
	const GLuint deltaJTexture = makeEmptyTex3D(TEXTURE_UNIT_DELTA_J, resUniformBlock->m_resMuS * resUniformBlock->m_resNu, resUniformBlock->m_resMu, resUniformBlock->m_resR);

	// Configure offsets for each uniform block by pointing them to a random program 
	// that uses the block. Because the definition of the block is the same within
	// each program (layout=shared), offsets are constant.
	resUniformBlock->setOffsetsFromProgram(*COPY_INSCATTER_1_PROGRAM, "ResUniformBlock");
	resUniformBlock->upload();
	planetUniformBlock->setOffsetsFromProgram(*DRAW_PROGRAM, "PlanetUniformBlock");
	planetUniformBlock->upload();
	layerUniformBlock.setOffsetsFromProgram(*INSCATTER_1_PROGRAM, "LayerUniformBlock");
	atmosphereUniformBlock->setOffsetsFromProgram(*DRAW_PROGRAM, "AtmosphereUniformBlock");
	atmosphereUniformBlock->upload();

	dynamicDrawUniformBlock->setOffsetsFromProgram(*DRAW_PROGRAM, "DynamicDrawUniformBlock");

	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	// computes transmittance texture T (line 1 in algorithm 4.1)
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_transmittanceTexture, 0);
	glViewport(0, 0, resUniformBlock->m_transmittance_w, resUniformBlock->m_transmittance_h);
	glUseProgram(TRANSMITTANCE_PROGRAM->m_id);
	FullscreenQuad::draw();

	// computes irradiance texture deltaE (line 2 in algorithm 4.1)
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, deltaETexture, 0);
	glViewport(0, 0, resUniformBlock->m_sky_w, resUniformBlock->m_sky_h);
	glUseProgram(IRRADIANCE_1_PROGRAM->m_id);
	FullscreenQuad::draw();

	// computes single scattering texture deltaS (line 3 in algorithm 4.1)
	// Rayleigh and Mie separated in deltaSR + deltaSM
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, deltaSRTexture, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, deltaSMTexture, 0);
	unsigned int bufs[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, bufs);
	glViewport(0, 0, resUniformBlock->m_resMuS * resUniformBlock->m_resNu, resUniformBlock->m_resMu);
	glUseProgram(INSCATTER_1_PROGRAM->m_id);
	for (int layer = 0; layer < resUniformBlock->m_resR; ++layer)
	{
		setLayer(&layerUniformBlock, layer);
		FullscreenQuad::draw();
	}
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, 0, 0);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	// copies deltaE into irradiance texture E (line 4 in algorithm 4.1)
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_irradianceTexture, 0);
	glViewport(0, 0, resUniformBlock->m_sky_w, resUniformBlock->m_sky_h);
	glUseProgram(COPY_IRRADIANCE_PROGRAM->m_id);
	COPY_IRRADIANCE_PROGRAM->setK(0.0);
	FullscreenQuad::draw();

	// copies deltaS into inscatter texture S (line 5 in algorithm 4.1)
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_inscatterTexture, 0);
	glViewport(0, 0, resUniformBlock->m_resMuS * resUniformBlock->m_resNu, resUniformBlock->m_resMu);
	glUseProgram(COPY_INSCATTER_1_PROGRAM->m_id);
	for (int layer = 0; layer < resUniformBlock->m_resR; ++layer)
	{
		setLayer(&layerUniformBlock, layer);
		FullscreenQuad::draw();
	}

	// loop for each scattering order (line 6 in algorithm 4.1)
	for (int order = 2; order <= 4; ++order)
	{
		// computes deltaJ (line 7 in algorithm 4.1)
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, deltaJTexture, 0);
		glViewport(0, 0, resUniformBlock->m_resMuS * resUniformBlock->m_resNu, resUniformBlock->m_resMu);
		glUseProgram(INSCATTER_S_PROGRAM->m_id);
		INSCATTER_S_PROGRAM->setFirst(order == 2 ? 1.0f : 0.0f);
		for (int layer = 0; layer < resUniformBlock->m_resR; ++layer)
		{
			setLayer(&layerUniformBlock, layer);
			FullscreenQuad::draw();
		}

		// computes deltaE (line 8 in algorithm 4.1)
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, deltaETexture, 0);
		glViewport(0, 0, resUniformBlock->m_sky_w, resUniformBlock->m_sky_h);
		glUseProgram(IRRADIANCE_N_PROGRAM->m_id);
		IRRADIANCE_N_PROGRAM->setFirst(order == 2 ? 1.0f : 0.0f);
		FullscreenQuad::draw();

		// computes deltaS (line 9 in algorithm 4.1)
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, deltaSRTexture, 0);
		glViewport(0, 0, resUniformBlock->m_resMuS * resUniformBlock->m_resNu, resUniformBlock->m_resMu);
		glUseProgram(INSCATTER_N_PROGRAM->m_id);
		INSCATTER_N_PROGRAM->setFirst(order == 2 ? 1.0f : 0.0f);
		for (int layer = 0; layer < resUniformBlock->m_resR; ++layer)
		{
			setLayer(&layerUniformBlock, layer);
			FullscreenQuad::draw();
		}

		glEnable(GL_BLEND);
		glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
		glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ONE);

		// adds deltaE into irradiance texture E (line 10 in algorithm 4.1)
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_irradianceTexture, 0);
		glViewport(0, 0, resUniformBlock->m_sky_w, resUniformBlock->m_sky_h);
		glUseProgram(COPY_IRRADIANCE_PROGRAM->m_id);
		COPY_IRRADIANCE_PROGRAM->setK(1.0);
		FullscreenQuad::draw();

		// adds deltaS into inscatter texture S (line 11 in algorithm 4.1)
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_inscatterTexture, 0);
		glViewport(0, 0, resUniformBlock->m_resMuS * resUniformBlock->m_resNu, resUniformBlock->m_resMu);
		glUseProgram(COPY_INSCATTER_N_PROGRAM->m_id);
		for (int layer = 0; layer < resUniformBlock->m_resR; ++layer)
		{
			setLayer(&layerUniformBlock, layer);
			FullscreenQuad::draw();
		}

		glDisable(GL_BLEND);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

Atmosphere::~Atmosphere()
{
	// TODO: delete stuff
}

void Atmosphere::setLayer(LayerUniformBlock* block, int layer)
{
	const float Rg = m_planetUniformBlock->m_rg;
	const float Rt = m_planetUniformBlock->m_rt;

	double r = layer / (m_resUniformBlock->m_resR - 1.0);
	r = r * r;
	r = sqrt(Rg * Rg + r * (Rt * Rt - Rg * Rg)) + (layer == 0 ? 0.01 : (layer == m_resUniformBlock->m_resR - 1 ? -0.001 : 0.0));
	double dmin = Rt - r;
	double dmax = sqrt(r * r - Rg * Rg) + sqrt(Rt * Rt - Rg * Rg);
	double dminp = r - Rg;
	double dmaxp = sqrt(r * r - Rg * Rg);

	float dhdH[4] = { float(dmin), float(dmax), float(dminp), float(dmaxp) };
	block->setValues((float)r, dhdH, layer);
}

void Atmosphere::draw()
{
	// Rebind our textures to the texture units
	glActiveTexture(GL_TEXTURE0 + TEXTURE_UNIT_REFLECTANCE);
	glBindTexture(GL_TEXTURE_2D, m_reflectanceTexture);
	glActiveTexture(GL_TEXTURE0 + TEXTURE_UNIT_TRANSMITTANCE);
	glBindTexture(GL_TEXTURE_2D, m_transmittanceTexture);
	glActiveTexture(GL_TEXTURE0 + TEXTURE_UNIT_IRRADIANCE);
	glBindTexture(GL_TEXTURE_2D, m_irradianceTexture);
	glActiveTexture(GL_TEXTURE0 + TEXTURE_UNIT_INSCATTER);
	glBindTexture(GL_TEXTURE_3D, m_inscatterTexture);

	// Point the global uniform binding indices towards our instance's specific uniform buffers
	glBindBufferBase(GL_UNIFORM_BUFFER, UNIFORM_BINDING_INDEX_RES, m_resUniformBlock->m_bufferId);
	glBindBufferBase(GL_UNIFORM_BUFFER, UNIFORM_BINDING_INDEX_PLANET, m_planetUniformBlock->m_bufferId);
	glBindBufferBase(GL_UNIFORM_BUFFER, UNIFORM_BINDING_INDEX_ATMOSPHERE, m_atmosphereUniformBlock->m_bufferId);
	glBindBufferBase(GL_UNIFORM_BUFFER, UNIFORM_BINDING_INDEX_DYNAMICDRAW, m_dynamicDrawUniformBlock->m_bufferId);

	glUseProgram(DRAW_PROGRAM->m_id);
	FullscreenQuad::draw();
}

} // end namespace Bruneton
