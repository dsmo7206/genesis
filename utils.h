#pragma once

#include <string>
#include <vector>
#include <atomic>
#include <stdlib.h>

#include "globals.h"

const float PI = 3.141582653589f;
const float INV_LOG_2 = 1.4426950408889634f;
const float DEG_TO_RAD = PI / 180.0f;
const float RAD_TO_DEG = 1.0f / DEG_TO_RAD;

std::string stringFromFile(const char* filename);

class SpinLock
{
	std::atomic_bool m_lockVariable;

	public:

	SpinLock()            { m_lockVariable = false; }
	inline void acquire() { while (m_lockVariable.exchange(true)) {} }
	inline void release() { m_lockVariable = false; }
};

inline float dist2PointToLineSegment(const glm::vec3& point, const glm::vec3& seg0, const glm::vec3& seg1)
{
	const glm::vec3 v = seg1 - seg0;
	const glm::vec3 w = point - seg0;

	const float c1 = glm::dot(w, v);
	if (c1 <= 0.0)
		return glm::length2(w);

	const float c2 = glm::dot(v, v);
	if (c2 <= c1)
		return glm::length2(point - seg1);

	const float b = c1 / c2;
	const glm::vec3 Pb = seg0 + b * v;
	return glm::length2(point - Pb);
}

inline glm::vec3 posToLatLongAlt(const glm::vec3& pos)
{
	const float altitude = glm::length(pos);

	glm::vec3 result(
		RAD_TO_DEG * asinf(pos.y / altitude), 
		RAD_TO_DEG * atanf(-pos.z / pos.x),
		altitude
	);
	if (result.y < 0.0f)
		result.y += (pos.z < 0.0f ? 1.0f : -1.0f) * 180.0f;

	return result;
}

void buildSphereApproximation(int patchSize, std::vector<glm::vec4>& points, std::vector<GLuint>& indexes);

inline void systemExit(int code)
{
	fprintf(stderr, "Exiting with code %d\n", code);
	exit(code);
}

