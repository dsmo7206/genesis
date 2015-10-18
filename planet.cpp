#include "planet.h"
#include "globals.h"
#include "scene.h"
#include "camera.h"
#include "utils.h"
#include "planet_data_buffer.h"
#include "lightsource.h"

static inline int fastIntMaxZero(int x)
{
	return x - ((x >> 31) & x);
}

static inline int fastCeil(float x)
{
	return 32768 - (int)(32768.0 - x);
}

static inline float fastLog2(float val)
{
	int* const exp_ptr = reinterpret_cast<int *>(&val);
	int x = *exp_ptr;
	const int log_2 = ((x >> 23) & 255) - 128;
	x &= ~(255 << 23);
	x += 127 << 23;
	*exp_ptr = x;

	val = ((-1.0f / 3) * val + 2) * val - 2.0f / 3; //(1)

	return (val + log_2);
}

static inline void cameraPositionToPatchPosition(
	const glm::vec3& cameraPosition,
	PatchOrientation* po,
	glm::vec3& patchPosition
)
{
	// patchPosition is (dim0, dim1, altitude)

	patchPosition.z = glm::length(cameraPosition);
	const glm::vec3 acp = glm::abs(cameraPosition);

	if (acp.x >= acp.y && acp.x >= acp.z)
	{
		const glm::vec3 positionOnCube = cameraPosition * (1.0f / acp.x);

		if (cameraPosition.x < 0.0)
		{
			*po = PatchOrientation::X_NEGATIVE; 
			patchPosition.x = positionOnCube.z; patchPosition.y = positionOnCube.y;
		}
		else
		{
			*po = PatchOrientation::X_POSITIVE; 
			patchPosition.x = -positionOnCube.z; patchPosition.y  = positionOnCube.y;
		}
	}
	else if (acp.y >= acp.z && acp.y >= acp.x)
	{
		const glm::vec3 positionOnCube = cameraPosition * (1.0f / acp.y);

		if (cameraPosition.y < 0.0)
		{
			*po = PatchOrientation::Y_NEGATIVE; 
			patchPosition.x = positionOnCube.x; patchPosition.y = -positionOnCube.z;
		}
		else
		{
			*po = PatchOrientation::Y_POSITIVE; 
			patchPosition.x = positionOnCube.x; patchPosition.y = positionOnCube.z;
		}
	}
	else
	{
		const glm::vec3 positionOnCube = cameraPosition * (1.0f / acp.z);

		if (cameraPosition.z < 0.0)
		{
			*po = PatchOrientation::Z_NEGATIVE; 
			patchPosition.x = -positionOnCube.x; patchPosition.y = positionOnCube.y;
		}
		else
		{
			*po = PatchOrientation::Z_POSITIVE; 
			patchPosition.x = positionOnCube.x; patchPosition.y = positionOnCube.y;
		}
	}
}

static std::vector<PlanetPatch*> makeRootPatches()
{
	return std::vector<PlanetPatch*>({
		new PlanetPatch(X_NEGATIVE_ROOT, 0, nullptr), new PlanetPatch(X_POSITIVE_ROOT, 0, nullptr),
		new PlanetPatch(Y_NEGATIVE_ROOT, 0, nullptr), new PlanetPatch(Y_POSITIVE_ROOT, 0, nullptr),
		new PlanetPatch(Z_NEGATIVE_ROOT, 0, nullptr), new PlanetPatch(Z_POSITIVE_ROOT, 0, nullptr)
	});
}

static AtmosphereConstants* addRadiusToConstants(AtmosphereConstants* ac, float planetRadius)
{
	if (ac)
	{
		ac->m_innerRadius = planetRadius;
		ac->m_outerRadius = planetRadius * 1.025f;
	}
	return ac;
}

#include "planet_overlay_macros.inl"

