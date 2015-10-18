#include <stdio.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <stdlib.h>
#include <string.h>
#include <cerrno>

#include <GL/glew.h>

#include "utils.h"
#include "shader_program.h"
#include "planet_data_buffer.h"

static const char* DEFINE_VERTEX = "#define _VERTEX_";
static const char* DEFINE_GEOMETRY = "#define _GEOMETRY_";
static const char* DEFINE_FRAGMENT = "#define _FRAGMENT_";
static const char* DEFINE_COMPUTE = "#define _COMPUTE_";

static std::string intToString(int value)
{
	char str[8] = {0};
	_itoa_s(value, str, 10);
	return std::string(str);
}

static GLuint compileShader(GLenum shaderType, const std::string& _code, int version)
{
	std::ostringstream oss;

	if (version != 0)
		oss << "#version " << version << " core\n";
	else
		oss << "#version 430 core\n";

	switch (shaderType)
	{
	case GL_VERTEX_SHADER:
		oss << DEFINE_VERTEX << "\n";
		break;
	case GL_GEOMETRY_SHADER:
		oss << DEFINE_GEOMETRY << "\n";
		break;
	case GL_FRAGMENT_SHADER:
		oss << DEFINE_FRAGMENT << "\n";
		break;
	case GL_COMPUTE_SHADER:
		oss << DEFINE_COMPUTE << "\n";
		break;
	default:
		throw std::runtime_error("Unknown shader type");
	}

	oss << _code;
	const std::string code = oss.str();

	const GLuint shaderId = glCreateShader(shaderType);

	char const* codePtr = code.c_str();
	glShaderSource(shaderId, 1, &codePtr, NULL);
	glCompileShader(shaderId);

	GLint result = GL_FALSE;
	int infoLogLength;

	glGetShaderiv(shaderId, GL_COMPILE_STATUS, &result);
	glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &infoLogLength);

	if (result == GL_FALSE)
	{
		printf("%s\n", code.c_str());

		std::vector<char> errorMessage(infoLogLength + 1);
		glGetShaderInfoLog(shaderId, infoLogLength, NULL, &errorMessage[0]);
		printf("%s\n", &errorMessage[0]);
		throw std::exception("Bad shader!");
	}

	return shaderId;
}

static GLuint linkShaderProgram(const std::vector<const ShaderStage*>& stages)
{
	const GLuint programId = glCreateProgram();

	for (auto it = stages.begin(); it != stages.end(); ++it)
		glAttachShader(programId, (*it)->m_id);

	glLinkProgram(programId);

	GLint result = GL_FALSE;
	int infoLogLength;

	glGetProgramiv(programId, GL_LINK_STATUS, &result);
	glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &infoLogLength);
	
	if (result == GL_FALSE)
	{
		std::vector<char> errorMessage(infoLogLength + 1);
		glGetProgramInfoLog(programId, infoLogLength, NULL, &errorMessage[0]);
		printf("Error linking shader program: %s\n", &errorMessage[0]);
		throw std::exception("Can't link shader");
	}

	return programId;
}

static GLuint linkShaderProgram(const std::vector<GLenum>& types, const std::string& code, int version)
{
	std::vector<const ShaderStage*> stages;

	for (auto type : types)
		stages.push_back(new ShaderStage(type, code, version));

	const GLuint result = linkShaderProgram(stages);

	for (auto stage : stages)
		delete stage;

	return result;
}

std::string buildComputeShaderSource(const std::string& lib)
{
	const std::string numPointsStr = intToString(PLANET_PATCH_CONSTANTS->m_verticesPerSide + 2);
	const std::string numPatchesStr = intToString(PLANET_PATCH_CONSTANTS->m_patchesPerBatch);

	std::stringstream oss;
	oss << 
		"#define NUM_POINTS " << numPointsStr << "\n" <<
		"#define PATCHES_PER_COMPUTE_BATCH " << numPatchesStr + "\n\n" <<
		lib << "\n\n" <<
		stringFromFile("terrain_cs.glsl")
	;
	return oss.str();
}

ShaderStage::ShaderStage(GLenum type, const std::string& code, int version) :
m_type(type), m_id(compileShader(type, code, version))
{
}

ShaderProgram::ShaderProgram(const std::initializer_list<GLenum>& types, const std::string& code, int version) :
m_id(linkShaderProgram(types, code, version))
{
}

ShaderProgram::ShaderProgram(const std::vector<const ShaderStage*>& stages) :
	m_id(linkShaderProgram(stages))
{
}

ShaderProgram::ShaderProgram(const std::initializer_list<const ShaderStage*>& stages) :
	m_id(linkShaderProgram(stages))
{
}

namespace ShaderStages
{
	namespace Vertex
	{
		ShaderStage* terrainNoAtm = nullptr;
		ShaderStage* terrainInAtm = nullptr;
		ShaderStage* terrainOutAtm = nullptr;
		ShaderStage* bruneton_init = nullptr;
		ShaderStage* bruneton_fft = nullptr;
		ShaderStage* bruneton_variances = nullptr;
		ShaderStage* bruneton_render = nullptr;
		ShaderStage* simpleWater = nullptr;
		ShaderStage* skyInAtm = nullptr;
		ShaderStage* skyOutAtm = nullptr;
		ShaderStage* skyBox = nullptr;

