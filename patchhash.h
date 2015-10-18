#pragma once

#include <cassert>
#include <cstdint>
#include <hash_map>
#include <utility>

#include "glstuff.h"

enum class PatchOrientation { X_NEGATIVE, X_POSITIVE, Y_NEGATIVE, Y_POSITIVE, Z_NEGATIVE, Z_POSITIVE };
enum class ChildPosition { DIM0LO_DIM1LO = 1, DIM0HI_DIM1LO = 2, DIM0LO_DIM1HI = 4, DIM0HI_DIM1HI = 8 };
enum class MoveDirection { DIM0_NEGATIVE = 0, DIM0_POSITIVE = 1, DIM1_NEGATIVE = 2, DIM1_POSITIVE = 3 };

const uint64_t ORIENTATION_BIT_MASK = 0xe000000000000000;
const uint64_t LEVEL_BIT_MASK       = 0x1f00000000000000;
const uint64_t DIM0_BIT_MASK        = 0x00fffffff0000000;
const uint64_t DIM1_BIT_MASK        = 0x000000000fffffff;

const int ORIENTATION_SHIFT = 61;
const int LEVEL_SHIFT       = 56;
const int DIM0_SHIFT        = 28;
const int DIM1_SHIFT        =  0;

const uint64_t DIM_MAX_VALUE = 0x10000000;
const float SQRT_2 = 1.4142135623730951f;

inline uint64_t patchOrientationToBits(PatchOrientation patchOrientation)
{
	assert(static_cast<uint64_t>(patchOrientation) < 6);
	return static_cast<uint64_t>(patchOrientation) << ORIENTATION_SHIFT;
}

inline uint64_t levelToBits(int level)
{
	assert(static_cast<uint32_t>(level) < 28);
	return static_cast<uint64_t>(level) << LEVEL_SHIFT;
}

inline uint64_t dimLevelMask(int level)
{
	return ~((1 << (28 - level)) - 1);
}

inline uint64_t dim0ToBits(float dim0, int level)
{
	assert(dim0 >= -1.0f && dim0 < 1.0f);
	return (static_cast<uint64_t>((dim0 + 1.0f)*(1 << 27)) & dimLevelMask(level)) << DIM0_SHIFT;
}

inline uint64_t dim1ToBits(float dim1, int level)
{
	assert(dim1 >= -1.0f && dim1 < 1.0f);
	return (static_cast<uint64_t>((dim1 + 1.0f)*(1 << 27)) & dimLevelMask(level)) << DIM1_SHIFT;
}

inline uint64_t makePatchHash(
	PatchOrientation patchOrientation, int level, float dim0, float dim1
)
{
	return 
		patchOrientationToBits(patchOrientation) | 
		levelToBits(level) |
		dim0ToBits(dim0, level) | 
		dim1ToBits(dim1, level)
	;
}

inline float shiftedBitsToDim(uint64_t dimBits)
{
	return (float(dimBits) / (1 << 27)) - 1.0f;
}

inline glm::vec3 dimsToUnnormalisedVec3(PatchOrientation patchOrientation, float dim0, float dim1)
{
	switch (patchOrientation)
	{
		case PatchOrientation::X_NEGATIVE:
			return glm::vec3(-1.0, dim1, dim0);
		case PatchOrientation::X_POSITIVE:
			return glm::vec3(1.0, dim1, -dim0);
		case PatchOrientation::Y_NEGATIVE:
			return glm::vec3(dim0, -1.0, dim1);
		case PatchOrientation::Y_POSITIVE:
			return glm::vec3(dim0, 1.0, -dim1);
		case PatchOrientation::Z_NEGATIVE:
			return glm::vec3(-dim0, dim1, -1.0);
		case PatchOrientation::Z_POSITIVE:
			return glm::vec3(dim0, dim1, 1.0);
		default:
			throw std::exception("Unknown patchOrientation");
	}
}

inline glm::vec3 dimsToVec3(PatchOrientation patchOrientation, float dim0, float dim1)
{
	return glm::normalize(dimsToUnnormalisedVec3(patchOrientation, dim0, dim1));
}