Planet::Planet(
	const std::string& name, 
	float radius, TerrainGenerator* terrainGenerator, 
	AtmosphereConstants* atmosphereConstants,
	Water* water, Position* position
) :
	Shape(name, position),
	m_radius(radius),
	m_maxDistSurfaceToSky(atmosphereConstants ?
		sqrtf(
			atmosphereConstants->m_outerRadius * atmosphereConstants->m_outerRadius -
			m_radius * m_radius
		) : 
		0.0f
	),
	m_atmosphereConstants(atmosphereConstants),
	m_overlay_bar(TwNewBar(std::string("Planet - " + m_name).c_str())),
	m_rootPatches(makeRootPatches()),
	m_terrainGenerator(terrainGenerator),
	m_terrainInAtmProgram(
		new TerrainDrawProgram(
			atmosphereConstants ?
			new ShaderProgram({ ShaderStages::Vertex::terrainInAtm, ShaderStages::Fragment::terrainAtm }) :
			new ShaderProgram({ ShaderStages::Vertex::terrainNoAtm, ShaderStages::Fragment::terrainNoAtm })
		)
	),
	m_terrainOutAtmProgram(
		new TerrainDrawProgram(
			atmosphereConstants ?
			new ShaderProgram({ ShaderStages::Vertex::terrainOutAtm, ShaderStages::Fragment::terrainAtm }) :
			new ShaderProgram({ ShaderStages::Vertex::terrainNoAtm, ShaderStages::Fragment::terrainNoAtm })
		)
	),
	m_skyInAtmProgram(
		atmosphereConstants ?
		new SkyDrawProgram(
			new ShaderProgram({ ShaderStages::Vertex::skyInAtm, ShaderStages::Fragment::sky })
		) :
		0
	),
	m_skyOutAtmProgram(
		atmosphereConstants ?
		new SkyDrawProgram(
			new ShaderProgram({ ShaderStages::Vertex::skyOutAtm, ShaderStages::Fragment::sky })
		) :
		0
	),
	m_water(water)
{
	// Set up overlay
	TwSetParam(m_overlay_bar, NULL, "refresh", TW_PARAM_CSTRING, 1, "0.1");
	TwAddVarRO(m_overlay_bar, "Num CPU Patches", TW_TYPE_INT32, &m_overlay_numPatches, " group=Statistics ");
	TwAddVarRO(m_overlay_bar, "Queue Size", TW_TYPE_INT32, &m_overlay_queueSize, " group=Statistics ");
	TwAddVarRO(m_overlay_bar, "T Patches Drawn", TW_TYPE_INT32, &m_overlay_terrainPatchesDrawn, " group=Statistics ");
	TwAddVarRO(m_overlay_bar, "W Patches Drawn", TW_TYPE_INT32, &m_overlay_waterPatchesDrawn, " group=Statistics ");
	TwAddVarRO(m_overlay_bar, "Lowest Patch", TW_TYPE_INT32, &m_overlay_lowestPatchLevel, " group=Statistics ");
	TwAddVarRO(m_overlay_bar, "Highest Patch", TW_TYPE_INT32, &m_overlay_highestPatchLevel, " group=Statistics ");
	TwAddVarRO(m_overlay_bar, "Patches Traversed", TW_TYPE_INT32, &m_overlay_patchesTraversed, " group=Statistics ");
	TwAddVarRO(m_overlay_bar, "Patches Discarded", TW_TYPE_INT32, &m_overlay_patchesDiscarded, " group=Statistics ");
	TwAddVarRO(m_overlay_bar, "Altitude", TW_TYPE_FLOAT, &m_overlay_altitude, " group=Statistics ");
	TwAddVarRO(m_overlay_bar, "Ground Altitude", TW_TYPE_FLOAT, &m_overlay_groundAltitude, " group=Statistics ");
	m_terrainGenerator->addToOverlay(m_overlay_bar);
	
	// Add root patches to patchmap
	for (int i = 0; i < m_rootPatches.size(); ++i)
		m_patchMap.emplace(m_rootPatches[i]->m_hash.m_value, m_rootPatches[i]);

	// Set up terrain vertex array
	{
		glBindVertexArray(m_terrainDrawVertexArray.m_id);
		glBindBuffer(GL_ARRAY_BUFFER, PLANET_DATA_BUFFER->m_vertexBuffer.m_id);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(PatchVertexData), (void*)offsetof(PatchVertexData, positionAndNormal));
		glVertexAttribPointer(1, GL_BGRA, GL_UNSIGNED_INT_2_10_10_10_REV, GL_TRUE, sizeof(PatchVertexData), (void*)offsetof(PatchVertexData, positionAndNormal.w));
		glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(PatchVertexData), (void*)offsetof(PatchVertexData, colour));
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, PLANET_DATA_BUFFER->m_indexBuffer.m_id);
		glBindBuffer(GL_UNIFORM_BUFFER, m_uniformBuffer.m_id);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(PlanetUniforms), nullptr, GL_DYNAMIC_DRAW); // Allocate space here (because we bind it first)
		glBindBufferBase(GL_UNIFORM_BUFFER, PLANET_UNIFORMS_BINDING_POINT, m_uniformBuffer.m_id);
		glBindVertexArray(0);
	}

	// Set up water vertex array
	if (m_water)
	{
		glBindVertexArray(m_waterDrawVertexArray.m_id);
		glBindBuffer(GL_ARRAY_BUFFER, PLANET_DATA_BUFFER->m_vertexBuffer.m_id);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(PatchVertexData), (void*)offsetof(PatchVertexData, positionAndNormal));
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, PLANET_DATA_BUFFER->m_indexBuffer.m_id);
		glBindBufferBase(GL_UNIFORM_BUFFER, PLANET_UNIFORMS_BINDING_POINT, m_uniformBuffer.m_id);
		glBindVertexArray(0);

		m_water->addToOverlayBar(m_overlay_bar);
	}
	
	if (atmosphereConstants)
	{
		// Setup overlay
		TwAddVarCB(m_overlay_bar, "Wavelength", TW_TYPE_COLOR3F, antSetWaveLength, antGetWaveLength, this, " group=Atmosphere ");
		TwAddVarCB(m_overlay_bar, "Kr", TW_TYPE_FLOAT, antSetKr, antGetKr, this, " step=0.0001 group=Atmosphere ");
		TwAddVarCB(m_overlay_bar, "Km", TW_TYPE_FLOAT, antSetKm, antGetKm, this, " step=0.0001 group=Atmosphere ");
		TwAddVarCB(m_overlay_bar, "ESun", TW_TYPE_FLOAT, antSetESun, antGetESun, this, " group=Atmosphere ");
		TwAddVarCB(m_overlay_bar, "G", TW_TYPE_FLOAT, antSetG, antGetG, this, " min=-1 max=1 step=0.0001 group=Atmosphere ");
		TwAddVarCB(m_overlay_bar, "Scale Depth", TW_TYPE_FLOAT, antSetScaleDepth, antGetScaleDepth, this, " min=0 max=1 step=0.01 group=Atmosphere ");
		TwAddVarCB(m_overlay_bar, "Samples", TW_TYPE_INT32, antSetSamples, antGetSamples, this, " min=0 group=Atmosphere ");

		// Set initial values
		antSetWaveLength(&m_atmosphereConstants->m_waveLength, this);
		antSetInnerRadius(&m_atmosphereConstants->m_innerRadius, this);
		antSetOuterRadius(&m_atmosphereConstants->m_outerRadius, this);
		antSetKr(&m_atmosphereConstants->m_kr, this);
		antSetKm(&m_atmosphereConstants->m_km, this);
		antSetESun(&m_atmosphereConstants->m_eSun, this);
		antSetG(&m_atmosphereConstants->m_g, this);
		antSetScaleDepth(&m_atmosphereConstants->m_scaleDepth, this);
		antSetSamples(&m_atmosphereConstants->m_samples, this);

		// Setup sky vertex array
		glBindVertexArray(m_skyDrawVertexArray.m_id);

		// Create dome
		std::vector<glm::vec4> points; std::vector<GLuint> indexes;
		buildSphereApproximation(atmosphereConstants->m_patchSize, points, indexes);

		// Set up vertex info
		glBindBuffer(GL_ARRAY_BUFFER, m_skyVertexBuffer.m_id);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * points.size(), &points[0], GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), 0);

		// Set up index info
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_skyIndexBuffer.m_id);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indexes.size(), &indexes[0], GL_STATIC_DRAW);
		m_numSkyIndexes = (GLint)indexes.size();

		glBindBufferBase(GL_UNIFORM_BUFFER, PLANET_UNIFORMS_BINDING_POINT, m_uniformBuffer.m_id);

		glBindVertexArray(0);
	}
}

