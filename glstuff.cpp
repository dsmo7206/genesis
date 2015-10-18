#include "glstuff.h"
#include "globals.h"
#include "utils.h"

void onGLFWError(int error, const char* desc)
{
	fprintf(stderr, desc);
	systemExit(1);
}

void APIENTRY glErrorCallback(
	GLenum source, GLenum type, GLuint id, GLenum severity,
	GLsizei length, const GLchar* message, void* userParam
)
{
	printf("-------- glErrorCallback --------\n Message: %s\n Source: ", (const char*)message);
	switch (source)
	{
		case GL_DEBUG_SOURCE_API:             printf("API"); break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   printf("WINDOW_SYSTEM"); break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER: printf("SHADER_COMPILER"); break;
		case GL_DEBUG_SOURCE_THIRD_PARTY:     printf("THIRD_PARTY"); break;
		case GL_DEBUG_SOURCE_APPLICATION:     printf("APPLICATION"); break;
		case GL_DEBUG_SOURCE_OTHER:           printf("OTHER"); break;
		default:                              printf("UNKNOWN"); break;
	}
	printf(", Type: ");
	switch (type) 
	{
		case GL_DEBUG_TYPE_ERROR:               printf("ERROR"); break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: printf("DEPRECATED_BEHAVIOR"); break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  printf("UNDEFINED_BEHAVIOR"); break;
		case GL_DEBUG_TYPE_PORTABILITY:         printf("PORTABILITY"); break;
		case GL_DEBUG_TYPE_PERFORMANCE:         printf("PERFORMANCE"); break;
		case GL_DEBUG_TYPE_OTHER:               printf("OTHER"); break;
		default:                                printf("UNKNOWN"); break;
	}
	printf(", Id: %u, Severity: ", id);
	switch (severity)
	{
		case GL_DEBUG_SEVERITY_LOW:    printf("LOW"); break;
		case GL_DEBUG_SEVERITY_MEDIUM: printf("MEDIUM"); break;
		case GL_DEBUG_SEVERITY_HIGH:   printf("HIGH"); break;
		default:                       printf("UNKNOWN"); break;
	}
	printf("\n");
}

int initialiseGL()
{
	// Initialise GLFW
	glfwSetErrorCallback(onGLFWError);

	if (!glfwInit())
	{
		fprintf(stderr, "Failed to initialize GLFW\n");
		systemExit(1);
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
#ifdef _DEBUG
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif

	// Open a window and create its OpenGL context - these parameters don't really matter,
	// they are overridden almost immediately when the globals are set.
	appWindow = glfwCreateWindow(
		GLOBALS.getWindowWidth(), GLOBALS.getWindowHeight(),
		GLOBALS.getWindowTitle().c_str(),
		GLOBALS.m_fullScreen ? glfwGetPrimaryMonitor() : NULL,
		NULL
	);
	glfwMakeContextCurrent(appWindow);
	glfwSetInputMode(appWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK)
	{
		fprintf(stderr, "Failed to initialize GLEW\n");
		systemExit(1);
	}

	// After GLEW is initialised, we can use OpenGL functions.

	printf("OpenGL version: %s\n", (const char*)glGetString(GL_VERSION));
	printf("OpenGL vendor:  %s\n", glGetString(GL_VENDOR));

#if _DEBUG
	if (glDebugMessageCallback)
	{
		printf("Registering OpenGL debug callback\n");
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(glErrorCallback, nullptr);
		GLuint unusedIds = 0;
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, &unusedIds, true);
	}
	else
		printf("glDebugMessageCallback not available\n");
#endif

	GLOBALS.setWindowTitle(GLOBALS.getWindowTitle());

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(appWindow, GLFW_STICKY_KEYS, GL_TRUE);
	glfwSetCursorPos(appWindow, GLOBALS.getWindowWidth() / 2, GLOBALS.getWindowHeight() / 2);
	glfwSetWindowSizeCallback(appWindow, setGlobalsWindowSize);

	glClearColor(GLOBALS.m_clearColour.r, GLOBALS.m_clearColour.g, GLOBALS.m_clearColour.b, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	return 0;
}

GLFWwindow* appWindow;

VertexArray::~VertexArray() { if (!GLOBALS.m_shuttingDown) glDeleteVertexArrays(1, &m_id); }
VertexBuffer::~VertexBuffer() { if (!GLOBALS.m_shuttingDown) glDeleteBuffers(1, &m_id); }
FrameBuffer::~FrameBuffer() { if (!GLOBALS.m_shuttingDown) glDeleteFramebuffers(1, &m_id); }
Texture::~Texture() { if (!GLOBALS.m_shuttingDown) glDeleteTextures(1, &m_id); }

glm::vec3 buildFVec3FromXMLNode(XMLNode& node)
{
	XMLChildFinder finder(node);

	return glm::vec3(
		finder.required("X", buildFloatFromXMLNode),
		finder.required("Y", buildFloatFromXMLNode),
		finder.required("Z", buildFloatFromXMLNode)
	);
}

glm::vec4 buildFVec4FromXMLNode(XMLNode& node)
{
	XMLChildFinder finder(node);

	return glm::vec4(
		finder.required("X", buildFloatFromXMLNode),
		finder.required("Y", buildFloatFromXMLNode),
		finder.required("Z", buildFloatFromXMLNode),
		finder.required("W", buildFloatFromXMLNode)
	);
}

glm::dvec3 buildDVec3FromXMLNode(XMLNode& node)
{
	XMLChildFinder finder(node);

	return glm::dvec3(
		finder.required("X", buildDoubleFromXMLNode),
		finder.required("Y", buildDoubleFromXMLNode),
		finder.required("Z", buildDoubleFromXMLNode)
	);
}

glm::dvec4 buildDVec4FromXMLNode(XMLNode& node)
{
	XMLChildFinder finder(node);

	return glm::dvec4(
		finder.required("X", buildDoubleFromXMLNode),
		finder.required("Y", buildDoubleFromXMLNode),
		finder.required("Z", buildDoubleFromXMLNode),
		finder.required("W", buildDoubleFromXMLNode)
	);
}



