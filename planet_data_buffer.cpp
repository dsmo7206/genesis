#include <chrono>
#include <atomic>

#include "planet_data_buffer.h"
#include "overlay.h"
#include "globals.h"
#include "planet.h"
#include "utils.h"

static std::vector<GLuint> makeAllIndexes(GLuint visiblePolygons, GLuint verticesPerSide)
{
	std::vector<GLuint> indexes;

	for (unsigned yIndex = 0; yIndex < visiblePolygons; ++yIndex)
	{
		for (unsigned xIndex = 0; xIndex < visiblePolygons; ++xIndex)
		{
			const GLuint startIndex = yIndex*verticesPerSide + xIndex;
			indexes.push_back(startIndex);
			indexes.push_back(startIndex + 1);
			indexes.push_back(startIndex + verticesPerSide);
			indexes.push_back(startIndex + verticesPerSide);
			indexes.push_back(startIndex + 1);
			indexes.push_back(startIndex + verticesPerSide + 1);
		}
	}
	
	return indexes;
}

static unsigned floatToSortableUint(float value)
{
	const unsigned valueAsUint = *reinterpret_cast<unsigned*>(&value);
	return valueAsUint ^ (-int(valueAsUint >> 31) | 0x80000000);
}

void* getStatsZeroData(unsigned bufferSizePatches)
{
	const unsigned highestFloatAsUint = floatToSortableUint(std::numeric_limits<float>::max());
	const unsigned lowestFloatAsUint = floatToSortableUint(std::numeric_limits<float>::lowest());

	glm::uvec4* statsZeroData = new glm::uvec4[bufferSizePatches];
	for (unsigned i = 0; i < bufferSizePatches; ++i)
		((glm::uvec4*)statsZeroData)[i] = glm::uvec4(highestFloatAsUint, lowestFloatAsUint, 0, 0);

	return (void*)statsZeroData;
}

PlanetPatchConstants::PlanetPatchConstants(unsigned visiblePolygons, unsigned patchesPerBatch) :
	m_visiblePolygons(visiblePolygons),
	m_verticesPerSide(m_visiblePolygons + 1),
	m_visibleVertices(m_verticesPerSide * m_verticesPerSide),
	m_totalVertices(m_verticesPerSide * m_verticesPerSide),
	m_totalSizeBytes(m_totalVertices * sizeof(PatchVertexData)),
	m_patchesPerBatch(patchesPerBatch),
	m_allIndexes(makeAllIndexes(m_visiblePolygons, m_verticesPerSide))
{
}

void TW_CALL antGetBufferSizeMB(void* value, void* clientData) 
{ 
	*(float*)value = PLANET_DATA_BUFFER->m_bufferSizeBytes / 1048576.0f; 
}

void TW_CALL antGetGPUPatches(void* value, void* clientData) 
{ 
	*(unsigned*)value = PLANET_DATA_BUFFER->numAllocatedPatches(); 
}

void TW_CALL antGetPercentFull(void* value, void* clientData) 
{ 
	*(float*)value = 100.0f * PLANET_DATA_BUFFER->numAllocatedPatches() / PLANET_DATA_BUFFER->m_bufferSizePatches; 
}