Planet::~Planet()
{
	delete m_terrainGenerator;
	delete m_position;
	TwDeleteBar(m_overlay_bar);
}


static inline int patchVisible(
	const glm::vec3& v3_cameraPos_MS,
	const PatchHash& hash,
	const PatchHash& currentPatchHashPosition,
	const PatchBoundingVectors& vecs,
	const Frustum& frustum
)
{
	const float maxDist2 = glm::length2(v3_cameraPos_MS) - 1.0f;
	return (
		(
			currentPatchHashPosition.m_value != hash.m_value && 
			dist2PointToLineSegment(v3_cameraPos_MS, vecs.m_corner00, vecs.m_corner01) > maxDist2 && // FIX maxDist2 so this works underwater!
			dist2PointToLineSegment(v3_cameraPos_MS, vecs.m_corner00, vecs.m_corner10) > maxDist2 &&
			dist2PointToLineSegment(v3_cameraPos_MS, vecs.m_corner11, vecs.m_corner01) > maxDist2 &&
			dist2PointToLineSegment(v3_cameraPos_MS, vecs.m_corner11, vecs.m_corner10) > maxDist2
		) ||
		frustum.sphereOutside(vecs.m_center, vecs.m_radius)
	) ? 0 : 1;
}

void Planet::updateGeneral(const WorldClock& worldClock)
{
	m_m4d_absTerrainM = glm::scale(m_position->getMatrix(), glm::dvec3(m_radius));
	if (m_water) m_water->update(worldClock);
}

