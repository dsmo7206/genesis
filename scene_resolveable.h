#pragma once

#include <vector>

class Scene;

class SceneResolveable
{
	bool m_resolved;
	virtual void _resolve(Scene* scene) = 0;

	public:

	SceneResolveable(bool resolved);
	inline void resolve(Scene* scene) 
	{ 
		if (!m_resolved) 
		{ 
			_resolve(scene); 
			m_resolved = true; 
		} 
	}
};

extern std::vector<SceneResolveable*> SCENE_RESOLVEABLES;