PlanetDataBuffer::PlanetDataBuffer(GLuint bufferSizeBytes, unsigned statsBufferSizePatches) :
	m_bufferSizeBytes(bufferSizeBytes),
	m_bufferSizePatches(bufferSizeBytes / PLANET_PATCH_CONSTANTS->m_totalSizeBytes),
	m_patchPointers(new PlanetPatch*[m_bufferSizePatches]),
	m_ownerPointers(new Planet*[m_bufferSizePatches]),
	m_lastDrawnTimes(new double[m_bufferSizePatches]),
	m_statsBufferSizePatches(statsBufferSizePatches),
	m_statsBufferSizeBytes(m_statsBufferSizePatches * sizeof(glm::uvec4)),
	m_statsDataClientBuffer(new glm::uvec4[m_statsBufferSizePatches]),
	m_statsZeroData(getStatsZeroData(m_statsBufferSizePatches))
{
	m_bufferLock.acquire();

	// Create vertex storage
	glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer.m_id);
	glBufferData(GL_ARRAY_BUFFER, m_bufferSizeBytes, 0, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Create stats storage
	glBindBuffer(GL_ARRAY_BUFFER, m_statsBuffer.m_id);
	glBufferData(GL_ARRAY_BUFFER, PLANET_PATCH_CONSTANTS->m_patchesPerBatch * sizeof(glm::uvec4), 0, GL_STREAM_READ);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Create index storage
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer.m_id);
	glBufferData(
		GL_ELEMENT_ARRAY_BUFFER, 
		PLANET_PATCH_CONSTANTS->m_allIndexes.size()*sizeof(GLuint), 
		&PLANET_PATCH_CONSTANTS->m_allIndexes[0], 
		GL_STATIC_DRAW
	);

	// Create offset stack (starts at the highest)
	std::vector<GLint> v;
	for (int i = 0; i < (int)m_bufferSizePatches; ++i)
		v.push_back(i);
	for (std::vector<GLint>::const_reverse_iterator it = v.rbegin(); it != v.rend(); ++it)
		m_offsetStack.push(*it);

	// Create cleanup arrays
	for (int i = 0; i < (int)m_bufferSizePatches; ++i)
	{
		m_patchPointers[i] = nullptr;
		m_ownerPointers[i] = nullptr;
		m_lastDrawnTimes[i] = std::numeric_limits<double>::max();
	}

	// Make overlay bar (for constants too)
	TwAddVarRO(GLOBALS.m_overlay_bar, "Visible Polygons", TW_TYPE_UINT32, &PLANET_PATCH_CONSTANTS->m_visiblePolygons, " group=PatchConstants ");
	TwAddVarRO(GLOBALS.m_overlay_bar, "Total Verts", TW_TYPE_UINT32, &PLANET_PATCH_CONSTANTS->m_totalVertices, " group=PatchConstants ");
	TwAddVarRO(GLOBALS.m_overlay_bar, "Byte Size", TW_TYPE_UINT32, &PLANET_PATCH_CONSTANTS->m_totalSizeBytes, " group=PatchConstants ");
	TwAddVarCB(GLOBALS.m_overlay_bar, "Total MB Size", TW_TYPE_FLOAT, 0, antGetBufferSizeMB, 0, " group=PlanetBuffer ");
	TwAddVarRO(GLOBALS.m_overlay_bar, "Patch Capacity", TW_TYPE_UINT32, &m_bufferSizePatches, " group=PlanetBuffer ");
	TwAddVarCB(GLOBALS.m_overlay_bar, "Curr Num Patches", TW_TYPE_UINT32, 0, antGetGPUPatches, 0, " group=PlanetBuffer ");
	TwAddVarCB(GLOBALS.m_overlay_bar, "% Full", TW_TYPE_FLOAT, 0, antGetPercentFull, 0, " group=PlanetBuffer ");

	// Make cleanup thread
	m_cleanupThread = new std::thread(cleanupPatches);

	m_bufferLock.release();
}

PlanetDataBuffer::~PlanetDataBuffer()
{
	delete[] m_patchPointers;
	delete[] m_ownerPointers;
	delete[] m_lastDrawnTimes;
	delete[] m_statsDataClientBuffer;
}

void cleanupPatchesSingleFrame()
{
	PlanetPatch** const patches = PLANET_DATA_BUFFER->m_patchPointers;
	Planet** const owners = PLANET_DATA_BUFFER->m_ownerPointers;
	double* const times = PLANET_DATA_BUFFER->m_lastDrawnTimes;

	const double oldTime = glfwGetTime() - 5.0; // Num seconds - should make this dynamic

	PLANET_DATA_BUFFER->m_bufferLock.acquire();

	for (int i = 0; i < (int)PLANET_DATA_BUFFER->m_bufferSizePatches; ++i)
	{
		PlanetPatch* const patch = patches[i];

		if (patch && (times[i] < oldTime) && !patch->m_children && !patches[i]->m_numChildrenPopulated)
		{
			owners[i]->notifyPatchDelete(patch);
			patch->m_populated = false;
			if (patch->m_parent)
				patch->m_parent->m_numChildrenPopulated &= ~(1 << patch->m_childNumber);
			PLANET_DATA_BUFFER->freeOffset(patch->m_bufferOffset);
			patches[i] = nullptr; // To avoid repeated deletes
		}
	}

	PLANET_DATA_BUFFER->m_bufferLock.release();
}

void cleanupPatches()
{
	while (!GLOBALS.m_shuttingDown)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		cleanupPatchesSingleFrame(); 
	}
}

const PlanetPatchConstants* PLANET_PATCH_CONSTANTS;
PlanetDataBuffer* PLANET_DATA_BUFFER;

void initPlanetDataBufferAndConstants()
{
	PLANET_PATCH_CONSTANTS = new PlanetPatchConstants(32, 1);
	PLANET_DATA_BUFFER = new PlanetDataBuffer(268435456, 256); // 256 MB
}