void Planet::updateForCamera(const Camera* camera)
{
	m_m4f_zeroPosUnscaledMV = camera->getZeroViewMatrix() * glm::mat4(camera->getInvPosMatrix() * m_position->getMatrix());
	m_v3f_planetPos_VS = matrixPosition(m_m4f_zeroPosUnscaledMV);
}

FloatPair Planet::getMinMaxDrawDist() const
{
	const float distanceToCenter = glm::length(m_v3f_planetPos_VS);
	const float totalRadius = m_atmosphereConstants ? m_atmosphereConstants->m_outerRadius : m_radius;
	const float distanceToHorizon = sqrt(distanceToCenter*distanceToCenter - m_radius*m_radius);

	return std::make_pair(
		std::max(distanceToCenter - totalRadius, 0.0f),
		distanceToHorizon + m_maxDistSurfaceToSky
	);
}

void Planet::draw(const Scene* scene, const Camera* camera)
{
	std::vector<PlanetPatch*> drawList;

	//PLANET_DATA_BUFFER->m_bufferLock.acquire();

	populateDrawLists(scene, camera, drawList);

	// Draw terrain and sky
	drawImmediate(scene, camera, drawList);

	// If there are patches to be calculated, don't release the buffer lock,
	// because garbage collection will corrupt the state of the buffer - 
	// calculation of queued patches must finish first.
	//glFinish();
	//if (m_queuedPatches.empty())
	//	PLANET_DATA_BUFFER->m_bufferLock.release(); // What about multiple shapes??!?
}

