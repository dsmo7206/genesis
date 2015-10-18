#pragma once

#include <vector>
#include <hash_map>

#include "overlay.h"
#include "xml.h"
#include "skybox.h"

class Shape;
class Camera;
class LightSource;
class WorldClock;

typedef std::hash_map<std::string, Camera*> CameraMap;

class Scene
{
	public:

	const std::string m_name;
	CameraMap m_cameras;
	std::vector<Shape*> m_shapes;
	std::vector<LightSource*> m_lightSources;
	SkyBox m_skyBox;

	Scene(
		const std::string& name, 
		const std::vector<Camera*>& cameras,
		const std::vector<Shape*>& shapes, 
		const std::vector<LightSource*>& lightSources
	);
	virtual ~Scene();

	Shape* findShapeByName(const std::string& name);
	const std::vector<LightSource*>& getLightSources() const { return m_lightSources; }

	virtual void updateGeneral(const WorldClock& worldClock, double mouseX, double mouseY);
	virtual void updateForCamera(const Camera* camera);

	virtual void draw(const Camera* const camera);

	static Scene* buildFromXMLNode(XMLNode& node);
};

typedef std::hash_map<std::string, Scene*> SceneMap;
extern SceneMap SCENE_MAP;
