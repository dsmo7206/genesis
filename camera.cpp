#include "camera.h"
#include "globals.h"
#include "world_clock.h"

void TW_CALL antSetFovY(const void* value, void* clientData) 
{ 
	((Camera*)clientData)->setFovY(*(const float*)value);
}
void TW_CALL antSetZNear(const void* value, void* clientData)
{ 
	((Camera*)clientData)->setZNear(*(const float*)value); 
}
void TW_CALL antSetZFar(const void* value, void* clientData) 
{ 
	((Camera*)clientData)->setZFar(*(const float*)value); 
}

void TW_CALL antGetFovY(void* value, void* clientData) 
{ 
	*(float*)value = ((Camera*)clientData)->getFovY(); 
}
void TW_CALL antGetZNear(void* value, void* clientData) 
{ 
	*(float*)value = ((Camera*)clientData)->getZNear(); 
}
void TW_CALL antGetZFar(void* value, void* clientData) 
{ 
	*(float*)value = ((Camera*)clientData)->getZFar(); 
}

Frustum::Frustum(const glm::mat4& mvp) :
	m_checkMVP   (glm::transpose(mvp)),
	m_rightPlane (glm::normalize(m_checkMVP[3] - m_checkMVP[0])),
	m_leftPlane  (glm::normalize(m_checkMVP[3] + m_checkMVP[0])),
	m_bottomPlane(glm::normalize(m_checkMVP[3] + m_checkMVP[1])),
	m_topPlane   (glm::normalize(m_checkMVP[3] - m_checkMVP[1])),
	m_farPlane   (glm::normalize(m_checkMVP[3] - m_checkMVP[2])),
	m_nearPlane  (glm::normalize(m_checkMVP[3] + m_checkMVP[2]))
{
}

Camera::Camera(
	const std::string& name,
	float fovY, float zNear, float zFar,
	RelativePosition* cameraPosition, const glm::dvec3& cameraDirection,
	const glm::dvec3& cameraUp, double lookSpeed, double moveSpeed
) :
	m_name(name),
	m_fovY(fovY), m_zNear(zNear), m_zFar(zFar),
	m_cameraPosition(cameraPosition), m_v3d_cameraDir(cameraDirection),
	m_v3d_cameraUp(cameraUp), m_lookSpeed(lookSpeed), m_moveSpeed(moveSpeed)
{
	refreshProjectionMatrix();
	refreshViewMatrixes();
	refreshViewProjectionMatrixes();
	refreshDepthFCoef();

	// Create settings bar
	m_settingsBar = TwNewBar("Camera");
	TwSetParam((TwBar*)m_settingsBar, NULL, "refresh", TW_PARAM_CSTRING, 1, "0.1");
	TwSetParam((TwBar*)m_settingsBar, NULL, "position", TW_PARAM_CSTRING, 1, "16 400");
	
	TwAddVarCB((TwBar*)m_settingsBar, "Vertical FOV", TW_TYPE_FLOAT, antSetFovY, antGetFovY, this, " min=0.1 max=179.9 step=0.1 "); 
	TwAddVarCB((TwBar*)m_settingsBar, "z Near", TW_TYPE_FLOAT, antSetZNear, antGetZNear, this, nullptr); 
	TwAddVarCB((TwBar*)m_settingsBar, "z Far", TW_TYPE_FLOAT, antSetZFar, antGetZFar, this, nullptr); 
	TwAddVarRW((TwBar*)m_settingsBar, "Camera Dir", TW_TYPE_DIR3D, &m_v3d_cameraDir, nullptr);
	TwAddVarRW((TwBar*)m_settingsBar, "Camera Up", TW_TYPE_DIR3D, &m_v3d_cameraUp, nullptr);
	TwAddVarRW((TwBar*)m_settingsBar, "Look Speed", TW_TYPE_DOUBLE, &m_lookSpeed, nullptr);
	TwAddVarRW((TwBar*)m_settingsBar, "Move Speed", TW_TYPE_DOUBLE, &m_moveSpeed, " min=0.1 step=0.1 ");
}

Camera::~Camera()
{
	TwDeleteBar((TwBar*)m_settingsBar);
}

