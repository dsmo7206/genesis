#pragma once

#include <algorithm>
#include "patchhash.h"

struct PlanetPatch
{
	const PatchHash m_hash;
	const int m_childNumber;
	PatchBoundingVectors m_boundingVectors;

	GLint m_bufferOffset;
	PlanetPatch* m_parent;
	PlanetPatch* m_children;
	int m_numChildrenPopulated; // bit mask
	bool m_parentPopulated;
	bool m_populated;
	float m_minAltitude;
	float m_maxAltitude;
	float m_averageAltitude;
	unsigned m_numSubmerged;

	PlanetPatch(PatchHash hash, int childNumber, PlanetPatch* parent) :
		m_hash(hash), m_childNumber(childNumber),
		m_boundingVectors(hash.getBoundingVectors()),
		m_parent(parent), m_children(0), m_numChildrenPopulated(0),
		m_populated(false), m_minAltitude(1.0), m_maxAltitude(1.0),
		m_averageAltitude(1.0), m_numSubmerged(0)
	{}

	~PlanetPatch() {}

	inline void setAltitudes(float minAltitude, float maxAltitude)
	{
		m_minAltitude = minAltitude;
		m_maxAltitude = maxAltitude;
		m_averageAltitude = 0.5f * (m_minAltitude + m_maxAltitude);
		const float maxDistFromRadius = std::max(fabs(1.0f - m_minAltitude), fabs(1.0f - m_maxAltitude));
		m_boundingVectors.m_radius = sqrtf(
			2.0f * m_hash.getSize() * m_hash.getSize() +
			maxDistFromRadius * maxDistFromRadius
		);
	}
};

struct PlanetPatchHashFunc : public std::hash_compare<uint64_t>
{
	inline size_t operator()(uint64_t key) const
	{
		key ^= key >> 33;
		key *= 0xff51afd7ed558ccd;
		key ^= key >> 33;
		key *= 0xc4ceb9fe1a85ec53;
		key ^= key >> 33;
		return key & 0xFFFFFFFF;
	}

	inline bool operator()(uint64_t key1, uint64_t key2) const
	{
		return key1 < key2;
	}
};

typedef std::hash_map<uint64_t, PlanetPatch*, PlanetPatchHashFunc> PlanetPatchMap;
