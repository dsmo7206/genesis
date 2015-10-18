#pragma once

#include <vector>
#include <hash_map>
#include <algorithm>

#include "shapes.h"
#include "planet_patch.h"
#include "shader_program.h"
#include "overlay.h"
#include "terrain_generators.h"
#include "planet_programs.h"
#include "position.h"
#include "xml.h"
#include "compute_queue.h"

class Camera;

class Planet : public Shape, public ComputeClient
{
	public:

	// Constants
	const float m_radius;
	const float m_maxDistSurfaceToSky;
	AtmosphereConstants* const m_atmosphereConstants;
	TwBar* const m_overlay_bar;

	// All the patches for the terrain
	const std::vector<PlanetPatch*> m_rootPatches;
	PlanetPatchMap m_patchMap;

	Water* const m_water;

	// Patches waiting for calculation
	std::deque<PlanetPatch*> m_queuedPatches;

	// Time-specific variables
	glm::dmat4 m_m4d_absTerrainM; // Scaled
	glm::mat4 m_m4f_zeroPosUnscaledMV;
	glm::vec3 m_v3f_planetPos_VS;

	// Buffers etc
	VertexArray m_terrainDrawVertexArray;
	VertexArray m_skyDrawVertexArray;
	VertexArray m_waterDrawVertexArray;
	VertexBuffer m_skyVertexBuffer;
	VertexBuffer m_skyIndexBuffer;
	VertexBuffer m_uniformBuffer;
	GLsizei m_numSkyIndexes;

	PlanetUniforms m_uniforms;

	// Shader programs
	TerrainDrawProgram* const m_terrainInAtmProgram;
	TerrainDrawProgram* const m_terrainOutAtmProgram;
	SkyDrawProgram* const m_skyInAtmProgram; // Null if no atmosphere
	SkyDrawProgram* const m_skyOutAtmProgram; // Null if no atmosphere
	TerrainGenerator* const m_terrainGenerator;

	// AntTweakBar parameters for terrain
	int m_overlay_patchesTraversed;
	int m_overlay_patchesDiscarded;
	int m_overlay_numPatches;
	int m_overlay_queueSize;
	int m_overlay_terrainPatchesDrawn;
	int m_overlay_waterPatchesDrawn;
	int m_overlay_lowestPatchLevel;
	int m_overlay_highestPatchLevel;
	float m_overlay_altitude;
	float m_overlay_groundAltitude;
	
	void drawImmediate(const Scene* scene, const Camera* camera, const std::vector<PlanetPatch*>& drawList);
	
	Planet(
		const std::string& name, 
		float radius, TerrainGenerator* terrainGenerator, 
		AtmosphereConstants* atmosphereConstants,
		Water* water, Position* position
	);
	~Planet();
	
	void updateGeneral(const WorldClock& worldClock) override;
	void updateForCamera(const Camera* camera) override;

	FloatPair getMinMaxDrawDist() const override;

	void draw(const Scene* scene, const Camera* camera) override;

	inline void notifyPatchDelete(PlanetPatch* patch)
	{
		m_patchMap.erase(patch->m_hash.m_value);
	}

	void populateDrawLists(const Scene* scene, const Camera* camera, std::vector<PlanetPatch*>& drawList);

	// ComputeClient implementations
	unsigned runAllComputeItems() override;
	unsigned runSomeComputeItems(int count, bool& allRun) override;

	static Planet* buildFromXMLNode(XMLNode& node);
};
