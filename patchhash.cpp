#include "patchhash.h"

void printHash(PatchHash p)
{
	const PatchOrientation po = p.getOrientation();

	printf(
		"PatchHash(\n\torientation = %s, level = %d\n\tdim0 = %f, dim1 = %f\n)\n",
		(
			(po == PatchOrientation::X_NEGATIVE) ? "X_NEGATIVE" :
			(po == PatchOrientation::X_POSITIVE) ? "X_POSITIVE" :
			(po == PatchOrientation::Y_NEGATIVE) ? "Y_NEGATIVE" :
			(po == PatchOrientation::Y_POSITIVE) ? "Y_POSITIVE" :
			(po == PatchOrientation::Z_NEGATIVE) ? "Z_NEGATIVE" :
			"Z_POSITIVE"
		),
		p.getLevel(), p.getDim0(), p.getDim1()
	);
}

/*
inline void addToDisplayList(
	DisplayList& dl, PatchHash patchHash, bool checkParent
)
{
	dl.insert(std::make_pair(patchHash.m_value, 0xf));
	if (checkParent)
	{
		ChildPosition childPosition;
		PatchHash parent = patchHash.getParent(childPosition);
		DisplayList::iterator parentIt = dl.find(parent.m_value);
		assert(parentIt != dl.end());
		parentIt->second &= ~childPosition;
	}
}

inline void addUpPatchCircleRow(
	DisplayList& dl, bool checkParent,
	PatchHash center, float maxDistToCenter2, float patchSize,
	MoveDirection rowDir, float leftDist, float rightDist
)
{
	// Add center
	addToDisplayList(dl, center, checkParent);

	// Probe left
	{
		float distToCenter = leftDist;
		PatchHash patch = center;
		MoveDirection columnDir = moveDirectionLeftTurn[rowDir];

		while (distToCenter*distToCenter < maxDistToCenter2)
		{
			patch = patch.move(columnDir);
			addToDisplayList(dl, patch, checkParent);
			distToCenter += patchSize;
		}
	}

	// Probe right
	{
		float distToCenter = rightDist;
		PatchHash patch = center;
		MoveDirection columnDir = moveDirectionRightTurn[rowDir];

		while (distToCenter*distToCenter < maxDistToCenter2)
		{
			patch = patch.move(columnDir);
			addToDisplayList(dl, patch, checkParent);
			distToCenter += patchSize;
		}
	}
}

inline void addDownPatchCircleRow(
	DisplayList& dl, bool checkParent,
	PatchHash center, float maxDistToCenter2, float patchSize,
	MoveDirection rowDir, float leftDist, float rightDist
) // Note: turns are reversed
{
	// Add center
	addToDisplayList(dl, center, checkParent);

	// Probe left
	{
		float distToCenter = leftDist;
		PatchHash patch = center;
		MoveDirection columnDir = moveDirectionRightTurn[rowDir];

		while (distToCenter*distToCenter < maxDistToCenter2)
		{
			patch = patch.move(columnDir);
			addToDisplayList(dl, patch, checkParent);
			distToCenter += patchSize;
		}
	}

	// Probe right
	{
		float distToCenter = rightDist;
		PatchHash patch = center;
		MoveDirection columnDir = moveDirectionLeftTurn[rowDir];

		while (distToCenter*distToCenter < maxDistToCenter2)
		{
			patch = patch.move(columnDir);
			addToDisplayList(dl, patch, checkParent);
			distToCenter += patchSize;
		}
	}
}

void addPatchCircle(
	DisplayList& dl, bool checkParent,
	PatchOrientation po, 
	float dim0, float dim1, int level, float radius2
)
{
	const float patchSize = powf(2.0, 1 - level);
	const PatchHash center = makePatchHash(po, level, dim0, dim1);

	const float leftDist = dim0 - center.getDim0();
	const float rightDist = patchSize - leftDist;
	const float botDist = dim1 - center.getDim1();
	const float topDist = patchSize - botDist;
	
	// Add middle row
	addUpPatchCircleRow(
		dl, checkParent, center, radius2, 
		patchSize, DIM1_POSITIVE, leftDist, rightDist
	);
	
	// Probe up
	{
		float distToRow = topDist;
		PatchHash rowCenter = center;
		MoveDirection rowDirection = DIM1_POSITIVE; // Mutable

		while (distToRow*distToRow < radius2)
		{
			rowCenter = rowCenter.move(rowDirection);
			addUpPatchCircleRow(
				dl, checkParent, rowCenter, 
				radius2-distToRow*distToRow, 
				patchSize, rowDirection, leftDist, rightDist
			);
			distToRow += patchSize;
		}
	}

	// Probe down
	{
		float distToRow = botDist;
		PatchHash rowCenter = center;
		MoveDirection rowDirection = DIM1_NEGATIVE; // Mutable

		while (distToRow*distToRow < radius2)
		{
			rowCenter = rowCenter.move(rowDirection);
			addDownPatchCircleRow(
				dl, checkParent, rowCenter, 
				radius2-distToRow*distToRow, 
				patchSize, rowDirection, leftDist, rightDist
			);
			distToRow += patchSize;
		}
	}
}

void getPatchDisplayList(
	DisplayList& dl,
	PatchOrientation po, float dim0, float dim1,
	int baseLevel, const std::vector<float>& radii2
)
{
	std::vector<float>::const_iterator it = radii2.begin();
	addPatchCircle(dl, false, po, dim0, dim1, baseLevel++, *(it++));

	while (it != radii2.end())
	{
		addPatchCircle(dl, true, po, dim0, dim1, baseLevel++, *(it++));
	}

	
}
*/
