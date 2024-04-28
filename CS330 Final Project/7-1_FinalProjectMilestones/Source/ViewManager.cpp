///////////////////////////////////////////////////////////////////////////////
// viewmanager.h
// ============
// manage the viewing of 3D objects within the viewport
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "ViewManager.h"

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>    

// declaration of the global variables and defines
namespace
{
	// Variables for window width and height
	const int WINDOW_WIDTH = 1000;
	const int WINDOW_HEIGHT = 800;
	const char* g_ViewName = "view";
	const char* g_ProjectionName = "projection";

	// camera object used for viewing and interacting with
	// the 3D scene
	Camera* g_pCamera = nullptr;

	// these variables are used for mouse movement processing
	float gLastX = WINDOW_WIDTH / 2.0f;
	float gLastY = WINDOW_HEIGHT / 2.0f;
	bool gFirstMouse = true;
	float yaw = -89.0f;
	float pitch = 0.0f;
	float fov = 45.0f;

	// time between current frame and last frame
	float gDeltaTime = 0.0f; 
	float gLastFrame = 0.0f;

	// the following variable is false when orthographic projection
	// is off and true when it is on
	bool bOrthographicProjection = false;
}

/***********************************************************
 *  ViewManager()
 *
 *  The constructor for the class
 ***********************************************************/
ViewManager::ViewManager(
	ShaderManager *pShaderManager)
{
	// initialize the member variables
	m_pShaderManager = pShaderManager;
	m_pWindow = NULL;
	g_pCamera = new Camera();
	// default camera view parameters
	g_pCamera->Position = glm::vec3(0.0f, 5.0f, 12.0f);
	g_pCamera->Front = glm::vec3(0.0f, -0.5f, -2.0f);
	g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);
	g_pCamera->Zoom = 80;
	g_pCamera->MovementSpeed = 0.1f;
}

/***********************************************************
 *  ~ViewManager()
 *
 *  The destructor for the class
 ***********************************************************/
ViewManager::~ViewManager()
{
	// free up allocated memory
	m_pShaderManager = NULL;
	m_pWindow = NULL;
	if (NULL != g_pCamera)
	{
		delete g_pCamera;
		g_pCamera = NULL;
	}
}

/***********************************************************
 *  Scroll_Callback()
 *  
 *  This method handles scroll wheel input
 ***********************************************************/
void scrollCallback(GLFWwindow* window, double xOffset, double yOffset)
{
	// Controls camera movement speed with scroll wheel
	g_pCamera->MovementSpeed += yOffset * 0.1f;

	// Constrains movement speed to prevent speed being set to 0 or being too fast to control
	if (g_pCamera->MovementSpeed < 0.1f) {
		g_pCamera->MovementSpeed = 0.1f;
	}
	if (g_pCamera->MovementSpeed > 1.0f) {
		g_pCamera->MovementSpeed = 1.0f;
	}
}

/***********************************************************
 *  CreateDisplayWindow()
 *
 *  This method is used to create the main display window.
 ***********************************************************/
GLFWwindow* ViewManager::CreateDisplayWindow(const char* windowTitle)
{
	GLFWwindow* window = nullptr;

	// try to create the displayed OpenGL window
	window = glfwCreateWindow(
		WINDOW_WIDTH,
		WINDOW_HEIGHT,
		windowTitle,
		NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return NULL;
	}
	glfwMakeContextCurrent(window);

	// tell GLFW to capture all mouse events
	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// this callback is used to receive mouse moving events
	glfwSetCursorPosCallback(window, &ViewManager::Mouse_Position_Callback);

	// Receives scroll wheel events
	glfwSetScrollCallback(window, scrollCallback);

	// enable blending for supporting tranparent rendering
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	m_pWindow = window;

	return(window);
}

/***********************************************************
 *  Mouse_Position_Callback()
 *
 *  This method is automatically called from GLFW whenever
 *  the mouse is moved within the active GLFW display window.
 ***********************************************************/
