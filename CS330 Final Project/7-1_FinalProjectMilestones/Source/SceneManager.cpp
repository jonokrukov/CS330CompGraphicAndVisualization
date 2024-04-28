///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();

	// initialize the texture collection
	for (int i = 0; i < 16; i++)
	{
		m_textureIDs[i].tag = "/0";
		m_textureIDs[i].ID = -1;
	}
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;

	// destroy the created OpenGL textures
	DestroyGLTextures();
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		//m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/***********************************************************
 *  LoadSceneTextures()
 *
 * 
 *  This method loads textures into memory
 ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	bool bReturn = false;

	// Load textures into memory and assign associated shape
	bReturn = CreateGLTexture("ceramicTexture.jpg", "mug");
	bReturn = CreateGLTexture("stoneTexture.jpg", "table");
	bReturn = CreateGLTexture("blackPlasticTexture.jpg", "blackPlastic");
	bReturn = CreateGLTexture("whitePlasticTexture.jpg", "whitePlastic");
	bReturn = CreateGLTexture("bluePlasticTexture.jpg", "bluePlastic");
	bReturn = CreateGLTexture("redPaperTexture.jpg", "redPaper");
	bReturn = CreateGLTexture("blackBookTexture.jpg", "blackBook");
	bReturn = CreateGLTexture("brownBookTexture.jpg", "brownBook");

	// Bind loaded textures to texture slots
	BindGLTextures();
}

