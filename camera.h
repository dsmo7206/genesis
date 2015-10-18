#pragma once

#include <hash_map>
#include "glstuff.h"
#include "overlay.h"
#include "position.h"

class WorldClock;

class Frustum
{
	public:

	const glm::mat4 m_checkMVP;

	const glm::vec4 m_rightPlane;
	const glm::vec4 m_leftPlane;
	const glm::vec4 m_bottomPlane;
	const glm::vec4 m_topPlane;
	const glm::vec4 m_farPlane;
	const glm::vec4 m_nearPlane;

	Frustum(const glm::mat4& mvpMatrix);
	inline bool sphereOutside(const glm::vec3& center, float radius) const
	{
		const glm::vec4 center4(center, 1.0);

		return glm::dot(center4, m_nearPlane) <= -radius
			|| glm::dot(center4, m_rightPlane) <= -radius
			|| glm::dot(center4, m_leftPlane) <= -radius
			|| glm::dot(center4, m_bottomPlane) <= -radius
			|| glm::dot(center4, m_topPlane) <= -radius
			|| glm::dot(center4, m_farPlane) <= -radius
		;
	}
};

class Camera
{
	float m_fovY;
	float m_zNear;
	float m_zFar;
	float m_f_depthCoef;
	
	RelativePosition* const m_cameraPosition;
	glm::dvec3 m_v3d_cameraDir;
	glm::dvec3 m_v3d_cameraUp;
	glm::mat4 m_projectionMatrix;
	glm::mat4 m_zeroViewMatrix;
	glm::mat4 m_zeroViewProjectionMatrix;
	glm::dmat4 m_absViewMatrix;
	glm::dmat4 m_absViewProjectionMatrix;
	glm::dmat4 m_m4d_invPos;
	glm::dvec3 m_absPosition;

	double m_lookSpeed;
	double m_moveSpeed;

	TwBar* m_settingsBar;

	void refreshDepthFCoef();
	void refreshFrustum();
	
	public:

	const std::string m_name;

	Camera(
		const std::string& name,
		float fovY, float zNear, float zFar,
		RelativePosition* cameraPosition, const glm::dvec3& cameraDirection,
		const glm::dvec3& cameraUp, double lookSpeed, double moveSpeed
	);
	~Camera();

	inline const RelativePosition* getPosition() const { return m_cameraPosition; }
	
	inline float getFovY() const { return m_fovY; }
	inline float getZNear() const { return m_zNear; }
	inline float getZFar() const { return m_zFar; }
	inline float getDepthFCoef() const { return m_f_depthCoef; }

	inline const glm::dvec3& getAbsPosition() const { return m_absPosition; }
	inline const glm::dmat4& getInvPosMatrix() const { return m_m4d_invPos; }
	inline const glm::mat4& getProjectionMatrix() const { return m_projectionMatrix; }
	inline const glm::mat4& getZeroViewMatrix() const { return m_zeroViewMatrix; }
	inline const glm::mat4& getZeroViewProjectionMatrix() const { return m_zeroViewProjectionMatrix; }
	inline const glm::dmat4& getAbsViewProjectionMatrix() const { return m_absViewProjectionMatrix; }

	void setFovY(float fovY);
	void setZNear(float zNear);
	void setZFar(float zFar);

	void refreshProjectionMatrix();
	void refreshViewMatrixes();
	void refreshViewProjectionMatrixes();
	
	virtual void update(const WorldClock& worldClock, double mouseX, double mouseY);

	static Camera* buildFromXMLNode(XMLNode& node);
};