		void initialise()
		{
			terrainNoAtm = new ShaderStage(
				GL_VERTEX_SHADER,
				std::string("#define ATMOSPHERE 0\n") +
				stringFromFile("terrain_vs.glsl")
			);
			terrainInAtm = new ShaderStage(
				GL_VERTEX_SHADER, 
				std::string("#define ATMOSPHERE 1\n") +
				std::string("#define OUTSIDE_ATMOSPHERE 0\n") +
				stringFromFile("terrain_vs.glsl")
			);
			terrainOutAtm = new ShaderStage(
				GL_VERTEX_SHADER, 
				std::string("#define ATMOSPHERE 1\n") +
				std::string("#define OUTSIDE_ATMOSPHERE 1\n") +
				stringFromFile("terrain_vs.glsl")
			);
			/*
			bruneton_init = new ShaderStage(
				GL_VERTEX_SHADER,
				stringFromFile("bruneton_init_vs.glsl")
			);
			bruneton_fft = new ShaderStage(
				GL_VERTEX_SHADER,
				stringFromFile("bruneton_fft_vs.glsl")
			);
			bruneton_variances = new ShaderStage(
				GL_VERTEX_SHADER,
				stringFromFile("bruneton_variances_vs.glsl")
			);
			bruneton_render = new ShaderStage(
				GL_VERTEX_SHADER,
				stringFromFile("bruneton_render_vs.glsl")
			);
			*/
			simpleWater = new ShaderStage(
				GL_VERTEX_SHADER,
				stringFromFile("simplewater_vs.glsl")
			);
			skyInAtm = new ShaderStage(
				GL_VERTEX_SHADER, 
				std::string("#define OUTSIDE_ATMOSPHERE 0\n") +
				stringFromFile("sky_vs.glsl")
			);
			skyOutAtm = new ShaderStage(
				GL_VERTEX_SHADER, 
				std::string("#define OUTSIDE_ATMOSPHERE 1\n") +
				stringFromFile("sky_vs.glsl")
			);
			skyBox = new ShaderStage(
				GL_VERTEX_SHADER,
				stringFromFile("skybox_vs.glsl")
			);
		}
	}

	namespace Geometry
	{
		ShaderStage* bruneton_fft = nullptr;

		void initialise()
		{
			bruneton_fft = new ShaderStage(
				GL_GEOMETRY_SHADER,
				stringFromFile("bruneton_fft_gs.glsl")
			);
		}
	}

	namespace Fragment
	{
		ShaderStage* terrainNoAtm = nullptr;
		ShaderStage* terrainAtm = nullptr;
		ShaderStage* bruneton_init = nullptr;
		ShaderStage* bruneton_fftx = nullptr;
		ShaderStage* bruneton_ffty = nullptr;
		ShaderStage* bruneton_variances = nullptr;
		ShaderStage* bruneton_render = nullptr;
		ShaderStage* simpleWater = nullptr;
		ShaderStage* sky = nullptr;
		ShaderStage* skyBox = nullptr;

		void initialise()
		{
			terrainNoAtm = new ShaderStage(
				GL_FRAGMENT_SHADER, 
				std::string("#define ATMOSPHERE 0\n") +
				stringFromFile("terrain_fs.glsl")
			);
			terrainAtm = new ShaderStage(
				GL_FRAGMENT_SHADER, 
				std::string("#define ATMOSPHERE 1\n") +
				stringFromFile("terrain_fs.glsl")
			);
			/*
			bruneton_init = new ShaderStage(
				GL_FRAGMENT_SHADER,
				stringFromFile("bruneton_init_fs.glsl")
			);
			bruneton_fftx = new ShaderStage(
				GL_FRAGMENT_SHADER,
				stringFromFile("bruneton_fftx_fs.glsl")
			);
			bruneton_ffty = new ShaderStage(
				GL_FRAGMENT_SHADER,
				stringFromFile("bruneton_ffty_fs.glsl")
			);
			bruneton_variances = new ShaderStage(
				GL_FRAGMENT_SHADER,
				stringFromFile("bruneton_variances_fs.glsl")
			);
			bruneton_render = new ShaderStage(
				GL_FRAGMENT_SHADER,
				stringFromFile("bruneton_render_fs.glsl")
			);
			*/
			simpleWater = new ShaderStage(
				GL_FRAGMENT_SHADER,
				stringFromFile("simplewater_fs.glsl")
			);
			sky = new ShaderStage(
				GL_FRAGMENT_SHADER, 
				stringFromFile("sky_fs.glsl")
			);
			skyBox = new ShaderStage(
				GL_FRAGMENT_SHADER,
				stringFromFile("skybox_fs.glsl")
			);
		}
	}

	namespace Compute
	{
		ShaderStage* terrainGenRidgedMF = nullptr;
		ShaderStage* terrainGenLibnoise = nullptr;

		void initialise()
		{
			// Need to compile in the number of vertices and patches per batch
			terrainGenRidgedMF = new ShaderStage(
				GL_COMPUTE_SHADER, 
				buildComputeShaderSource(
					stringFromFile("lib_ridgedmf.glsl") + "\n" + 
					stringFromFile("lib_gustavsson_perlin.glsl")
				)
			);
			terrainGenLibnoise = new ShaderStage(
				GL_COMPUTE_SHADER, 
				buildComputeShaderSource(stringFromFile("lib_libnoise.glsl"))
			);
		}
	}

	void initialise()
	{
		printf("Compiling shaders!!!!!!!!!!!!!!!!!!!\n");
		Vertex::initialise();
		Geometry::initialise();
		Fragment::initialise();
		Compute::initialise();
		printf("Compiled shaders!!!!!!!!!!!!!!!!!!!\n");
	}
}
