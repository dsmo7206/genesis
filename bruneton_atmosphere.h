#pragma once

#include <memory>
#include "shader_program.h"
#include "uniform_block.h"

std::string stringFromFile(const char* filename);

namespace Bruneton {

/////////////////// Uniform blocks /////////////////////

struct ResUniformBlock : public UniformBlock
{
	int m_resR;
	int m_resMu;
	int m_resMuS;
	int m_resNu;
	int m_transmittance_w;
	int m_transmittance_h;
	int m_transmittance_integral_samples;
	int m_irradiance_integral_samples;
	int m_inscatter_integral_samples;
	int m_inscatter_spherical_integral_samples;
	int m_sky_w;
	int m_sky_h;

	ResUniformBlock();
	void upload();
};

struct PlanetUniformBlock : public UniformBlock
{
	float m_rg;
	float m_rt;
	float m_rL;

	PlanetUniformBlock();
	void upload();
};

struct AtmosphereUniformBlock : public UniformBlock
{
	glm::vec3 m_betaR;
	float m_hr;
	float m_hm;
	glm::vec3 m_betaMSca;
	glm::vec3 m_betaMEx;
	float m_mieG;

	AtmosphereUniformBlock();
	void upload();
};

struct DynamicDrawUniformBlock : public UniformBlock
{
	DynamicDrawUniformBlock();
	void setValues(
		const glm::mat4& invProj, const glm::mat4& invView,
		const glm::vec3& cameraPos, const glm::vec3& sunDirection,
		float exposure, float iSun
	);
};

struct LayerUniformBlock;

class Atmosphere
{
	GLuint m_reflectanceTexture, m_transmittanceTexture, m_irradianceTexture, m_inscatterTexture;

	// The uniform blocks we need for drawing each frame
	std::shared_ptr<ResUniformBlock> m_resUniformBlock;
	std::shared_ptr<PlanetUniformBlock> m_planetUniformBlock;
	std::shared_ptr<AtmosphereUniformBlock> m_atmosphereUniformBlock;

	void setLayer(LayerUniformBlock* block, int layer);
	GLuint makeEmptyTex2D(int samplerUnit, int width, int height);
	GLuint makeEmptyTex3D(int samplerUnit, int width, int height, int depth);

public:

	std::shared_ptr<DynamicDrawUniformBlock> m_dynamicDrawUniformBlock;

	Atmosphere(
		GLuint reflectanceTexture,
		const std::shared_ptr<ResUniformBlock>& resUniformBlock,
		const std::shared_ptr<PlanetUniformBlock>& planetUniformBlock,
		const std::shared_ptr<AtmosphereUniformBlock>& atmosphereUniformBlock,
		const std::shared_ptr<DynamicDrawUniformBlock>& dynamicDrawUniformBlock
	);

	~Atmosphere();

	void draw();
	static void initialiseShaders();
};

} // end namespace Bruneton
