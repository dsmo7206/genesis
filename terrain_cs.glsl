layout (local_size_x=NUM_POINTS, local_size_y=NUM_POINTS, local_size_z=1) in;

struct TerrainVertexData
{
	vec4 positionAndNormal;
	vec4 colour;
};

layout (std140, binding=0) buffer TerrainOutputs 
{
	writeonly TerrainVertexData terrainOutputs[];
};

struct StatsStruct
{
	uint minAlt;
	uint maxAlt;
	uint numSubmerged;
	uint unused1;
};

layout (std140, binding=1) buffer StatsOutputs
{
	writeonly StatsStruct statsOutputs[];
};

shared vec4 sharedPositions[NUM_POINTS*NUM_POINTS];

// orientationMatrixId (3 bits) and offset (29 bits), stepSize (float), dim0Start (float), dim1Start (float)
uniform vec4 uniform_patchDetails[PATCHES_PER_COMPUTE_BATCH]; 
uniform int  uniform_seed;
uniform mat3 uniform_orientationMatrixes[6];

vec3 getVertexPositionSphereSpace()
{	
	vec4 details = uniform_patchDetails[gl_WorkGroupID.x];

	vec3 cubePos = uniform_orientationMatrixes[floatBitsToUint(details[0]) >> 29] * vec3(
		1.0, 
		details[2] + details[1]*gl_LocalInvocationID.x, 
		details[3] + details[1]*gl_LocalInvocationID.y
	);

	// "Better" mapping: see http://mathproofs.blogspot.co.uk/2005/07/mapping-cube-to-sphere.html
	/*
	const vec3 cp2 = cubePos * cubePos;
	return cubePos * sqrt(vec3(1.0) + (cp2.yzx*cp2.zxy)/3.0 - 0.5*(cp2.yzx+cp2.zxy));
	*/

	// Standard mapping
	return normalize(cubePos);
}

bool isExtremity()
{
	return any(bvec4(
		gl_LocalInvocationID.x == 0, gl_LocalInvocationID.x == NUM_POINTS - 1,
		gl_LocalInvocationID.y == 0, gl_LocalInvocationID.y == NUM_POINTS - 1
	));
}

float compressNormal(vec3 normal) // Compress to GL_BGRA format
{
	// Each normal component is in range [-1, 1]; want [0, 1023]
	normal = (normal + vec3(1.0)) * 1023.0 * 0.5;
	return uintBitsToFloat((uint(normal.x) << 20) | (uint(normal.y) << 10) | uint(normal.z));
}

uint floatToSortableUint(float value)
{
	const uint valueAsUint = floatBitsToUint(value);
	return valueAsUint ^ (-int(valueAsUint >> 31) | 0x80000000);
}

void main()
{	
	const int sharedIndex = int(gl_LocalInvocationIndex);
	
	vec3 vertexPositionSphereSpace = getVertexPositionSphereSpace();
	vec4 colourAndAltitude = getColourAndAltitude(vertexPositionSphereSpace, uniform_seed);
	const vec3 outputPosition = vertexPositionSphereSpace * colourAndAltitude.w;
	
	// Set shared memory
	
	sharedPositions[sharedIndex].xyz = outputPosition;
	barrier(); // Synchronise shared memory

	// Calculate normal use differential method from shared memory
	vec3 xDn = sharedPositions[max(sharedIndex-1, 0)].xyz;
	vec3 xUp = sharedPositions[min(sharedIndex+1, NUM_POINTS*NUM_POINTS-1)].xyz;
	vec3 yDn = sharedPositions[max(sharedIndex-NUM_POINTS, 0)].xyz;
	vec3 yUp = sharedPositions[min(sharedIndex+NUM_POINTS, NUM_POINTS*NUM_POINTS-1)].xyz;
	const vec3 normal = normalize(cross(xUp - xDn, yUp - yDn));

	// Set values
	if (!isExtremity())
	{
		// Patch offset is lowest 21 bits (0-20)
		const uint patchOffset = floatBitsToUint(uniform_patchDetails[gl_WorkGroupID.x][0]) & 0x1fffff;
		const uint numOutPoints = NUM_POINTS - 2;
		const uint terrainVertexIndex = 
			patchOffset*numOutPoints*numOutPoints +
			(gl_LocalInvocationID.y-1)*numOutPoints + 
			gl_LocalInvocationID.x - 1
		;

		// Set vertex
		terrainOutputs[terrainVertexIndex].positionAndNormal = vec4(outputPosition, compressNormal(normal));
		terrainOutputs[terrainVertexIndex].colour = vec4(colourAndAltitude.xyz, 1.0);
		
		// Set stats; offset is next 8 bits (21-28)
		/*
		const uint statsOffset = (floatBitsToUint(uniform_patchDetails[gl_WorkGroupID.x][0]) >> 21) & 0xff;
		
		uint altitudeAsUint = floatToSortableUint(colourAndAltitude.w); 
		atomicMin(statsOutputs[statsOffset].minAlt, altitudeAsUint);
		atomicMax(statsOutputs[statsOffset].maxAlt, altitudeAsUint);
		if (colourAndAltitude.w < 1.0)
			atomicAdd(statsOutputs[statsOffset].numSubmerged, 1);
			*/
	}
}