void Planet::populateDrawLists(const Scene* scene, const Camera* camera, std::vector<PlanetPatch*>& drawList)
{
	const unsigned oldQueueSize = (unsigned)m_queuedPatches.size();
	m_queuedPatches.clear();

	const glm::vec3 v3f_cameraPos_MS(glm::inverse(m_m4d_absTerrainM) * glm::dvec4(camera->getAbsPosition(), 1.0f));
	m_overlay_altitude = glm::length(m_v3f_planetPos_VS) - m_radius;

	PatchOrientation eyePatchOrientation; glm::vec3 eyePatchPosition;
	cameraPositionToPatchPosition(v3f_cameraPos_MS, &eyePatchOrientation, eyePatchPosition);
	
	const Frustum frustum(glm::mat4(camera->getAbsViewProjectionMatrix() * m_m4d_absTerrainM));

	std::vector<std::pair<PlanetPatch*, bool>> patchQueue;
	patchQueue.reserve(100000);
	for (int i = 0; i < m_rootPatches.size(); ++i)
		patchQueue.emplace_back(m_rootPatches[i], false);

	// Reset variables
	m_overlay_lowestPatchLevel = 10000;
	m_overlay_highestPatchLevel = -1;
	m_overlay_patchesTraversed = 0;
	m_overlay_patchesDiscarded = 0;

	// We should be grouping patches into which edges are drawn based on the detail level of neighbouring patches.
	// For now, we'll just draw everything at maximum detail and cope with the seams.

	const PatchHash currentPatchHashPositions[28] = {
		makePatchHash(eyePatchOrientation, 0, eyePatchPosition.x, eyePatchPosition.y),
		makePatchHash(eyePatchOrientation, 1, eyePatchPosition.x, eyePatchPosition.y),
		makePatchHash(eyePatchOrientation, 2, eyePatchPosition.x, eyePatchPosition.y),
		makePatchHash(eyePatchOrientation, 3, eyePatchPosition.x, eyePatchPosition.y),
		makePatchHash(eyePatchOrientation, 4, eyePatchPosition.x, eyePatchPosition.y),
		makePatchHash(eyePatchOrientation, 5, eyePatchPosition.x, eyePatchPosition.y),
		makePatchHash(eyePatchOrientation, 6, eyePatchPosition.x, eyePatchPosition.y),
		makePatchHash(eyePatchOrientation, 7, eyePatchPosition.x, eyePatchPosition.y),
		makePatchHash(eyePatchOrientation, 8, eyePatchPosition.x, eyePatchPosition.y),
		makePatchHash(eyePatchOrientation, 9, eyePatchPosition.x, eyePatchPosition.y),
		makePatchHash(eyePatchOrientation, 10, eyePatchPosition.x, eyePatchPosition.y),
		makePatchHash(eyePatchOrientation, 11, eyePatchPosition.x, eyePatchPosition.y),
		makePatchHash(eyePatchOrientation, 12, eyePatchPosition.x, eyePatchPosition.y),
		makePatchHash(eyePatchOrientation, 13, eyePatchPosition.x, eyePatchPosition.y),
		makePatchHash(eyePatchOrientation, 14, eyePatchPosition.x, eyePatchPosition.y),
		makePatchHash(eyePatchOrientation, 15, eyePatchPosition.x, eyePatchPosition.y),
		makePatchHash(eyePatchOrientation, 16, eyePatchPosition.x, eyePatchPosition.y),
		makePatchHash(eyePatchOrientation, 17, eyePatchPosition.x, eyePatchPosition.y),
		makePatchHash(eyePatchOrientation, 18, eyePatchPosition.x, eyePatchPosition.y),
		makePatchHash(eyePatchOrientation, 19, eyePatchPosition.x, eyePatchPosition.y),
		makePatchHash(eyePatchOrientation, 20, eyePatchPosition.x, eyePatchPosition.y),
		makePatchHash(eyePatchOrientation, 21, eyePatchPosition.x, eyePatchPosition.y),
		makePatchHash(eyePatchOrientation, 22, eyePatchPosition.x, eyePatchPosition.y),
		makePatchHash(eyePatchOrientation, 23, eyePatchPosition.x, eyePatchPosition.y),
		makePatchHash(eyePatchOrientation, 24, eyePatchPosition.x, eyePatchPosition.y),
		makePatchHash(eyePatchOrientation, 25, eyePatchPosition.x, eyePatchPosition.y),
		makePatchHash(eyePatchOrientation, 26, eyePatchPosition.x, eyePatchPosition.y),
		makePatchHash(eyePatchOrientation, 27, eyePatchPosition.x, eyePatchPosition.y)
	};

	// Find current ground altitude
	for (int i = 0; i < 28; ++i)
	{
		auto it = m_patchMap.find(currentPatchHashPositions[i].m_value);
		if (it == m_patchMap.end())
			break;

		if (!it->second->m_populated)
			continue;

		m_overlay_groundAltitude = it->second->m_averageAltitude - 1.0f;
	}

	for (auto it = patchQueue.begin(); it != patchQueue.end(); ++it)
	{
		++m_overlay_patchesTraversed;
		PlanetPatch* const patch = it->first;
		const bool parentAlreadyDrawn = it->second;
		
		// Note: this can be made faster - do it later if necessary
		const PatchHash& hash = patch->m_hash;
		const int patchLevel = hash.getLevel();
		
		// log(x^2) == log(x)*2, therefore right shift by 1 (divide by 2) at the end:
		// this means we don't need a sqrtf via glm::length.
		const int desiredLevel = fastIntMaxZero(fastCeil(fastLog2(
			GLOBALS.m_planetLevel1Distance * GLOBALS.m_planetLevel1Distance / 
			glm::length2(v3f_cameraPos_MS - patch->m_boundingVectors.m_center)
		))) >> 1;
		
		if (desiredLevel > patchLevel && patchLevel < GLOBALS.m_maxPlanetPatchLevel) 
		{
			// We need to traverse deeper

			if (!patch->m_children)
			{
				// Need children drawn - make child patches if necessary
				
				const PatchOrientation po = hash.getOrientation();
				const float dim0 = hash.getDim0();
				const float dim1 = hash.getDim1();
				const float halfSize = hash.getSize() / 2.0f;

				PlanetPatch* children = (PlanetPatch*)malloc(4 * sizeof(PlanetPatch));
				new (children + 0) PlanetPatch(makePatchHash(po, patchLevel + 1, dim0, dim1), 0, patch);
				new (children + 1) PlanetPatch(makePatchHash(po, patchLevel + 1, dim0+halfSize, dim1), 1, patch);
				new (children + 2) PlanetPatch(makePatchHash(po, patchLevel + 1, dim0, dim1+halfSize), 2, patch);
				new (children + 3) PlanetPatch(makePatchHash(po, patchLevel + 1, dim0+halfSize, dim1+halfSize), 3, patch);
				patch->m_children = children;
				m_patchMap.emplace((children+0)->m_hash.m_value, children+0);
				m_patchMap.emplace((children+1)->m_hash.m_value, children+1);
				m_patchMap.emplace((children+2)->m_hash.m_value, children+2);
				m_patchMap.emplace((children+3)->m_hash.m_value, children+3);
			}

			const int c0Visible = patchVisible(v3f_cameraPos_MS, (patch->m_children + 0)->m_hash, currentPatchHashPositions[patchLevel + 1], (patch->m_children + 0)->m_boundingVectors, frustum);
			const int c1Visible = patchVisible(v3f_cameraPos_MS, (patch->m_children + 1)->m_hash, currentPatchHashPositions[patchLevel + 1], (patch->m_children + 1)->m_boundingVectors, frustum);
			const int c2Visible = patchVisible(v3f_cameraPos_MS, (patch->m_children + 2)->m_hash, currentPatchHashPositions[patchLevel + 1], (patch->m_children + 2)->m_boundingVectors, frustum);
			const int c3Visible = patchVisible(v3f_cameraPos_MS, (patch->m_children + 3)->m_hash, currentPatchHashPositions[patchLevel + 1], (patch->m_children + 3)->m_boundingVectors, frustum);

			// Check visibility of children
			const int childVisibleMask = (c0Visible << 0) | (c1Visible << 1) | (c2Visible << 2) | (c3Visible << 3);

			if (!parentAlreadyDrawn && patch->m_populated && patch->m_numChildrenPopulated < childVisibleMask)
			{
				drawList.push_back(patch); // Draw this until children ready
				// Children are not drawable!
				if (c0Visible) patchQueue.emplace_back(patch->m_children + 0, true);
				if (c1Visible) patchQueue.emplace_back(patch->m_children + 1, true);
				if (c2Visible) patchQueue.emplace_back(patch->m_children + 2, true);
				if (c3Visible) patchQueue.emplace_back(patch->m_children + 3, true);
			}
			else
			{
				if (c0Visible) patchQueue.emplace_back(patch->m_children + 0, parentAlreadyDrawn);
				if (c1Visible) patchQueue.emplace_back(patch->m_children + 1, parentAlreadyDrawn);
				if (c2Visible) patchQueue.emplace_back(patch->m_children + 2, parentAlreadyDrawn);
				if (c3Visible) patchQueue.emplace_back(patch->m_children + 3, parentAlreadyDrawn);

				if (!patch->m_populated)
					m_queuedPatches.push_back(patch);
			}
		}
		else // Need this patch drawn
		{
			if (patch->m_populated)
			{
				m_overlay_lowestPatchLevel = std::min(patchLevel, m_overlay_lowestPatchLevel);
				m_overlay_highestPatchLevel = std::max(patchLevel, m_overlay_highestPatchLevel);
				if (!parentAlreadyDrawn)
					drawList.push_back(patch);
			}
			else
			{
				m_queuedPatches.push_back(patch);
			}
		}
	}
	
	if (m_queuedPatches.size() > 0 && oldQueueSize == 0)
		ComputeQueue::get().addClient(this);
	
	m_overlay_numPatches = (int)m_patchMap.size();
	m_overlay_queueSize = (int)m_queuedPatches.size();
}


