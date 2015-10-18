#pragma once

#include <initializer_list>
#include "glstuff.h"
#include "shader_program.h"

class UniformBlock
{
protected:

	template <typename T> void copyArray(void* dest, int offset, T* value, int count)
	{
		memcpy(static_cast<char*>(dest)+offset, (void*)value, count*sizeof(T));
	}

	template <typename T> void copyScalar(void* dest, int offset, T* value)
	{
		copyArray(dest, offset, value, 1);
	}

public:

	const GLuint m_bufferId;
	const GLenum m_usageHint;
	const std::vector<std::string> m_names;
	GLint* m_offsets;

	UniformBlock(
		GLenum usageHint,
		const std::initializer_list<std::string>& names
	);
	~UniformBlock();

	void setOffsetsFromProgram(const ShaderProgram& program, const char* blockName);
};
