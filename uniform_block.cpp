#include "uniform_block.h"

GLuint makeUniformBuffer()
{
	GLuint id;
	glGenBuffers(1, &id);
	glBindBuffer(GL_UNIFORM_BUFFER, id);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	return id;
}

UniformBlock::UniformBlock(
	GLenum usageHint,
	const std::initializer_list<std::string>& names
) :
	m_bufferId(makeUniformBuffer()),
	m_usageHint(usageHint),
	m_names(names),
	m_offsets(new GLint[names.size()])
{
}

UniformBlock::~UniformBlock()
{
	delete[] m_offsets;
}

void UniformBlock::setOffsetsFromProgram(const ShaderProgram& program, const char* blockName)
{
	// Allocate memory for client block
	const size_t blockSize = program.getUniformBlockSize(blockName);

	// Allocate memory for server block
	glBindBuffer(GL_UNIFORM_BUFFER, m_bufferId);
	glBufferData(GL_UNIFORM_BUFFER, blockSize, 0, m_usageHint);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	// Populate offsets
	const GLchar** names = new const GLchar*[m_names.size()];
	for (size_t i = 0; i < m_names.size(); ++i)
		names[i] = m_names[i].c_str();

	program.getUniformOffsets(m_names.size(), names, m_offsets);

	delete[] names;
}