void Planet::drawImmediate(const Scene* scene, const Camera* camera, const std::vector<PlanetPatch*>& drawList)
{
	m_overlay_terrainPatchesDrawn = 0;
	m_overlay_waterPatchesDrawn = 0;

	const float distanceToCenter = glm::length(m_v3f_planetPos_VS);

	// Decide which programs to use
	const bool inAtmosphere =
		m_atmosphereConstants &&
		(distanceToCenter <= m_atmosphereConstants->m_outerRadius)
	;

	const std::vector<LightSource*>& lightSources = scene->getLightSources();
	assert(lightSources.size() == 1);

	// Iterate over visible patches and split into terrain and water
	const GLsizei numIndexes = (GLsizei)(PLANET_PATCH_CONSTANTS->m_allIndexes.size());
	const double currentTime = glfwGetTime();

	GLsizei* counts = new GLsizei[drawList.size()];
	GLvoid** indices = new GLvoid*[drawList.size()];
	GLint* terrainBaseVertexes = new GLint[drawList.size()];
	GLint* waterBaseVertexes = new GLint[drawList.size()];

	unsigned numTerrainFound = 0, numWaterFound = 0;

	for (int drawListIndex = 0; drawListIndex < drawList.size(); ++drawListIndex)
	{
		const PlanetPatch* const patch = drawList[drawListIndex];

		if (!m_water || patch->m_numSubmerged < PLANET_PATCH_CONSTANTS->m_visibleVertices) // There is at least some land
			terrainBaseVertexes[numTerrainFound++] = patch->m_bufferOffset * PLANET_PATCH_CONSTANTS->m_totalVertices;

		if (m_water && patch->m_numSubmerged > 0) // There is at least some water
			waterBaseVertexes[numWaterFound++] = patch->m_bufferOffset * PLANET_PATCH_CONSTANTS->m_totalVertices;

		counts[drawListIndex] = numIndexes;
		indices[drawListIndex] = (GLvoid*)0;
		PLANET_DATA_BUFFER->m_lastDrawnTimes[patch->m_bufferOffset] = currentTime;
	}

	// Update uniforms
	m_uniforms.m4_terrainMV = glm::scale(m_m4f_zeroPosUnscaledMV, glm::vec3(m_radius));
	m_uniforms.m4_terrainMVP = camera->getProjectionMatrix() * m_uniforms.m4_terrainMV;
	if (m_atmosphereConstants)
	{
		m_uniforms.m4_skyMV = glm::scale(m_m4f_zeroPosUnscaledMV, glm::vec3(m_atmosphereConstants->m_outerRadius));
		m_uniforms.m4_skyMVP = camera->getProjectionMatrix() * m_uniforms.m4_skyMV;
	}
	m_uniforms.v3_lightCol = lightSources[0]->getLightSourceColour();
	m_uniforms.v3_lightPos_VS = lightSources[0]->m_v3f_pos_VS;
	m_uniforms.v3_lightDir_VS = glm::normalize(m_uniforms.v3_lightPos_VS - m_v3f_planetPos_VS);
	m_uniforms.v3_planetPos_VS = m_v3f_planetPos_VS;
	m_uniforms.v3_cameraFromCenter_VS = -m_v3f_planetPos_VS;
	m_uniforms.f_depthCoef = camera->getDepthFCoef();
	m_uniforms.f_cameraHeight = distanceToCenter;
	m_uniforms.f_cameraHeight2 = distanceToCenter * distanceToCenter;

	glBindBuffer(GL_UNIFORM_BUFFER, m_uniformBuffer.m_id);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(m_uniforms), (const GLvoid*)&m_uniforms);
	glBindBufferBase(GL_UNIFORM_BUFFER, PLANET_UNIFORMS_BINDING_POINT, m_uniformBuffer.m_id);
	
	if (numTerrainFound > 0) // Set up terrain program and draw terrain
	{
		m_overlay_terrainPatchesDrawn = numTerrainFound;
		TerrainDrawProgram* const terrainDrawProgram = inAtmosphere ? m_terrainInAtmProgram : m_terrainOutAtmProgram;

		glBindVertexArray(m_terrainDrawVertexArray.m_id);
		glUseProgram(terrainDrawProgram->m_program->m_id);
		glMultiDrawElementsBaseVertex(GL_TRIANGLES, counts, GL_UNSIGNED_INT, indices, (GLsizei)numTerrainFound, terrainBaseVertexes);
	}

	if (numWaterFound > 0) // Set up water program and draw water
	{
		m_overlay_waterPatchesDrawn = numWaterFound;

		glBindVertexArray(m_waterDrawVertexArray.m_id);
		glUseProgram(m_water->m_program->m_id);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glMultiDrawElementsBaseVertex(GL_TRIANGLES, counts, GL_UNSIGNED_INT, indices, (GLsizei)numWaterFound, waterBaseVertexes);
		glDisable(GL_BLEND);
	}

	delete[] counts;
	delete[] indices;
	delete[] terrainBaseVertexes;
	delete[] waterBaseVertexes;

	if (m_atmosphereConstants)
	{
		// Setup sky program
		SkyDrawProgram* const skyDrawProgram = inAtmosphere ? m_skyInAtmProgram : m_skyOutAtmProgram;

		glBindVertexArray(m_skyDrawVertexArray.m_id);
		glUseProgram(skyDrawProgram->m_program->m_id);
		glCullFace(GL_FRONT);
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
		glDrawElements(GL_TRIANGLES, m_numSkyIndexes, GL_UNSIGNED_INT, 0);
		glDisable(GL_BLEND);
		glCullFace(GL_BACK);
	}

	glBindVertexArray(0);
}