void Camera::update(const WorldClock& worldClock, double mouseX, double mouseY)
{
	// Update camera
	m_cameraPosition->update(worldClock);

	const double lookDx = m_lookSpeed * (mouseX - GLOBALS.getWindowWidth() / 2);
	const double lookDy = m_lookSpeed * (mouseY - GLOBALS.getWindowHeight() / 2);
	const double lookDYaw =
		(keyPressed('q') || keyPressed('Q')) ?  1000.0 * m_lookSpeed * worldClock.getDt() :
		(keyPressed('e') || keyPressed('E')) ? -1000.0 * m_lookSpeed * worldClock.getDt() :
		0.0
	;

	const glm::dvec3 cameraRight = glm::cross(m_v3d_cameraDir, m_v3d_cameraUp);

	const glm::dquat total = 
		/*glm::normalize*/(glm::angleAxis(-lookDy, cameraRight)) * 
		/*glm::normalize*/(glm::angleAxis(-lookDx, m_v3d_cameraUp)) *
		/*glm::normalize*/(glm::angleAxis(-lookDYaw, m_v3d_cameraDir))
	;

	m_v3d_cameraUp = total * m_v3d_cameraUp;
	m_v3d_cameraDir = total * m_v3d_cameraDir;
	
	const double distMoved = worldClock.getDt() * m_moveSpeed;

	if (keyPressed('w') || keyPressed('W'))
		m_cameraPosition->m_value += m_v3d_cameraDir * distMoved;
	if (keyPressed('s') || keyPressed('S'))
		m_cameraPosition->m_value -= m_v3d_cameraDir * distMoved;
	if (keyPressed('a') || keyPressed('A'))
		m_cameraPosition->m_value -= cameraRight * distMoved;
	if (keyPressed('d') || keyPressed('D'))
		m_cameraPosition->m_value += cameraRight * distMoved;
	if (keyPressed('r') || keyPressed('R'))
		m_cameraPosition->m_value += m_v3d_cameraUp * distMoved;
	if (keyPressed('f') || keyPressed('F'))
		m_cameraPosition->m_value -= m_v3d_cameraUp * distMoved;

	// Set position inverse
	m_absPosition = matrixPosition(m_cameraPosition->getMatrix());

	// Just brute-force update these - don't bother checking dependencies, not worth the effort
	refreshProjectionMatrix();
	refreshViewMatrixes();
	refreshViewProjectionMatrixes();

	m_m4d_invPos = glm::inverse(m_cameraPosition->getMatrix());
}

void Camera::refreshProjectionMatrix()
{
	m_projectionMatrix = glm::perspective(
		getFovY(), 
		(float)GLOBALS.getWindowWidth() / GLOBALS.getWindowHeight(), 
		getZNear(), 
		getZFar()
	);
}

void Camera::refreshViewMatrixes()
{	
	// m_cameraDirection and m_cameraUp are relative to the orientation of the camera,
	// i.e. m_cameraPosition->getMatrix(). 
	const glm::dvec3 v3f_absViewDir(m_cameraPosition->getMatrix() * glm::dvec4(m_v3d_cameraDir, 0.0));
	const glm::dvec3 v3f_absViewUp(m_cameraPosition->getMatrix() * glm::dvec4(m_v3d_cameraUp, 0.0));

	m_zeroViewMatrix = glm::lookAt(glm::vec3(0), glm::vec3(m_v3d_cameraDir), glm::vec3(m_v3d_cameraUp));
	m_absViewMatrix = glm::lookAt(getAbsPosition(), getAbsPosition() + v3f_absViewDir, v3f_absViewUp);
}

void Camera::refreshViewProjectionMatrixes()
{
	m_absViewProjectionMatrix = glm::dmat4(m_projectionMatrix) * m_absViewMatrix;
	m_zeroViewProjectionMatrix = m_projectionMatrix * m_zeroViewMatrix;
}

void Camera::refreshDepthFCoef()
{
	m_f_depthCoef = 2.0f / (logf(m_zFar + 1.0f) / logf(2.0f));
}

void Camera::setFovY(float fovY)
{
	m_fovY = fovY;
	refreshProjectionMatrix();
}

void Camera::setZNear(float zNear)
{
	m_zNear = zNear;
	refreshProjectionMatrix();
}

void Camera::setZFar(float zFar)
{
	m_zFar = zFar;
	refreshDepthFCoef();
	refreshProjectionMatrix();
}

Camera* Camera::buildFromXMLNode(XMLNode& node)
{
	XMLChildFinder finder(node);

	return new Camera(
		finder.required("Name", buildStringFromXMLNode), 
		finder.required("FovY", buildFloatFromXMLNode), 
		finder.required("ZNear", buildFloatFromXMLNode),
		finder.required("ZFar", buildFloatFromXMLNode),
		finder.required("CameraPosition", RelativePosition::buildFromXMLNode), 
		finder.required("CameraDirection", buildDVec3FromXMLNode), 
		finder.required("CameraUp", buildDVec3FromXMLNode), 
		finder.required("LookSpeed", buildDoubleFromXMLNode),
		finder.required("MoveSpeed", buildDoubleFromXMLNode)
	);
}
