#include "utils.h"

#include <iostream>
#include <fstream>

std::string stringFromFile(const char* filename)
{
	std::ifstream in(filename, std::ios::in | std::ios::binary);

	if (!in)
		throw std::exception((std::string("Problem reading file: ") + filename).c_str());

	std::string contents;
	in.seekg(0, std::ios::end);
	contents.resize(in.tellg());
	in.seekg(0, std::ios::beg);
	in.read(&contents[0], contents.size());
	in.close();
	return contents;
}

void buildSphereApproximation(int patchSize, std::vector<glm::vec4>& points, std::vector<GLuint>& indexes)
{
	points.clear();
	indexes.clear();
	const float stepSize = 2.0f / patchSize;

	float dim1 = -1.0;
	for (int dim1Index = 0; dim1Index <= patchSize; ++dim1Index)
	{
		float dim0 = -1.0;
		for (int dim0Index = 0; dim0Index <= patchSize; ++dim0Index)
		{
			points.push_back(glm::vec4(glm::normalize(glm::vec3(-1.0, dim1, dim0)), 1.0)); // X_NEGATIVE
			points.push_back(glm::vec4(glm::normalize(glm::vec3(1.0, dim1, -dim0)), 1.0)); // X_POSITIVE
			points.push_back(glm::vec4(glm::normalize(glm::vec3(dim0, -1.0, dim1)), 1.0)); // Y_NEGATIVE
			points.push_back(glm::vec4(glm::normalize(glm::vec3(dim0, 1.0, -dim1)), 1.0)); // Y_POSITIVE
			points.push_back(glm::vec4(glm::normalize(glm::vec3(-dim0, dim1, -1.0)), 1.0)); // Z_NEGATIVE
			points.push_back(glm::vec4(glm::normalize(glm::vec3(dim0, dim1, 1.0)), 1.0)); // Z_POSITIVE
			dim0 += stepSize;
		}

		dim1 += stepSize;
	}

	// Now build indexes. Need to multiply each vertex by 6 and add offset for side
	// because vertices created above are interleaved.
	const int dX = 6; // How many indices to move up for one dim0
	const int dY = (patchSize + 1) * 6;

	for (int yIndex = 0; yIndex < patchSize; ++yIndex)
	{
		for (int xIndex = 0; xIndex < patchSize; ++xIndex)
		{
			for (int sideNumber = 0; sideNumber < 6; ++sideNumber)
			{
				const GLuint startIndex = yIndex*dY + xIndex*dX + sideNumber;
				indexes.push_back(startIndex);
				indexes.push_back(startIndex + dX);
				indexes.push_back(startIndex + dY);
				indexes.push_back(startIndex + dY);
				indexes.push_back(startIndex + dX);
				indexes.push_back(startIndex + dX + dY);
			}
		}
	}
}