const uint64_t X_NEGATIVE_ROOT = (uint64_t)PatchOrientation::X_NEGATIVE << ORIENTATION_SHIFT;
const uint64_t X_POSITIVE_ROOT = (uint64_t)PatchOrientation::X_POSITIVE << ORIENTATION_SHIFT;
const uint64_t Y_NEGATIVE_ROOT = (uint64_t)PatchOrientation::Y_NEGATIVE << ORIENTATION_SHIFT;
const uint64_t Y_POSITIVE_ROOT = (uint64_t)PatchOrientation::Y_POSITIVE << ORIENTATION_SHIFT;
const uint64_t Z_NEGATIVE_ROOT = (uint64_t)PatchOrientation::Z_NEGATIVE << ORIENTATION_SHIFT;
const uint64_t Z_POSITIVE_ROOT = (uint64_t)PatchOrientation::Z_POSITIVE << ORIENTATION_SHIFT;

struct PatchBoundingVectors
{ // All vec3s are on the unit sphere
	const glm::vec3 m_center;
	const glm::vec3 m_corner00;
	const glm::vec3 m_corner01;
	const glm::vec3 m_corner10;
	const glm::vec3 m_corner11;
	float m_radius;

	PatchBoundingVectors(
		const glm::vec3& center, 
		const glm::vec3& corner00, const glm::vec3& corner01,
		const glm::vec3& corner10, const glm::vec3& corner11,
		const float radius
	) : 
		m_center(center), 
		m_corner00(corner00), m_corner01(corner01), 
		m_corner10(corner10), m_corner11(corner11),
		m_radius(radius)
	{}
};

struct PatchHash
{
	// (3) bits 63 - 61: Patch orientation (uint) (from enum)
	// (5) bits 60 - 56: Patch detail (uint)
	// (28) bits 55 - 28: Fractional dim0 position
	// (28) bits 27 - 0: Fractional dim1 position
	uint64_t m_value;

	PatchHash(uint64_t value) : m_value(value) {}

	inline uint64_t getOrientationBits() const { return m_value & ORIENTATION_BIT_MASK; }
	inline uint64_t getLevelBits() const       { return m_value & LEVEL_BIT_MASK; }
	inline uint64_t getDim0Bits() const        { return m_value & DIM0_BIT_MASK; }
	inline uint64_t getDim1Bits() const        { return m_value & DIM1_BIT_MASK; }
	
	inline PatchOrientation getOrientation() const 
	{ 
		return PatchOrientation(getOrientationBits() >> ORIENTATION_SHIFT); 
	}
	inline int getLevel() const  { return getLevelBits() >> LEVEL_SHIFT; }
	inline float getSize() const { return 2.0f / (1 << getLevel()); }
	inline float getDim0() const { return shiftedBitsToDim(getDim0Bits() >> DIM0_SHIFT); }
	inline float getDim1() const { return shiftedBitsToDim(getDim1Bits() >> DIM1_SHIFT); }

	PatchBoundingVectors getBoundingVectors() const
	{
		const PatchOrientation po = getOrientation();
		const float dim0 = getDim0();
		const float dim1 = getDim1();
		const float size = getSize();

		return PatchBoundingVectors(
			dimsToVec3(po, dim0 + size / 2.0f, dim1 + size / 2.0f),
			dimsToVec3(po, dim0, dim1),
			dimsToVec3(po, dim0, dim1 + size),
			dimsToVec3(po, dim0 + size, dim1),
			dimsToVec3(po, dim0 + size, dim1 + size),
			SQRT_2 * size
		);
	}

	inline PatchHash getParent(ChildPosition& childPosition) const
	{
		const int level = getLevel();
		if (level == 0)
			assert(0);

		const uint64_t levelMask = dimLevelMask(level - 1);

		const uint64_t dim0Bit = (m_value & (1LL << (56 - level)));
		const uint64_t dim1Bit = (m_value & (1LL << (28 - level)));

		childPosition = 
			( dim0Bit &&  dim1Bit) ? ChildPosition::DIM0HI_DIM1HI :
			( dim0Bit && ~dim1Bit) ? ChildPosition::DIM0HI_DIM1LO :
			(~dim0Bit &&  dim1Bit) ? ChildPosition::DIM0LO_DIM1HI :
			ChildPosition::DIM0LO_DIM1LO;

		return PatchHash(
			getOrientationBits() |
			levelToBits(level - 1) |
			(getDim0Bits() & (levelMask << DIM0_SHIFT)) |
			(getDim1Bits() & (levelMask << DIM1_SHIFT))
		);
	}
};

void printHash(PatchHash p);
