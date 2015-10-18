#pragma once

// Include GLEW
#define GLEW_STATIC
#include <GL/glew.h>
 
// Include GLFW
#include <GLFW/glfw3.h>
 
// Include GLM
#define GLM_FORCE_RADIANS
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/quaternion.hpp>
#include <gtc/type_ptr.hpp>
#include <gtx/quaternion.hpp>
#include <gtx/euler_angles.hpp>
#include <gtx/norm.hpp>

#include <vector>
#include <string>
#include "xml.h"

int initialiseGL();

extern GLFWwindow* appWindow;

inline bool keyPressed(char key) { return glfwGetKey(appWindow, key) == GLFW_PRESS; }

// RAII wrappers around simple OpenGL objects

inline GLuint makeVertexArray() { GLuint id; glGenVertexArrays(1, &id); return id; }
inline GLuint makeBuffer()      { GLuint id; glGenBuffers(1, &id);      return id; }
inline GLuint makeFrameBuffer() { GLuint id; glGenFramebuffers(1, &id); return id; }
inline GLuint makeTexture()     { GLuint id; glGenTextures(1, &id);     return id; }

struct VertexArray  { const GLuint m_id; VertexArray()  : m_id(makeVertexArray()) {} ~VertexArray(); };
struct VertexBuffer { const GLuint m_id; VertexBuffer() : m_id(makeBuffer()) {}      ~VertexBuffer(); };
struct FrameBuffer  { const GLuint m_id; FrameBuffer()  : m_id(makeFrameBuffer()) {} ~FrameBuffer(); };
struct Texture      
{ 
	const GLuint m_id; 
	
	Texture() : m_id(makeTexture()) 
	{
	}

	Texture(GLenum textureType) : m_id(makeTexture())
	{
		glBindTexture(textureType, m_id);
	}

	~Texture(); 
};

template <typename T>
inline T* copyBufferToMemory(GLenum target, GLuint id, int count)
{
	T* data = new T[count];
	glBindBuffer(target, id);
	glGetBufferSubData(target, 0, count*sizeof(T), (GLvoid*)data);
	return data;
}

glm::vec3 buildFVec3FromXMLNode(XMLNode& node);
glm::vec4 buildFVec4FromXMLNode(XMLNode& node);
glm::dvec3 buildDVec3FromXMLNode(XMLNode& node);
glm::dvec4 buildDVec4FromXMLNode(XMLNode& node);

inline void setUniformMatrixm4f(GLuint locationId, const glm::mat4& mat)
{
	glUniformMatrix4fv(locationId, 1, GL_FALSE, glm::value_ptr(mat));
}

