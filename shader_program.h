#pragma once

#include <string>
#include <vector>
#include <initializer_list>

#include "glstuff.h"

struct ShaderStage
{
	const GLenum m_type;
	const GLuint m_id;

	ShaderStage(GLenum type, const std::string& code, int version = 0);
	~ShaderStage() { glDeleteShader(m_id); }
};

struct ShaderProgram
{
	const GLuint m_id;

	ShaderProgram(const std::initializer_list<GLenum>& types, const std::string& code, int version = 0);
	ShaderProgram(const std::vector<const ShaderStage*>& stages);
	ShaderProgram(const std::initializer_list<const ShaderStage*>& stages);

	virtual ~ShaderProgram() { glDeleteProgram(m_id); }

	inline GLint getUniformLocationId(const GLchar* name) const
	{
		return glGetUniformLocation(m_id, name);
	}

	inline GLuint getUniformBlockIndex(const char* name) const
	{
		return glGetUniformBlockIndex(m_id, name);
	}

	inline GLint getUniformBlockSize(const char* name) const
	{
		GLint blockSize;
		glGetActiveUniformBlockiv(m_id, getUniformBlockIndex(name), GL_UNIFORM_BLOCK_DATA_SIZE, &blockSize);
		return blockSize;
	}

	inline void getUniformOffsets(int numNames, const char* names[], GLint* offsets) const
	{
		// Map names to indices
		GLuint* indices = new GLuint[numNames];
		glGetUniformIndices(m_id, numNames, names, indices);

		// Map indices to offsets
		glGetActiveUniformsiv(m_id, numNames, indices, GL_UNIFORM_OFFSET, offsets);
		delete[] indices;
	}

	inline void setUniformBlockBinding(const char* uniformBlockName, GLuint bindingPoint)
	{
		const GLuint blockIndex = getUniformBlockIndex(uniformBlockName);
		glUniformBlockBinding(m_id, blockIndex, bindingPoint);
	}
};

// Register shader stages here
namespace ShaderStages
{
	void initialise();

	namespace Vertex
	{
		extern ShaderStage* terrainNoAtm;
		extern ShaderStage* terrainInAtm;
		extern ShaderStage* terrainOutAtm;
		extern ShaderStage* bruneton_init;
		extern ShaderStage* bruneton_fft;
		extern ShaderStage* bruneton_variances;
		extern ShaderStage* bruneton_render;
		extern ShaderStage* simpleWater;
		extern ShaderStage* skyInAtm;
		extern ShaderStage* skyOutAtm;
		extern ShaderStage* skyBox;
	}

	namespace Geometry
	{
		extern ShaderStage* bruneton_fft;
	}

	namespace Fragment
	{
		extern ShaderStage* terrainNoAtm;
		extern ShaderStage* terrainAtm;
		extern ShaderStage* bruneton_init;
		extern ShaderStage* bruneton_fftx;
		extern ShaderStage* bruneton_ffty;
		extern ShaderStage* bruneton_variances;
		extern ShaderStage* bruneton_render;
		extern ShaderStage* simpleWater;
		extern ShaderStage* sky;
		extern ShaderStage* skyBox;
	}

	namespace Compute
	{
		extern ShaderStage* terrainGenRidgedMF;
		extern ShaderStage* terrainGenLibnoise;
	}
}