/***********************************************************
 *  DefineObjectMaterials()
 *  
 *  This method defines and configures materials for 3D objects
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	// Ceramic Material
	OBJECT_MATERIAL ceramicMaterial;
	ceramicMaterial.ambientColor = glm::vec3(0.7f, 0.7f, 0.7f);
	ceramicMaterial.ambientStrength = 0.05f;
	ceramicMaterial.diffuseColor = glm::vec3(0.7f, 0.7f, 0.7f);
	ceramicMaterial.specularColor = glm::vec3(0.8f, 0.8f, 0.8f);
	ceramicMaterial.shininess = 4.0;
	ceramicMaterial.tag = "ceramic";

	// Add ceramic material to list of object materials
	m_objectMaterials.push_back(ceramicMaterial);

	// Marble Material
	OBJECT_MATERIAL marbleMaterial;
	marbleMaterial.ambientColor = glm::vec3(0.05f, 0.05f, 0.05f);
	marbleMaterial.ambientStrength = 0.1f;
	marbleMaterial.diffuseColor = glm::vec3(0.1f, 0.1f, 0.1f);
	marbleMaterial.specularColor = glm::vec3(0.7f, 0.7f, 0.7f);
	marbleMaterial.shininess = 20.0;
	marbleMaterial.tag = "marble";

	// Add marble material to list of object materials
	m_objectMaterials.push_back(marbleMaterial);

	// Paper Material
	OBJECT_MATERIAL paperMaterial;
	paperMaterial.ambientColor = glm::vec3(0.7f, 0.7f, 0.65f);
	paperMaterial.ambientStrength = 0.1f;
	paperMaterial.diffuseColor = glm::vec3(1.0f, 1.0f, 0.9f);
	paperMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);
	paperMaterial.shininess = 2.0;
	paperMaterial.tag = "paper";

	// Add paper material to list of object materials
	m_objectMaterials.push_back(paperMaterial);

	// Plastic Material
	OBJECT_MATERIAL plasticMaterial;
	plasticMaterial.ambientColor = glm::vec3(0.05f, 0.05f, 0.05f);
	plasticMaterial.ambientStrength = 0.1f;
	plasticMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.4f);
	plasticMaterial.specularColor = glm::vec3(0.7f, 0.7f, 0.7f);
	plasticMaterial.shininess = 60.0;
	plasticMaterial.tag = "plastic";

	// Add plastic material to list of object materials
	m_objectMaterials.push_back(plasticMaterial);

	// Dull Plastic Material
	OBJECT_MATERIAL dullPlasticMaterial;
	dullPlasticMaterial.ambientColor = glm::vec3(0.05f, 0.05f, 0.05f);
	dullPlasticMaterial.ambientStrength = 0.1f;
	dullPlasticMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.4f);
	dullPlasticMaterial.specularColor = glm::vec3(0.7f, 0.7f, 0.7f);
	dullPlasticMaterial.shininess = 20.0;
	dullPlasticMaterial.tag = "dullPlastic";

	// Add plastic material to list of object materials
	m_objectMaterials.push_back(dullPlasticMaterial);

	// Glass Material
	OBJECT_MATERIAL glassMaterial;
	glassMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	glassMaterial.ambientStrength = 0.1f;
	glassMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f);
	glassMaterial.specularColor = glm::vec3(0.9f, 0.9f, 0.9f);
	glassMaterial.shininess = 100.0;
	glassMaterial.tag = "glass";

	// Add glass material to list of object materials
	m_objectMaterials.push_back(glassMaterial);
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene. There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// Primary sunlight from back left window
	m_pShaderManager->setVec3Value("lightSources[0].position", -20.0f, 15.0f, -16.5f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 1.0f, 0.95f, 0.9f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 1.0f, 0.95f, 0.9f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 10.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.2f);
	
	// Secondary softer light from back left window
	m_pShaderManager->setVec3Value("lightSources[1].position", -20.0f, 6.0f, -16.5f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.8f, 0.75f, 0.7f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.5f, 0.5f, 0.5f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 0.01f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.0f);

	// Primary sunlight from back right window
	m_pShaderManager->setVec3Value("lightSources[2].position", 20.0f, 15.0f, -16.5f);
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", 1.0f, 0.95f, 0.9f);
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", 1.0f, 0.95f, 0.9f);
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 10.0f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.2f);

	// Secondary softer light from back right window
	m_pShaderManager->setVec3Value("lightSources[3].position", 20.0f, 6.0f, -16.5f);
	m_pShaderManager->setVec3Value("lightSources[3].ambientColor", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("lightSources[3].diffuseColor", 0.8f, 0.75f, 0.7f);
	m_pShaderManager->setVec3Value("lightSources[3].specularColor", 0.5f, 0.5f, 0.5f);
	m_pShaderManager->setFloatValue("lightSources[3].focalStrength", 0.01f);
	m_pShaderManager->setFloatValue("lightSources[3].specularIntensity", 0.0f);

	// Enable lighting
	m_pShaderManager->setBoolValue("bUseLighting", true);
}


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// Load textures for scene
	LoadSceneTextures();

	// Define materials for objects in 3D scene
	DefineObjectMaterials();

	// Add and configure light sources for 3D scene
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();

	// Load needed shapes into memory
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadBoxMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/

	/*************************** Table Plane Code *************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(10.0f, 1.0f, 9.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 1.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply texture associated with tag to shape
	SetShaderTexture("table");

	// Set active shader material based on tag
	SetShaderMaterial("marble");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	/*************************** Mug Bottom Tapered Cylinder Code *************************************/

	// set the XYZ scale for the tapered cylinder
	scaleXYZ = glm::vec3(1.0f, 0.8f, 1.0f);

	// set the XYZ rotation for the tapered cylinder
	XrotationDegrees = 180.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the tapered cylinder
	positionXYZ = glm::vec3(4.0f, 1.8f, -1.0f);

	// set the transformations into memory to be used on the drawn tapered cylinder
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply texture associated with tag to shape
	SetShaderTexture("mug");

	// Set active shader material based on tag
	SetShaderMaterial("ceramic");

	// draw the tapered cylinder with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();

	/*************************** Mug Handle Torus Code *************************************/

	// set the XYZ scale for the torus
	scaleXYZ = glm::vec3(0.6f, 0.7f, 1.0f);

	// set the XYZ rotation for the torus
	XrotationDegrees = 180.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the torus
	positionXYZ = glm::vec3(5.0f, 2.4f, -1.0f);

	// set the transformations into memory to be used on the drawn torus
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply texture associated with tag to shape
	SetShaderTexture("mug");

	// Set active shader material based on tag
	SetShaderMaterial("ceramic");

	// draw the torus with transformation values
	m_basicMeshes->DrawTorusMesh();

	/*************************** Mug Cylinder Code *************************************/

	// set the XYZ scale for the cylinder
	scaleXYZ = glm::vec3(1.0f, 1.8f, 1.0f);

	// set the XYZ rotation for the cylinder
	XrotationDegrees = 180.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the cylinder
	positionXYZ = glm::vec3(4.0f, 3.6f, -1.0f);

	// set the transformations into memory to be used on the drawn cylinder
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply texture associated with tag to shape
	SetShaderTexture("mug");

	// Set active shader material based on tag
	SetShaderMaterial("ceramic");

	// draw the cylinder with transformation values
	m_basicMeshes->DrawCylinderMesh();

	/*************************** Blue Book Box Code *************************************/

	// set the XYZ scale for box
	scaleXYZ = glm::vec3(6.0f, 0.275f, 3.75f);

	// set the XYZ rotation for box
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for box
	positionXYZ = glm::vec3(0.0f, 1.1f, 1.0f);

	// set the transformations into memory to be used on the box
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply texture associated with tag to shape
	SetShaderTexture("bluePlastic");

	// Set active shader material based on tag
	SetShaderMaterial("dullPlastic");

	// draw the box with transformation values
	m_basicMeshes->DrawBoxMesh();

	/*************************** Bottom Brown Book Box Code *************************************/

	// set the XYZ scale for box
	scaleXYZ = glm::vec3(6.4f, 0.6f, 3.75f);

	// set the XYZ rotation for box
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for box
	positionXYZ = glm::vec3(-2.0f, 1.2f, -4.4f);

	// set the transformations into memory to be used on the box
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply texture associated with tag to shape
	SetShaderTexture("brownBook");

	// Set active shader material based on tag
	SetShaderMaterial("paper");

	// draw the box with transformation values
	m_basicMeshes->DrawBoxMesh();

	/*************************** Middle Black Book Box Code *************************************/

	// set the XYZ scale for box
	scaleXYZ = glm::vec3(5.7f, 0.5f, 3.25f);

	// set the XYZ rotation for box
	XrotationDegrees = 0.0f;
	YrotationDegrees = -10.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for box
	positionXYZ = glm::vec3(-2.2f, 1.7f, -4.4f);

	// set the transformations into memory to be used on the box
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply texture associated with tag to shape
	SetShaderTexture("blackBook");

	// Set active shader material based on tag
	SetShaderMaterial("paper");

	// draw the box with transformation values
	m_basicMeshes->DrawBoxMesh();

	/*************************** Bottom Black Book Box Code *************************************/

	// set the XYZ scale for box
	scaleXYZ = glm::vec3(5.7f, 0.5f, 3.25f);

	// set the XYZ rotation for box
	XrotationDegrees = 0.0f;
	YrotationDegrees = 20.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for box
	positionXYZ = glm::vec3(-2.2f, 2.2f, -4.4f);

	// set the transformations into memory to be used on the box
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply texture associated with tag to shape
	SetShaderTexture("blackBook");

	// Set active shader material based on tag
	SetShaderMaterial("paper");

	// draw the box with transformation values
	m_basicMeshes->DrawBoxMesh();

	/*************************** Trail Mix Container Box Code *************************************/

	// set the XYZ scale for box
	scaleXYZ = glm::vec3(2.0f, 2.7f, 2.0f);

	// set the XYZ rotation for box
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for box
	positionXYZ = glm::vec3(-4.0f, 2.0f, -0.5f);

	// set the transformations into memory to be used on the box
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply texture associated with tag to shape
	SetShaderTexture("redPaper");

	// Set active shader material based on tag
	SetShaderMaterial("paper");

	// draw the box with transformation values
	m_basicMeshes->DrawBoxMesh();

	/*************************** Trail Mix Lid Cylinder Code *************************************/

	// set the XYZ scale for cylinder
	scaleXYZ = glm::vec3(1.09f, 0.4f, 1.09f);

	// set the XYZ rotation for cylinder
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for cylinder
	positionXYZ = glm::vec3(-4.0f, 3.35f, -0.5f);

	// set the transformations into memory to be used on the cylinder
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply texture associated with tag to shape
	SetShaderTexture("blackPlastic");

	// Set active shader material based on tag
	SetShaderMaterial("plastic");

	// draw the cylinder with transformation values
	m_basicMeshes->DrawCylinderMesh();

	/*************************** Main Pen Cylinder Code *************************************/

	// set the XYZ scale for cylinder
	scaleXYZ = glm::vec3(0.05f, 2.0f, 0.05f);

	// set the XYZ rotation for cylinder
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 64.0f;

	// set the XYZ position for cylinder
	positionXYZ = glm::vec3(0.9f, 1.33f, 1.0f);

	// set the transformations into memory to be used on the cylinder
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply texture associated with tag to shape
	SetShaderTexture("blackPlastic");

	// Set active shader material based on tag
	SetShaderMaterial("plastic");

	// draw the cylinder with transformation values
	m_basicMeshes->DrawCylinderMesh();

	/*************************** Pen Tip Cone Code *************************************/

	// set the XYZ scale for cone
	scaleXYZ = glm::vec3(0.05f, 0.12f, 0.05f);

	// set the XYZ rotation for cone
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 64.0f;

	// set the XYZ position for cone
	positionXYZ = glm::vec3(-0.9f, 1.33f, 1.877f);

	// set the transformations into memory to be used on the cone
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply texture associated with tag to shape
	SetShaderTexture("blackPlastic");

	// Set active shader material based on tag
	SetShaderMaterial("plastic");

	// draw the cone with transformation values
	m_basicMeshes->DrawConeMesh();

	/*************************** Pen Top Tapered Cylinder Code *************************************/

	// set the XYZ scale for tapered cylinder
	scaleXYZ = glm::vec3(0.05f, 0.09f, 0.05f);

	// set the XYZ rotation for tapered cylinder
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 244.0f;

	// set the XYZ position for tapered cylinder
	positionXYZ = glm::vec3(0.90f, 1.33f, 1.0f);

	// set the transformations into memory to be used on the tapered cylinder
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply texture associated with tag to shape
	SetShaderTexture("blackPlastic");

	// Set active shader material based on tag
	SetShaderMaterial("plastic");

	// draw the tapered cylinder with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();

	/*************************** Back Left Window Plane Code *************************************/
	// set the XYZ scale for window
	scaleXYZ = glm::vec3(6.0f, 1.0f, 9.0f);

	// set the XYZ rotation for window
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for window
	positionXYZ = glm::vec3(-20.0f, 6.0f, -17.0f);

	// set the transformations into memory to be used on window
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	
	// Apply texture associated with tag to shape
	SetShaderTexture("whitePlastic");

	// Set active shader material based on tag
	SetShaderMaterial("plastic");

	// draw window with transformation values
	m_basicMeshes->DrawPlaneMesh();

	/*************************** Back Right Window Plane Code *************************************/
	// set the XYZ scale for the light source
	scaleXYZ = glm::vec3(6.0f, 1.0f, 9.0f);

	// set the XYZ rotation for the light source
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the light source
	positionXYZ = glm::vec3(20.0f, 6.0f, -17.0f);

	// set the transformations into memory to be used on the light source
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// draw window with transformation values
	m_basicMeshes->DrawPlaneMesh();
}