void ViewManager::Mouse_Position_Callback(GLFWwindow* window, double xMousePos, double yMousePos)
{
	// Records first mouse event to correctly calculate X and Y offsets
	if (gFirstMouse)
	{
		gLastX = xMousePos;
		gLastY = yMousePos;
		gFirstMouse = false;
	}

	// Captures mouse cursor within window and hides it
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Calculates X and Y offset values for moving camera
	float xOffset = xMousePos - gLastX;
	float yOffset = gLastY - yMousePos;

	// Sets current position into last position variables
	gLastX = xMousePos;
	gLastY = yMousePos;

	// Controls sensitivity of mouse input
	float sensitivity = 0.1f;
	xOffset *= sensitivity;
	yOffset *= sensitivity;

	// Applies offsets to yaw and pitch
	yaw += xOffset;
	pitch += yOffset;

	// Constrains camera above 89.0 degrees and below -89.0 degress to prevent flipping.
	if (pitch > 89.0f) {
		pitch = 89.0f;
	}
	if (pitch < -89.0f) {
		pitch = -89.0f;
	}

	// Calculates camera direction vector using yaw and pitch
	glm::vec3 direction;
	direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	direction.y = sin(glm::radians(pitch));
	direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	g_pCamera->Front = glm::normalize(direction);
}

/***********************************************************
 *  ProcessKeyboardEvents()
 *
 *  This method is called to process any keyboard events
 *  that may be waiting in the event queue.
 ***********************************************************/
void ViewManager::ProcessKeyboardEvents()
{
	// close the window if the escape key has been pressed
	if (glfwGetKey(m_pWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(m_pWindow, true);
	}

	// Exits method if camera object is null
	if (NULL == g_pCamera)
	{
		return;
	}

	// Moves camera forward or backward when pressing W or S
	if (glfwGetKey(m_pWindow, GLFW_KEY_W) == GLFW_PRESS) {
		g_pCamera->Position += g_pCamera->MovementSpeed * g_pCamera->Front;
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_S) == GLFW_PRESS) {
		g_pCamera->Position -= g_pCamera->MovementSpeed * g_pCamera->Front;
	}

	// Moves camera left or right when pressing A or D
	if (glfwGetKey(m_pWindow, GLFW_KEY_A) == GLFW_PRESS) {
		g_pCamera->Position -= glm::normalize(glm::cross(g_pCamera->Front, g_pCamera->Up)) * g_pCamera->MovementSpeed;
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_D) == GLFW_PRESS) {
		g_pCamera->Position += glm::normalize(glm::cross(g_pCamera->Front, g_pCamera->Up)) * g_pCamera->MovementSpeed;
	}

	// Moves camera up or down when pressing Q or E
	if (glfwGetKey(m_pWindow, GLFW_KEY_Q) == GLFW_PRESS) {
		g_pCamera->Position += g_pCamera->Up * g_pCamera->MovementSpeed;
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_E) == GLFW_PRESS) {
		g_pCamera->Position -= g_pCamera->Up * g_pCamera->MovementSpeed;
	}

	// Switches between perspective or orthographic displays when pressing P or O
	if (glfwGetKey(m_pWindow, GLFW_KEY_P) == GLFW_PRESS) {
		bOrthographicProjection = false;
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_O) == GLFW_PRESS) {
		bOrthographicProjection = true;
	}
}

/***********************************************************
 *  PrepareSceneView()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void ViewManager::PrepareSceneView()
{
	glm::mat4 view;
	glm::mat4 projection;

	// per-frame timing
	float currentFrame = glfwGetTime();
	gDeltaTime = currentFrame - gLastFrame;
	gLastFrame = currentFrame;

	// process any keyboard events that may be waiting in the 
	// event queue
	ProcessKeyboardEvents();

	// get the current view matrix from the camera
	view = g_pCamera->GetViewMatrix();

	// Defines projection matrix based on whether perspective or orthographic view is selected
	if (bOrthographicProjection) {
		float orthoSize = 10.0f;
		projection = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, 0.1f, 100.0f);
	}
	else {
		projection = glm::perspective(glm::radians(g_pCamera->Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
	}

	// if the shader manager object is valid
	if (NULL != m_pShaderManager)
	{
		// set the view matrix into the shader for proper rendering
		m_pShaderManager->setMat4Value(g_ViewName, view);
		// set the view matrix into the shader for proper rendering
		m_pShaderManager->setMat4Value(g_ProjectionName, projection);
		// set the view position of the camera into the shader for proper rendering
		m_pShaderManager->setVec3Value("viewPosition", g_pCamera->Position);
	}
}