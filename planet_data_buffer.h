#pragma once

#include <stack>
#include <vector>
#include <thread>

#include "glstuff.h"
#include "utils.h"

struct PlanetPatch;
class Planet;

struct PatchVertexData
{
	glm::vec4 positionAndNormal;
	glm::vec4 colour;
};

struct PlanetPatchConstants
{
	const unsigned m_visiblePolygons;
	const unsigned m_verticesPerSide;
	const unsigned m_visibleVertices;
	const unsigned m_totalVertices;
	const unsigned m_totalSizeBytes;
	const unsigned m_patchesPerBatch;
	const std::vector<GLuint> m_allIndexes;

	private:

	PlanetPatchConstants(unsigned visiblePolygons, unsigned patchesPerBatch);
	friend void initPlanetDataBufferAndConstants();
};
extern const PlanetPatchConstants* PLANET_PATCH_CONSTANTS;

struct PlanetDataBuffer
{
	const GLuint m_bufferSizeBytes;
	const GLuint m_bufferSizePatches;

	const VertexBuffer m_vertexBuffer;
	const VertexBuffer m_statsBuffer;
	const VertexBuffer m_indexBuffer;

	const unsigned m_statsBufferSizePatches;
	const unsigned m_statsBufferSizeBytes;
	glm::uvec4* const m_statsDataClientBuffer;
	void* const m_statsZeroData;

	SpinLock m_bufferLock;

	// Each of these is m_bufferSizePatches long
	PlanetPatch** const m_patchPointers;
	Planet** const m_ownerPointers;
	double* const m_lastDrawnTimes;

	inline void clearStatsBuffer()
	{
		glBufferData(GL_ARRAY_BUFFER, m_statsBufferSizeBytes, m_statsZeroData, GL_DYNAMIC_COPY);
	}

	inline void downloadStatsBuffer()
	{
		glGetBufferSubData(GL_ARRAY_BUFFER, 0, m_statsBufferSizeBytes, m_statsDataClientBuffer);
	}

	inline GLint getOffset(PlanetPatch* patch, Planet* owner)
	{
		const GLint offset = m_offsetStack.top();
		m_offsetStack.pop();
		m_patchPointers[offset] = patch;
		m_ownerPointers[offset] = owner;
		return offset;
	}

	inline void freeOffset(GLint offset)
	{
		m_patchPointers[offset] = 0;
		m_offsetStack.push(offset);
	}

	inline unsigned numAllocatedPatches() const
	{
		return m_bufferSizePatches - (unsigned)m_offsetStack.size();
	}

	private:

	PlanetDataBuffer(GLuint bufferSizeBytes, unsigned statsBufferSizePatches);
	~PlanetDataBuffer();

	std::thread* m_cleanupThread;
	std::stack<GLint> m_offsetStack;

	friend void initPlanetDataBufferAndConstants();
};
extern PlanetDataBuffer* PLANET_DATA_BUFFER;

// Initialise everything
void initPlanetDataBufferAndConstants();

// Function for cleanup thread
void cleanupPatches();
