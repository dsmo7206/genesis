#include <algorithm>
#include "scene.h"
#include "globals.h"
#include "overlay.h"
#include "planet.h"
#include "camera.h"
#include "lightsource.h"
#include "world_clock.h"

Scene::Scene(
	const std::string& name, 
	const std::vector<Camera*>& cameras,
	const std::vector<Shape*>& shapes,
	const std::vector<LightSource*>& lightSources
) :
	m_name(name), m_shapes(shapes), m_lightSources(lightSources)
{
	for (auto cameraPtr : cameras)
		m_cameras.emplace(cameraPtr->m_name, cameraPtr);

	// Resolve all. The interface to resolving should be changed so that
	// a bool can be returned. Anything successfully resolved will be removed from the list.
	// Otherwise, in the presence of multiple scenes, each scene will try to resolve
	// each resolveable, causing either errors on no-resolve-possible (for a particular scene)
	// or (slight) slowdown due to redundancy.
	for (auto sceneResolveablePtr : SCENE_RESOLVEABLES)
		sceneResolveablePtr->resolve(this);
	SCENE_RESOLVEABLES.clear();
}

Scene::~Scene()
{
	for (auto shapePtr : m_shapes)
		delete shapePtr;
}

Shape* Scene::findShapeByName(const std::string& name)
{
	for (auto shapePtr : m_shapes)
		if (shapePtr->m_name == name)
			return shapePtr;

	return nullptr;
}

struct ShapeSortFunctor
{
	inline bool operator()(Shape* s1, Shape* s2)
	{
		// Sort in DESCENDING order (farthest to nearest)
		return s1->getMinMaxDrawDist().first > s2->getMinMaxDrawDist().first;
	}
};

void Scene::draw(const Camera* const camera)
{
	//m_skyBox.draw(camera);

	for (auto shapePtr : m_shapes)
		shapePtr->draw(this, camera);
}

void Scene::updateGeneral(const WorldClock& worldClock, double mouseX, double mouseY)
{
	// Update positions first - everything depends on these
	for (auto shapePtr : m_shapes)
		shapePtr->m_position->update(worldClock);

	// Update cameras next - shapes need the inverse matrices
	for (auto & it : m_cameras)
		it.second->update(worldClock, mouseX, mouseY);

	for (auto shapePtr : m_shapes)
		shapePtr->updateGeneral(worldClock);
}

void Scene::updateForCamera(const Camera* camera)
{
	for (auto shapePtr : m_shapes)
		shapePtr->updateForCamera(camera);
}

Scene* Scene::buildFromXMLNode(XMLNode& node)
{
	XMLChildFinder finder(node);

	const std::vector<Shape*> shapes = finder.requiredVector("Shapes", Shape::buildFromXMLNode);

	std::vector<LightSource*> lightSources;
	for (auto shapePtr : shapes)
	{
		LightSource* const lightSource = dynamic_cast<LightSource*>(shapePtr);
		if (lightSource)	
			lightSources.push_back(lightSource);
	}

	if (lightSources.size() > 1)
		raiseXMLException(node, "Too many light sources");

	return new Scene(
		finder.required("Name", buildStringFromXMLNode), 
		finder.requiredVector("Cameras", Camera::buildFromXMLNode),
		shapes, 
		lightSources
	);
}

SceneMap SCENE_MAP;