unsigned Planet::runAllComputeItems()
{
	bool allRun;
	return runSomeComputeItems((int)m_queuedPatches.size(), allRun);
}

static inline float sortableUintToFloat(unsigned sortableUint)
{
	const unsigned result = sortableUint ^ (((sortableUint >> 31) - 1) | 0x80000000);
	return *reinterpret_cast<const float*>(&result);
}

unsigned Planet::runSomeComputeItems(int maxNumBatches, bool& allRun)
{
	if (maxNumBatches == 0)
	{
		allRun = m_queuedPatches.empty();
		return 0;
	}

	//printf("Total %d patches\n", m_queuedPatches.size());

	glBindVertexArray(m_terrainGenerator->m_vertexArray.m_id);
	glUseProgram(m_terrainGenerator->m_program->m_id);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, PLANET_DATA_BUFFER->m_vertexBuffer.m_id);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, PLANET_DATA_BUFFER->m_statsBuffer.m_id);

	std::vector<PlanetPatch*> calculatedPatches;

	// Bind stats buffer (only needed for clearing/retrieving data)
	glBindBuffer(GL_ARRAY_BUFFER, PLANET_DATA_BUFFER->m_statsBuffer.m_id);
	
	// Clear before first use
	PLANET_DATA_BUFFER->clearStatsBuffer();

	unsigned statsOffset = 0;

	assert(PLANET_DATA_BUFFER->m_bufferSizePatches <= 2097152); // So the offset fits in 21 bits
	assert(PLANET_DATA_BUFFER->m_statsBufferSizePatches <= 256); // So the offset fits in 8 bits

	// Iterate over batches
	int batchNumber = 0;
	for ( ; !m_queuedPatches.empty() && batchNumber < maxNumBatches; ++batchNumber)
	{
		// Run one batch
		std::vector<glm::vec4> patchDetails;
		
		for (unsigned patchNumber = 0; !m_queuedPatches.empty() && patchNumber < PLANET_PATCH_CONSTANTS->m_patchesPerBatch; ++patchNumber)
		{
			PlanetPatch* const patch = m_queuedPatches.front();
			m_queuedPatches.pop_front();

			patch->m_populated = true;
			patch->m_bufferOffset = PLANET_DATA_BUFFER->getOffset(patch, this);

			if (patch->m_parent)
				patch->m_parent->m_numChildrenPopulated |= (1 << patch->m_childNumber);

			const float stepSize = patch->m_hash.getSize() / PLANET_PATCH_CONSTANTS->m_visiblePolygons;
			const unsigned orientationAndOffsetInt = 
				((unsigned)(patch->m_hash.getOrientation()) << 29) | 
				((statsOffset++) << 21) | 
				patch->m_bufferOffset
			;

			patchDetails.emplace_back(
				reinterpret_cast<const GLfloat&>(orientationAndOffsetInt),
				stepSize,
				patch->m_hash.getDim0() - stepSize, // dim0Start
				patch->m_hash.getDim1() - stepSize // dim1Start
			);
			calculatedPatches.push_back(patch);
		}
		
		// Now we have all the details to run this batch; set uniforms and run.
		glUniform4fv(m_terrainGenerator->m_locId_patchDetails, (GLsizei)patchDetails.size(), &patchDetails[0].x);
		glDispatchCompute((GLuint)patchDetails.size(), 1, 1);
		
		const unsigned numPatchesInNextBatch = 
			(batchNumber >= maxNumBatches - 1) ? 0 :
			std::min((unsigned)m_queuedPatches.size(), PLANET_PATCH_CONSTANTS->m_patchesPerBatch)
		;
		const unsigned spaceLeftInStatsBuffer = PLANET_DATA_BUFFER->m_statsBufferSizePatches - statsOffset;

		// If the client stats buffer needs to be cleared because it will overrun next iteration,
		// or because this is the last iteration, we download its contents, send them to
		// the appropriate patches, and reset the buffers if necessary.
		if (numPatchesInNextBatch > spaceLeftInStatsBuffer || numPatchesInNextBatch == 0)
		{
			//PLANET_DATA_BUFFER->downloadStatsBuffer();

			for (int i = 0; i < calculatedPatches.size(); ++i)
			{
				calculatedPatches[i]->setAltitudes(
					sortableUintToFloat(PLANET_DATA_BUFFER->m_statsDataClientBuffer[i].x), 
					sortableUintToFloat(PLANET_DATA_BUFFER->m_statsDataClientBuffer[i].y)
				);
				calculatedPatches[i]->m_numSubmerged = PLANET_DATA_BUFFER->m_statsDataClientBuffer[i].z;
			}

			if (numPatchesInNextBatch > 0)
			{
				statsOffset = 0;
				calculatedPatches.clear();
				PLANET_DATA_BUFFER->clearStatsBuffer();
			}
		}
	}

	PLANET_DATA_BUFFER->m_bufferLock.release();
	
	allRun = m_queuedPatches.empty();
	return (unsigned)batchNumber;
}

Planet* Planet::buildFromXMLNode(XMLNode& node)
{
	XMLChildFinder finder(node);

	const float radius = finder.required<float>("Radius", buildFloatFromXMLNode);

	return new Planet(
		finder.required<std::string>("Name", buildStringFromXMLNode), 
		radius,
		finder.required<TerrainGenerator*>("PatchGenerator", TerrainGenerator::buildFromXMLNode), 
		addRadiusToConstants(
			finder.optional("Atmosphere", AtmosphereConstants::buildFromXMLNode), 
			radius
		), 
		finder.optional("Water", Water::buildFromXMLNode), 
		finder.required("Position", Position::buildFromXMLNode)
	);
}
