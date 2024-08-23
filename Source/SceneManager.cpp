///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
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

	modelView = translation * rotationZ * rotationY * rotationX * scale;

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
		m_pShaderManager->setIntValue(g_UseTextureName, false);
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

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

// added method to load textures for the 3D scene
void SceneManager::LoadSceneTexture()
{
	bool bReturn = false;

	// try to load in ground path image for the center of the ground
	bReturn = CreateGLTexture("textures/path.jpg", "path");

	// try to load in grass image for the ground
	bReturn = CreateGLTexture("textures/grass.png", "grass");

	// try to load in elephant skin image for the elephant
	bReturn = CreateGLTexture("textures/elephant-skin-texture.jpg", "skin");

	// try to load in bark image for the tree
	bReturn = CreateGLTexture("textures/bark-texture.jpg", "bark");

	// try to load in leaves image for the tree
	bReturn = CreateGLTexture("textures/leaves.png", "leaves");

	// try to load in water image for the pond
	bReturn = CreateGLTexture("textures/water.jpg", "pond");

	// try to load in giraffe skin image for the giraffe
	bReturn = CreateGLTexture("textures/giraffe-skin.jpg", "giraffe");

	// try to load in cloud image for the sky
	bReturn = CreateGLTexture("textures/cloud.jpg", "sky");

	// try to load in rock image for the rocks
	bReturn = CreateGLTexture("textures/rock.jpg", "rock");

	// bound textures to texture slots
	BindGLTextures();
}

// method for adjusting lighting based on an object's material
void SceneManager::DefineObjectMaterials()
{
	// shiny material for objects that reflect light well
	OBJECT_MATERIAL shinyMaterial;
	shinyMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	shinyMaterial.ambientStrength = 0.4f;
	shinyMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.2f);
	shinyMaterial.specularColor = glm::vec3(0.3f, 0.3f, 0.3f);
	shinyMaterial.shininess = 12.0;
	shinyMaterial.tag = "shiny";

	m_objectMaterials.push_back(shinyMaterial);

	// grass material for slightly dimmed ground areas
	OBJECT_MATERIAL grassMaterial;
	grassMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.1f);
	grassMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
	grassMaterial.shininess = 0.1;
	grassMaterial.tag = "grass";

	m_objectMaterials.push_back(grassMaterial);

	// wood material for the trees
	OBJECT_MATERIAL woodMaterial;
	woodMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	woodMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.shininess = 0.3;
	woodMaterial.tag = "wood";

	m_objectMaterials.push_back(woodMaterial);

	// sun material for the sun
	OBJECT_MATERIAL sunMaterial;
	sunMaterial.ambientColor = glm::vec3(1.8f, 1.8f, 1.8f);
	sunMaterial.ambientStrength = 15.0f;
	sunMaterial.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
	sunMaterial.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
	sunMaterial.shininess = 15.0;
	sunMaterial.tag = "sun";

	m_objectMaterials.push_back(sunMaterial);

	// material for spotlight parts of objects
	OBJECT_MATERIAL roughMaterial;
	roughMaterial.ambientColor = glm::vec3(0.8f, 0.8f, 0.8f);
	roughMaterial.ambientStrength = 1.0f;
	roughMaterial.diffuseColor = glm::vec3(0.8f, 0.5f, 0.3f);
	roughMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);
	roughMaterial.shininess = 0.5;
	roughMaterial.tag = "rough";

	m_objectMaterials.push_back(roughMaterial);

	// material for darkening areas that are further from light source
	OBJECT_MATERIAL shadeMaterial;
	shadeMaterial.ambientColor = glm::vec3(0.02f, 0.0f, 0.0f);
	shadeMaterial.ambientStrength = 7.0f;
	shadeMaterial.diffuseColor = glm::vec3(0.08f, 0.08f, 0.08f);
	shadeMaterial.specularColor = glm::vec3(0.02f, 0.02f, 0.02f);
	shadeMaterial.shininess = 0.1;
	shadeMaterial.tag = "shade";

	m_objectMaterials.push_back(shadeMaterial);
}

// method for adding light sources throughout the scene
void SceneManager::SetupSceneLights()
{
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	m_pShaderManager->setVec3Value("globalAmbientColor", .20, .20, .00);

	// 1st light source
	m_pShaderManager->setVec3Value("lightSources[0].position", 35.0f, 32.0f, -1.0f);
	m_pShaderManager->setVec3Value("lightSorces[0].ambientColor", 1.5f, 1.5f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 2.8f, 2.8f, 2.8f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 5.5f, 5.5f, 5.5f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 10.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 2.5f);

	// 2nd light source
	m_pShaderManager->setVec3Value("lightSources[1].position", 19.8f, 18.0f, -9.5f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 0.1f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.1f);

	// 3rd light source
	m_pShaderManager->setVec3Value("lightSources[2].position", 0.0f, 10.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", 0.6f, 0.6f, 0.6f);
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 2.0f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 3.0f);

	// 4th light source
	m_pShaderManager->setVec3Value("lightSources[3].position", -4.0f, 6.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[3].diffuseColor", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[3].specularColor", 0.4f, 0.4f, 0.4f);
	m_pShaderManager->setFloatValue("lightSources[3].focalStrength", 5.0f);
	m_pShaderManager->setFloatValue("lightSources[3].specularIntensity", 1.8f);
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
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	// load in the textures from the stored image files
	LoadSceneTexture();

	// define object materials
	DefineObjectMaterials();

	// add light sources
	SetupSceneLights();

	m_basicMeshes->LoadPlaneMesh();

	// Loaded sphere, cylinder, and cone meshes for complex objects
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadPrismMesh();
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
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	// rotated the plane 90 degrees
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// changed texture of the center plane
	SetShaderTexture("path");
	// set lighting material for the path
	SetShaderMaterial("grass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

	// set the XYZ scale for the plane on the right
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the plane on the right
	XrotationDegrees = 0.0f;
	// rotated the plane 90 degrees
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the plane on the right
	positionXYZ = glm::vec3(20.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// changed texture of the right plane
	SetShaderTexture("grass");
	// set a lighting material
	SetShaderMaterial("grass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

	// set the XYZ scale for the plane on the left
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the plane on the left
	XrotationDegrees = 0.0f;
	// rotated the plane 90 degrees
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the plane on the left
	positionXYZ = glm::vec3(-20.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// changed texture of the left plane
	SetShaderTexture("grass");
	// set a lighting material for directional lighting
	SetShaderMaterial("shade");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

	// set the XYZ scale for the background plane
	scaleXYZ = glm::vec3(30.0f, 1.0f, 20.0f);

	// set the XYZ rotation for the background plane
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the background plane
	positionXYZ = glm::vec3(0.0f, 20.0f, -20.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// changed texture of the background plane
	SetShaderTexture("sky");
	// set a lighting material for directional lighting
	SetShaderMaterial("grass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

	// create elephant object
	RenderElephant();
	// create baby elephant objects
	RenderBabyElephant();
	// create Tree objects
	RenderTree();
	// create a sun object
	RenderSun();
	// create pond object
	RenderPond();
	// create giraffe object
	RenderGiraffe();
	// create rock objects
	RenderRocks();
	
}

// method for creating an elephant object
void SceneManager::RenderElephant()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// start creation of elephant legs using four cylinders

	// set the XYZ scale of front right leg
	scaleXYZ = glm::vec3(1.0f, 3.0f, 0.7f);

	// set the XYZ rotation of front right leg
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of front right leg
	positionXYZ = glm::vec3(3.0f, 0.1f, 2.0f);

	// set the transformation for the front right leg
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the front right leg
	SetShaderTexture("skin");
	// set the rough lighting material
	SetShaderMaterial("rough");

	// draw the first leg
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	// second elephant leg

	// set the XYZ scale of back right leg
	scaleXYZ = glm::vec3(1.0f, 3.0f, 0.7f);

	// set the XYZ rotation of back right leg
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of back right leg
	positionXYZ = glm::vec3(-3.0f, 0.1f, 2.0f);

	// set the transformation for the back right leg
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the back right leg
	SetShaderTexture("skin");
	// set the rough lighting material
	SetShaderMaterial("rough");

	// draw the second leg
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	// third elephant leg

	// set the XYZ scale of front left leg
	scaleXYZ = glm::vec3(1.0f, 3.0f, 0.7f);

	// set the XYZ rotation of front left leg
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of front left leg
	positionXYZ = glm::vec3(3.0f, 0.1f, -2.0f);

	// set the transformation for the front left leg
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the front left leg
	SetShaderTexture("skin");
	// set the rough lighting material
	SetShaderMaterial("rough");

	// draw the third leg
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	// start fourth elephant leg

	// set the XYZ scale of back left leg
	scaleXYZ = glm::vec3(1.0f, 3.0f, 0.7f);

	// set the XYZ rotation of back left leg
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of back left leg
	positionXYZ = glm::vec3(-3.0f, 0.1f, -2.0f);

	// set the transformation for the back left leg
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the back left leg
	SetShaderTexture("skin");
	// et the rough lighting material
	SetShaderMaterial("rough");

	// draw the fourth leg
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	// start the creation of the elephant's body using a sphere

	// set the XYZ scale of the elephant's body
	scaleXYZ = glm::vec3(6.0f, 3.5f, 4.8f);

	// set the XYZ rotation of the elephants's body
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the elephant's body
	positionXYZ = glm::vec3(0.0f, 5.0f, 0.0f);

	// set the transformation of the elephant's body
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the elephant's body
	SetShaderTexture("skin");
	// set the rough lighting material
	SetShaderMaterial("rough");

	// draw the elephant's body
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// start the creation of the elephant's head using a sphere

	// set the XYZ scale of the elephant's head
	scaleXYZ = glm::vec3(2.2f, 2.6f, 2.7f);

	// set the XYZ rotation of the elephant's head
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the elephant's head
	positionXYZ = glm::vec3(7.5f, 6.0f, 0.0f);

	// set the transformations of the elephant's head
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the elephant's head
	SetShaderTexture("skin");
	// set the rough lighting material
	SetShaderMaterial("rough");

	// draw the elephant's head
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// start the creation of the elephant's trunk using a tapered cylinder

	// set the XYZ scale of the elephant's trunk
	scaleXYZ = glm::vec3(1.0f, 5.8f, 1.0f);

	// set the XYZ rotation of the elephant's trunk
	XrotationDegrees = 0.0f;
	YrotationDegrees = 10.0f;
	ZrotationDegrees = 188.0f;

	// set the XYZ position of the elephant's trunk
	positionXYZ = glm::vec3(8.5f, 6.0f, 0.0f);

	// set the transformations of the elephant's trunk
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the elephant's trunk
	SetShaderTexture("skin");
	// set the rough lighting material
	SetShaderMaterial("rough");

	// draw the elephant's trunk
	m_basicMeshes->DrawTaperedCylinderMesh();
	/****************************************************************/

	// start the creation of the elephant's ears using sphere shapes

	// set the XYZ scale of the right ear
	scaleXYZ = glm::vec3(0.7f, 1.7f, 1.5f);

	// set the XYZ rotation of the right ear
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the right ear
	positionXYZ = glm::vec3(7.4f, 6.5f, 3.0f);

	// set the transformations of the elephant's right ear
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the elephant's right ear
	SetShaderTexture("skin");
	// set the rough lighting material
	SetShaderMaterial("rough");

	// draw the elephant's right ear
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// start the creation of the elephant's left ear

	// set the XYZ scale of the left ear
	scaleXYZ = glm::vec3(0.7f, 1.7f, 1.5f);

	// set the XYZ rotation of the left ear
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the left ear
	positionXYZ = glm::vec3(7.4f, 6.5f, -3.0f);

	// set the transformations of the elephant's left ear
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the elephant's left ear
	SetShaderTexture("skin");
	// set the rough lighting material
	SetShaderMaterial("rough");

	// draw the elephant's left ear
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// start creation of the elephant's eyes using sphere shapes

	// set the XYZ scale of the elephant's right eye
	scaleXYZ = glm::vec3(0.3f, 0.4f, 0.3f);

	// set the XYZ rotation of the elephant's right eye
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the elephant's right eye
	positionXYZ = glm::vec3(9.0f, 7.0f, 1.5f);

	// set the transformations of the elephant's right eye
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color of the elephant's right eye
	SetShaderColor(1, 1, 1, 1);

	// draw the elephant's right eye
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// start creation of the elephant's left eye

	// set the XYZ scale of the elephant's left eye
	scaleXYZ = glm::vec3(0.3f, 0.4f, 0.3f);

	// set the XYZ rotation of the elephant's left eye
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the elephant's left eye
	positionXYZ = glm::vec3(9.0f, 7.0f, -1.5f);

	// set the transformations of the elephant's left eye
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color of the elephant's left eye
	SetShaderColor(1, 1, 1, 1);

	// draw the elephant's left eye
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// start creation of the elephant's tusks using cone shapes

	// set the XYZ scale of the elephant's right tusk
	scaleXYZ = glm::vec3(0.3f, 2.9f, 0.5f);

	// set the XYZ rotation of the elephant's right tusk
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -135.0f;

	// set the XYZ position of the elephant's right tusk
	positionXYZ = glm::vec3(8.5f, 4.2f, 1.1f);

	// set the transformations of the elephant's right tusk
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color of the elephant's right tusk
	SetShaderColor(1, 1, 1, 1);

	// draw the elephant's right tusk
	m_basicMeshes->DrawConeMesh();
	/****************************************************************/

	// start creation of the elephant's left tusk

	// set the XYZ scale of the elephant's left tusk
	scaleXYZ = glm::vec3(0.3f, 2.9f, 0.5f);

	// set the XYZ rotation of the elephant's left tusk
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -135.0f;

	// set the XYZ position of the elephant's left tusk
	positionXYZ = glm::vec3(8.5f, 4.2f, -1.1f);

	// set the transformations of the elephant's left tusk
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color of the elephant's left tusk
	SetShaderColor(1, 1, 1, 1);

	// draw the elephant's left tusk
	m_basicMeshes->DrawConeMesh();
	/****************************************************************/

	// start creation of the elephant's tail using a tapered cylinder

	// set the XYZ scale of the elephant's tail
	scaleXYZ = glm::vec3(0.2f, 3.9f, 0.3f);

	// set the XYZ rotation of the elephant's tail
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 168.0f;

	// set the XYZ position of the elephant's tail
	positionXYZ = glm::vec3(-5.6f, 6.0f, 0.0f);

	// set the transformations of the elephant's tail
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the elephant's tail
	SetShaderTexture("skin");
	// set the shade lighting material
	SetShaderMaterial("shade");

	// draw the elephant's tail
	m_basicMeshes->DrawTaperedCylinderMesh();
	/****************************************************************/

	// start creation of the elephant's nails

	// set the XYZ scale of the elephant's left nail for the front right leg
	scaleXYZ = glm::vec3(0.2f, 0.4f, 0.2f);

	// set the XYZ rotation of the elephant's left nail for the front right leg
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the elephant's left nail for the front right leg
	positionXYZ = glm::vec3(3.9f, 0.4f, 1.7f);

	// set the transformations of the elephant's left nail for the front right leg
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color of the elephant's nails
	SetShaderColor(1, 1, 1, 1);

	// draw the nail on the left of the elephant's front right leg
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// set the XYZ scale of the elephant's right nail for the front right leg
	scaleXYZ = glm::vec3(0.2f, 0.4f, 0.2f);

	// set the XYZ rotation of the elephant's right nail for the front right leg
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the elephant's right nail for the front right leg
	positionXYZ = glm::vec3(3.9f, 0.4f, 2.2f);

	// set the transformations of the elephant's right nail for the front right leg
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color of the elephant's nails
	SetShaderColor(1, 1, 1, 1);

	// draw the nail on the right of the elephant's front right leg
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// set the XYZ scale of the elephant's right nail for the front left leg
	scaleXYZ = glm::vec3(0.2f, 0.4f, 0.2f);

	// set the XYZ rotation of the elephant's right nail for the front left leg
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the elephant's right nail for the front left leg
	positionXYZ = glm::vec3(3.9f, 0.4f, -1.7f);

	// set the transformations of the elephant's right nail for the front left leg
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color of the elephant's nails
	SetShaderColor(1, 1, 1, 1);

	// draw the nail on the right of the elephant's front left leg
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// set the XYZ scale of the elephant's left nail for the front left leg
	scaleXYZ = glm::vec3(0.2f, 0.4f, 0.2f);

	// set the XYZ rotation of the elephant's left nail for the front left leg
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the elephant's left nail for the front left leg
	positionXYZ = glm::vec3(3.9f, 0.4f, -2.2f);

	// set the transformations of the elephant's left nail for the front left leg
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color of the elephant's nails
	SetShaderColor(1, 1, 1, 1);

	// draw the nail on the left of the elephant's front left leg
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// set the XYZ scale of the elephant's left nail for the back right leg
	scaleXYZ = glm::vec3(0.2f, 0.4f, 0.2f);

	// set the XYZ rotation of the elephant's left nail for the back right leg
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the elephant's left nail for the back right leg
	positionXYZ = glm::vec3(-2.1f, 0.4f, 1.7f);

	// set the transformations of the elephant's left nail for the back right leg
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color of the elephant's nails
	SetShaderColor(1, 1, 1, 1);

	// draw the nail on the left of the elephant's back right leg
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// set the XYZ scale of the elephant's right nail for the back right leg
	scaleXYZ = glm::vec3(0.2f, 0.4f, 0.2f);

	// set the XYZ rotation of the elephant's right nail for the back right leg
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the elephant's right nail for the back right leg
	positionXYZ = glm::vec3(-2.1f, 0.4f, 2.2f);

	// set the transformations of the elephant's right nail for the back right leg
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color of the elephant's nails
	SetShaderColor(1, 1, 1, 1);

	// draw the nail on the right of the elephant's back right leg
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// set the XYZ scale of the elephant's left nail for the back left leg
	scaleXYZ = glm::vec3(0.2f, 0.4f, 0.2f);

	// set the XYZ rotation of the elephant's left nail for the back left leg
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the elephant's left nail for the back left leg
	positionXYZ = glm::vec3(-2.1f, 0.4f, -2.2f);

	// set the transformations of the elephant's left nail for the back left leg
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color of the elephant's nails
	SetShaderColor(1, 1, 1, 1);

	// draw the nail on the left of the elephant's back left leg
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// set the XYZ scale of the elephant's right nail for the front left leg
	scaleXYZ = glm::vec3(0.2f, 0.4f, 0.2f);

	// set the XYZ rotation of the elephant's right nail for the front left leg
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the elephant's right nail for the front left leg
	positionXYZ = glm::vec3(-2.1f, 0.4f, -1.7f);

	// set the transformations of the elephant's right nail for the front left leg
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color of the elephant's nails
	SetShaderColor(1, 1, 1, 1);

	// draw the nail on the right of the elephant's front left leg
	m_basicMeshes->DrawSphereMesh();

	// end adult elephant creation.
}

// method for creating baby elephant objects
void SceneManager::RenderBabyElephant()
{
	// create first baby elephant

	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// set the XYZ scale of front right leg
	scaleXYZ = glm::vec3(0.5f, 1.5f, 0.35f);

	// set the XYZ rotation of front right leg
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of front right leg
	positionXYZ = glm::vec3(17.0f, 0.1f, 7.0f);

	// set the transformation for the front right leg
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the front right leg
	SetShaderTexture("skin");
	// set the rough lighting material
	SetShaderMaterial("rough");

	// draw the first leg
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	// set the XYZ scale of back right leg
	scaleXYZ = glm::vec3(0.5f, 1.5f, 0.35f);

	// set the XYZ rotation of back right leg
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of back right leg
	positionXYZ = glm::vec3(17.0f, 0.1f, 9.0f);

	// set the transformation for the back right leg
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the back right leg
	SetShaderTexture("skin");
	// set the rough lighting material
	SetShaderMaterial("rough");

	// draw the first leg
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	// set the XYZ scale of front left leg
	scaleXYZ = glm::vec3(0.5f, 1.5f, 0.35f);

	// set the XYZ rotation of front left leg
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of front left leg
	positionXYZ = glm::vec3(15.0f, 0.1f, 7.0f);

	// set the transformation for the front left leg
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the front left leg
	SetShaderTexture("skin");
	// set the rough lighting material
	SetShaderMaterial("rough");

	// draw the first leg
	m_basicMeshes->DrawCylinderMesh();
	/**************************************************************/

	// set the XYZ scale of back left leg
	scaleXYZ = glm::vec3(0.5f, 1.5f, 0.35f);

	// set the XYZ rotation of back left leg
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of back left leg
	positionXYZ = glm::vec3(15.0f, 0.1f, 9.0f);

	// set the transformation for the back left leg
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the back left leg
	SetShaderTexture("skin");
	// set the rough lighting material
	SetShaderMaterial("rough");

	// draw the first leg
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	// start the creation of the elephant's body using a sphere

	// set the XYZ scale of the elephant's body
	scaleXYZ = glm::vec3(2.8f, 1.75f, 2.4f);

	// set the XYZ rotation of the elephants's body
	XrotationDegrees = 0.0f;
	YrotationDegrees = -90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the elephant's body
	positionXYZ = glm::vec3(16.2f, 2.1f, 8.2f);

	// set the transformation of the elephant's body
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the elephant's body
	SetShaderTexture("skin");
	// set the shade lighting material
	SetShaderMaterial("shade");

	// draw the elephant's body
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// start the creation of the elephant's head using a sphere

	// set the XYZ scale of the elephant's head
	scaleXYZ = glm::vec3(1.1f, 1.3f, 1.35f);

	// set the XYZ rotation of the elephant's head
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the elephant's head
	positionXYZ = glm::vec3(16.2f, 2.6f, 4.45f);

	// set the transformations of the elephant's head
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the elephant's head
	SetShaderTexture("skin");
	// set the rough lighting material
	SetShaderMaterial("rough");

	// draw the elephant's head
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// start the creation of the elephant's trunk using a tapered cylinder

	// set the XYZ scale of the elephant's trunk
	scaleXYZ = glm::vec3(0.5f, 2.9f, 0.5f);

	// set the XYZ rotation of the elephant's trunk
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 188.0f;

	//set the XYZ position of the elephant's trunk
	positionXYZ = glm::vec3(16.2f, 2.6f, 4.2f);

	// set the transformations of the elephant's trunk
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the elephant's trunk
	SetShaderTexture("skin");
	// set the rough lighting material
	SetShaderMaterial("rough");

	// draw the elephant's trunk
	m_basicMeshes->DrawTaperedCylinderMesh();
	/****************************************************************/

	// start the creation of the elephant's ears using sphere shapes

	// set the XYZ scale of the right ear
	scaleXYZ = glm::vec3(0.35f, 0.85f, 0.75f);

	// set the XYZ rotation of the right ear
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -90.0f;

	// set the XYZ position of the right ear
	positionXYZ = glm::vec3(17.6f, 2.7f, 4.4f);

	// set the transformations of the elephant's right ear
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the elephant's right ear
	SetShaderTexture("skin");
	// set the rough lighting material
	SetShaderMaterial("rough");

	// draw the elephant's right ear
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// set the XYZ scale of the left ear
	scaleXYZ = glm::vec3(0.35f, 0.85f, 0.75f);

	// set the XYZ rotation of the left ear
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -90.0f;

	// set the XYZ position of the left ear
	positionXYZ = glm::vec3(14.6f, 2.7f, 4.4f);

	// set the transformations of the elephant's left ear
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the elephant's left ear
	SetShaderTexture("skin");
	// set the rough lighting material
	SetShaderMaterial("rough");

	// draw the elephant's left ear
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// start creation of the elephant's eyes using sphere shapes

	// set the XYZ scale of the elephant's left eye
	scaleXYZ = glm::vec3(0.13f, 0.2f, 0.13f);

	// set the XYZ rotation of the elephant's left eye
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the elephant's left eye
	positionXYZ = glm::vec3(15.7f, 2.2f, 3.2f);

	// set the transformations of the elephant's left eye
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color of the elephant's left eye
	SetShaderColor(1, 1, 1, 1);

	// draw the elephant's left eye
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// start creation of the elephant's eyes using sphere shapes

	// set the XYZ scale of the elephant's right eye
	scaleXYZ = glm::vec3(0.13f, 0.2f, 0.13f);

	// set the XYZ rotation of the elephant's right eye
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the elephant's right eye
	positionXYZ = glm::vec3(16.7f, 2.2f, 3.2f);

	// set the transformations of the elephant's right eye
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color of the elephant's right eye
	SetShaderColor(1, 1, 1, 1);

	// draw the elephant's right eye
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// start creation of the elephant's tail using a tapered cylinder

	// set the XYZ scale of the elephant's tail
	scaleXYZ = glm::vec3(0.1f, 1.95f, 0.13f);

	// set the XYZ rotation of the elephant's tail
	XrotationDegrees = 40.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 180.0f;

	// set the XYZ position of the elephant's tail
	positionXYZ = glm::vec3(16.2f, 2.5f, 10.8f);

	// set the transformations of the elephant's tail
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the elephant's tail
	SetShaderTexture("skin");
	// set the shade lighting material
	SetShaderMaterial("shade");

	// draw the elephant's tail
	m_basicMeshes->DrawTaperedCylinderMesh();
	/****************************************************************/

	// start creation of the elephant's nails

	// set the XYZ scale of the elephant's left nail for the front right leg
	scaleXYZ = glm::vec3(0.1f, 0.2f, 0.1f);

	// set the XYZ rotation of the elephant's left nail for the front right leg
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the elephant's left nail for the front right leg
	positionXYZ = glm::vec3(16.8f, 0.2f, 6.7f);

	// set the transformations of the elephant's left nail for the front right leg
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color of the elephant's nails
	SetShaderColor(1, 1, 1, 1);

	// draw the nail on the left of the elephant's front right leg
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// start creation of the elephant's nails

	// set the XYZ scale of the elephant's right nail for the front right leg
	scaleXYZ = glm::vec3(0.1f, 0.2f, 0.1f);

	// set the XYZ rotation of the elephant's right nail for the front right leg
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the elephant's right nail for the front right leg
	positionXYZ = glm::vec3(17.2f, 0.2f, 6.7f);

	// set the transformations of the elephant's right nail for the front right leg
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color of the elephant's nails
	SetShaderColor(1, 1, 1, 1);

	// draw the nail on the right of the elephant's front right leg
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// set the XYZ scale of the elephant's left nail for the front left leg
	scaleXYZ = glm::vec3(0.1f, 0.2f, 0.1f);

	// set the XYZ rotation of the elephant's left nail for the front left leg
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the elephant's left nail for the front left leg
	positionXYZ = glm::vec3(14.8f, 0.2f, 6.7f);

	// set the transformations of the elephant's left nail for the front left leg
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color of the elephant's nails
	SetShaderColor(1, 1, 1, 1);

	// draw the nail on the left of the elephant's front left leg
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// set the XYZ scale of the elephant's right nail for the front left leg
	scaleXYZ = glm::vec3(0.1f, 0.2f, 0.1f);

	// set the XYZ rotation of the elephant's right nail for the front left leg
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the elephant's right nail for the front left leg
	positionXYZ = glm::vec3(15.2f, 0.2f, 6.7f);

	// set the transformations of the elephant's right nail for the front left leg
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color of the elephant's nails
	SetShaderColor(1, 1, 1, 1);

	// draw the nail on the right of the elephant's front left leg
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// set the XYZ scale of the elephant's left nail for the back right leg
	scaleXYZ = glm::vec3(0.1f, 0.2f, 0.1f);

	// set the XYZ rotation of the elephant's left nail for the back right leg
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the elephant's left nail for the back right leg
	positionXYZ = glm::vec3(16.8f, 0.2f, 8.7f);

	// set the transformations of the elephant's left nail for the back right leg
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color of the elephant's nails
	SetShaderColor(1, 1, 1, 1);

	// draw the nail on the left of the elephant's back right leg
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// set the XYZ scale of the elephant's right nail for the back right leg
	scaleXYZ = glm::vec3(0.1f, 0.2f, 0.1f);

	// set the XYZ rotation of the elephant's right nail for the back right leg
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the elephant's right nail for the back right leg
	positionXYZ = glm::vec3(17.2f, 0.2f, 8.7f);

	// set the transformations of the elephant's right nail for the back right leg
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color of the elephant's nails
	SetShaderColor(1, 1, 1, 1);

	// draw the nail on the right of the elephant's back right leg
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// set the XYZ scale of the elephant's left nail for the back left leg
	scaleXYZ = glm::vec3(0.1f, 0.2f, 0.1f);

	// set the XYZ rotation of the elephant's left nail for the back left leg
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the elephant's left nail for the back left leg
	positionXYZ = glm::vec3(14.8f, 0.2f, 8.7f);

	// set the transformations of the elephant's left nail for the back left leg
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color of the elephant's nails
	SetShaderColor(1, 1, 1, 1);

	// draw the nail on the left of the elephant's back left leg
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// set the XYZ scale of the elephant's right nail for the back left leg
	scaleXYZ = glm::vec3(0.1f, 0.2f, 0.1f);

	// set the XYZ rotation of the elephant's right nail for the back left leg
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the elephant's right nail for the back left leg
	positionXYZ = glm::vec3(15.2f, 0.2f, 8.7f);

	// set the transformations of the elephant's right nail for the back left leg
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color of the elephant's nails
	SetShaderColor(1, 1, 1, 1);

	// draw the nail on the right of the elephant's back left leg
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// end baby elephant object creation
}

// method for rendering tree objects
void SceneManager::RenderTree()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// start tree creation

	// set the XYZ scale of the upper left trunk using a box
	scaleXYZ = glm::vec3(2.0f, 15.0f, 1.6f);

	// set the XYZ rotation of the upper left trunk
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the upper left trunk
	positionXYZ = glm::vec3(-20.0f, 7.6f, -10.0f);

	// set the transformations of the upper left trunk
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the upper left trunk
	SetShaderTexture("bark");
	// set the lighting material
	SetShaderMaterial("wood");

	// draw the upper left trunk
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	// start creation of leaves using a sphere shape

	// set the XYZ scale of the leaves for the trunk
	scaleXYZ = glm::vec3(3.5f, 3.5f, 3.5f);

	// set the XYZ rotation of the leaves for the trunk
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the leaves for the tunk
	positionXYZ = glm::vec3(-19.8f, 18.0f, -9.5f);

	// set the transformations of the leaves
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the leaves
	SetShaderTexture("leaves");

	// draw the leaves for the trunk
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// start creation of branches

	// set the XYZ scale of the front branch using a tapered cylinder
	scaleXYZ = glm::vec3(1.0f, 6.5f, 1.0f);

	// set the XYZ rotation of the front branch
	XrotationDegrees = 60.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the front branch
	positionXYZ = glm::vec3(-19.8f, 11.0f, -9.5f);

	// set the transformations of the front branch
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the front branch
	SetShaderTexture("bark");

	// draw the front branch
	m_basicMeshes->DrawTaperedCylinderMesh();
	/****************************************************************/

	// set the XYZ scale of the leaves for the front branch
	scaleXYZ = glm::vec3(3.5f, 3.5f, 3.5f);

	// set the XYZ rotation of the leaves for the front branch
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the leaves for the front branch
	positionXYZ = glm::vec3(-19.8f, 16.0f, -3.5f);

	// set the transformations of the leaves
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the leaves
	SetShaderTexture("leaves");

	// draw the leaves for the front branch
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// set the XYZ scale of the left branch using a tapered cylinder
	scaleXYZ = glm::vec3(1.0f, 6.5f, 1.0f);

	// set the XYZ rotation of the left branch
	XrotationDegrees = 60.0f;
	YrotationDegrees = -95.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the left branch
	positionXYZ = glm::vec3(-20.7f, 11.8f, -10.1f);

	// set the transformations of the left branch
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the left branch
	SetShaderTexture("bark");
	// set the lighting material
	SetShaderMaterial("shade");

	// draw the left branch
	m_basicMeshes->DrawTaperedCylinderMesh();
	/****************************************************************/

	// set the XYZ scale of the leaves for the left branch
	scaleXYZ = glm::vec3(3.5f, 3.5f, 3.5f);

	// set the XYZ rotation of the leaves for the left branch
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the leaves for the left branch
	positionXYZ = glm::vec3(-25.0f, 16.0f, -9.5f);

	// set the transformations of the leaves
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the leaves
	SetShaderTexture("leaves");
	// set the lighting material
	SetShaderMaterial("shade");

	// draw the leaves for the left branch
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// set the XYZ scale of the right branch using a tapered cylinder
	scaleXYZ = glm::vec3(1.0f, 6.5f, 1.0f);

	// set the XYZ rotation of the right branch
	XrotationDegrees = -60.0f;
	YrotationDegrees = -95.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the right branch
	positionXYZ = glm::vec3(-19.5f, 11.8f, -10.1f);

	// set the transformations of the right branch
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the right branch
	SetShaderTexture("bark");

	// draw the right branch
	m_basicMeshes->DrawTaperedCylinderMesh();
	/****************************************************************/

	// set the XYZ scale of the leaves for the right branch
	scaleXYZ = glm::vec3(3.5f, 3.5f, 3.5f);

	// set the XYZ rotation of the leaves for the right branch
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the leaves for the right branch
	positionXYZ = glm::vec3(-15.0f, 16.0f, -9.5f);

	// set the transformations of the leaves
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the leaves
	SetShaderTexture("leaves");

	// draw the leaves for the right branch
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// set the XYZ scale of the back branch using a tapered cylinder
	scaleXYZ = glm::vec3(1.0f, 6.5f, 1.0f);

	// set the XYZ rotation of the back branch
	XrotationDegrees = -60.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the back branch
	positionXYZ = glm::vec3(-19.8f, 11.0f, -10.5f);

	// set the transformations of the back branch
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the back branch
	SetShaderTexture("bark");

	// draw the back branch
	m_basicMeshes->DrawTaperedCylinderMesh();
	/****************************************************************/

	// set the XYZ scale of the leaves for the back branch
	scaleXYZ = glm::vec3(3.5f, 3.5f, 3.5f);

	// set the XYZ rotation of the leaves for the back branch
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the leaves for the back branch
	positionXYZ = glm::vec3(-19.8f, 16.0f, -16.5f);

	// set the transformations of the leaves
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the leaves
	SetShaderTexture("leaves");

	// draw the leaves for the back branch
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// set the XYZ scale of the upper right trunk using a box
	scaleXYZ = glm::vec3(2.0f, 15.0f, 1.6f);

	// set the XYZ rotation of the upper right trunk
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the upper right trunk
	positionXYZ = glm::vec3(20.0f, 7.6f, -10.0f);

	// set the transformations of the upper right trunk
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the upper right trunk
	SetShaderTexture("bark");
	// set the lighting material
	SetShaderMaterial("wood");

	// draw the upper right trunk
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	// set the XYZ scale of the leaves for the trunk
	scaleXYZ = glm::vec3(3.5f, 3.5f, 3.5f);

	// set the XYZ rotation of the leaves for the trunk
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the leaves for the tunk
	positionXYZ = glm::vec3(19.8f, 18.0f, -9.5f);

	// set the transformations of the leaves
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the leaves
	SetShaderTexture("leaves");

	// draw the leaves for the trunk
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// set the XYZ scale of the front branch using a tapered cylinder
	scaleXYZ = glm::vec3(1.0f, 6.5f, 1.0f);

	// set the XYZ rotation of the front branch
	XrotationDegrees = 60.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the front branch
	positionXYZ = glm::vec3(19.8f, 11.0f, -9.5f);

	// set the transformations of the front branch
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the front branch
	SetShaderTexture("bark");

	// draw the front branch
	m_basicMeshes->DrawTaperedCylinderMesh();
	/****************************************************************/

	// set the XYZ scale of the leaves for the front branch
	scaleXYZ = glm::vec3(3.5f, 3.5f, 3.5f);

	// set the XYZ rotation of the leaves for the front branch
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the leaves for the front branch
	positionXYZ = glm::vec3(19.8f, 16.0f, -3.5f);

	// set the transformations of the leaves
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the leaves
	SetShaderTexture("leaves");

	// draw the leaves for the front branch
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// set the XYZ scale of the left branch using a tapered cylinder
	scaleXYZ = glm::vec3(1.0f, 6.5f, 1.0f);

	// set the XYZ rotation of the left branch
	XrotationDegrees = 60.0f;
	YrotationDegrees = -95.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the left branch
	positionXYZ = glm::vec3(19.2f, 11.8f, -10.1f);

	// set the transformations of the left branch
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the left branch
	SetShaderTexture("bark");

	// draw the left branch
	m_basicMeshes->DrawTaperedCylinderMesh();
	/****************************************************************/

	// set the XYZ scale of the leaves for the left branch
	scaleXYZ = glm::vec3(3.5f, 3.5f, 3.5f);

	// set the XYZ rotation of the leaves for the left branch
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the leaves for the left branch
	positionXYZ = glm::vec3(15.0f, 16.0f, -9.5f);

	// set the transformations of the leaves
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the leaves
	SetShaderTexture("leaves");

	// draw the leaves for the left branch
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// set the XYZ scale of the right branch using a tapered cylinder
	scaleXYZ = glm::vec3(1.0f, 6.5f, 1.0f);

	// set the XYZ rotation of the right branch
	XrotationDegrees = -60.0f;
	YrotationDegrees = -95.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the right branch
	positionXYZ = glm::vec3(19.5f, 11.8f, -10.1f);

	// set the transformations of the right branch
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the right branch
	SetShaderTexture("bark");

	// draw the right branch
	m_basicMeshes->DrawTaperedCylinderMesh();
	/****************************************************************/

	// set the XYZ scale of the leaves for the right branch
	scaleXYZ = glm::vec3(3.5f, 3.5f, 3.5f);

	// set the XYZ rotation of the leaves for the right branch
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the leaves for the right branch
	positionXYZ = glm::vec3(25.0f, 16.0f, -9.5f);

	// set the transformations of the leaves
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the leaves
	SetShaderTexture("leaves");

	// draw the leaves for the right branch
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// set the XYZ scale of the back branch using a tapered cylinder
	scaleXYZ = glm::vec3(1.0f, 6.5f, 1.0f);

	// set the XYZ rotation of the back branch
	XrotationDegrees = -60.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the back branch
	positionXYZ = glm::vec3(19.8f, 11.0f, -10.5f);

	// set the transformations of the back branch
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the back branch
	SetShaderTexture("bark");

	// draw the back branch
	m_basicMeshes->DrawTaperedCylinderMesh();
	/****************************************************************/

	// set the XYZ scale of the leaves for the back branch
	scaleXYZ = glm::vec3(3.5f, 3.5f, 3.5f);

	// set the XYZ rotation of the leaves for the back branch
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the leaves for the back branch
	positionXYZ = glm::vec3(19.8f, 16.0f, -16.5f);

	// set the transformations of the leaves
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the leaves
	SetShaderTexture("leaves");

	// draw the leaves for the back branch
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// set the XYZ scale of the lower left trunk using a box
	scaleXYZ = glm::vec3(2.0f, 15.0f, 1.6f);

	// set the XYZ rotation of the lower left trunk
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the lower left trunk
	positionXYZ = glm::vec3(-20.0f, 7.6f, 10.0f);

	// set the transformations of the lower left trunk
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the lower left trunk
	SetShaderTexture("bark");
	// set the lighting material
	SetShaderMaterial("wood");

	// draw the lower left trunk
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	// set the XYZ scale of the leaves for the trunk
	scaleXYZ = glm::vec3(3.5f, 3.5f, 3.5f);

	// set the XYZ rotation of the leaves for the trunk
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the leaves for the tunk
	positionXYZ = glm::vec3(-19.8f, 18.0f, 9.8f);

	// set the transformations of the leaves
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the leaves
	SetShaderTexture("leaves");

	// draw the leaves for the trunk
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// set the XYZ scale of the front branch using a tapered cylinder
	scaleXYZ = glm::vec3(1.0f, 6.5f, 1.0f);

	// set the XYZ rotation of the front branch
	XrotationDegrees = 60.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the front branch
	positionXYZ = glm::vec3(-19.8f, 11.0f, 10.4f);

	// set the transformations of the front branch
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the front branch
	SetShaderTexture("bark");

	// draw the front branch
	m_basicMeshes->DrawTaperedCylinderMesh();
	/****************************************************************/

	// set the XYZ scale of the leaves for the front branch
	scaleXYZ = glm::vec3(3.5f, 3.5f, 3.5f);

	// set the XYZ rotation of the leaves for the front branch
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the leaves for the front branch
	positionXYZ = glm::vec3(-19.8f, 16.0f, 16.5f);

	// set the transformations of the leaves
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the leaves
	SetShaderTexture("leaves");

	// draw the leaves for the front branch
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// set the XYZ scale of the left branch using a tapered cylinder
	scaleXYZ = glm::vec3(1.0f, 6.5f, 1.0f);

	// set the XYZ rotation of the left branch
	XrotationDegrees = 60.0f;
	YrotationDegrees = -95.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the left branch
	positionXYZ = glm::vec3(-20.7f, 11.8f, 10.1f);

	// set the transformations of the left branch
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the left branch
	SetShaderTexture("bark");
	// set the lighting material
	SetShaderMaterial("shade");

	// draw the left branch
	m_basicMeshes->DrawTaperedCylinderMesh();
	/****************************************************************/

	// set the XYZ scale of the leaves for the left branch
	scaleXYZ = glm::vec3(3.5f, 3.5f, 3.5f);

	// set the XYZ rotation of the leaves for the left branch
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the leaves for the left branch
	positionXYZ = glm::vec3(-25.0f, 16.0f, 9.5f);

	// set the transformations of the leaves
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the leaves
	SetShaderTexture("leaves");
	// set the lighting material
	SetShaderMaterial("shade");

	// draw the leaves for the left branch
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// set the XYZ scale of the right branch using a tapered cylinder
	scaleXYZ = glm::vec3(1.0f, 6.5f, 1.0f);

	// set the XYZ rotation of the right branch
	XrotationDegrees = -60.0f;
	YrotationDegrees = -95.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the right branch
	positionXYZ = glm::vec3(-19.5f, 11.8f, 10.1f);

	// set the transformations of the right branch
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the right branch
	SetShaderTexture("bark");

	// draw the right branch
	m_basicMeshes->DrawTaperedCylinderMesh();
	/****************************************************************/

	// set the XYZ scale of the leaves for the right branch
	scaleXYZ = glm::vec3(3.5f, 3.5f, 3.5f);

	// set the XYZ rotation of the leaves for the right branch
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the leaves for the right branch
	positionXYZ = glm::vec3(-15.0f, 16.0f, 9.5f);

	// set the transformations of the leaves
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the leaves
	SetShaderTexture("leaves");

	// draw the leaves for the right branch
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// set the XYZ scale of the back branch using a tapered cylinder
	scaleXYZ = glm::vec3(1.0f, 6.5f, 1.0f);

	// set the XYZ rotation of the back branch
	XrotationDegrees = -60.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the back branch
	positionXYZ = glm::vec3(-19.8f, 11.0f, 10.5f);

	// set the transformations of the back branch
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the back branch
	SetShaderTexture("bark");

	// draw the back branch
	m_basicMeshes->DrawTaperedCylinderMesh();
	/****************************************************************/

	// set the XYZ scale of the leaves for the back branch
	scaleXYZ = glm::vec3(3.5f, 3.5f, 3.5f);

	// set the XYZ rotation of the leaves for the back branch
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the leaves for the back branch
	positionXYZ = glm::vec3(-19.8f, 16.0f, 3.5f);

	// set the transformations of the leaves
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the leaves
	SetShaderTexture("leaves");

	// draw the leaves for the back branch
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// set the XYZ scale of the lower right trunk using a box
	scaleXYZ = glm::vec3(2.0f, 15.0f, 1.6f);

	// set the XYZ rotation of the lower right trunk
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the lower right trunk
	positionXYZ = glm::vec3(20.0f, 7.6f, 10.0f);

	// set the transformations of the lower right trunk
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the lower right trunk
	SetShaderTexture("bark");
	// set the lighting material
	SetShaderMaterial("wood");

	// draw the lower right trunk
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	// set the XYZ scale of the leaves for the trunk
	scaleXYZ = glm::vec3(3.5f, 3.5f, 3.5f);

	// set the XYZ rotation of the leaves for the trunk
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the leaves for the tunk
	positionXYZ = glm::vec3(19.8f, 18.0f, 10.0f);

	// set the transformations of the leaves
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the leaves
	SetShaderTexture("leaves");

	// draw the leaves for the trunk
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// set the XYZ scale of the front branch using a tapered cylinder
	scaleXYZ = glm::vec3(1.0f, 6.5f, 1.0f);

	// set the XYZ rotation of the front branch
	XrotationDegrees = 60.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the front branch
	positionXYZ = glm::vec3(20.0f, 11.0f, 9.8f);

	// set the transformations of the front branch
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the front branch
	SetShaderTexture("bark");

	// draw the front branch
	m_basicMeshes->DrawTaperedCylinderMesh();
	/****************************************************************/

	// set the XYZ scale of the leaves for the back branch
	scaleXYZ = glm::vec3(3.5f, 3.5f, 3.5f);

	// set the XYZ rotation of the leaves for the back branch
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the leaves for the back branch
	positionXYZ = glm::vec3(19.8f, 16.0f, 16.5f);

	// set the transformations of the leaves
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the leaves
	SetShaderTexture("leaves");

	// draw the leaves for the back branch
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// set the XYZ scale of the left branch using a tapered cylinder
	scaleXYZ = glm::vec3(1.0f, 6.5f, 1.0f);

	// set the XYZ rotation of the left branch
	XrotationDegrees = 60.0f;
	YrotationDegrees = -95.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the left branch
	positionXYZ = glm::vec3(19.2f, 11.8f, 10.1f);

	// set the transformations of the left branch
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the left branch
	SetShaderTexture("bark");

	// draw the left branch
	m_basicMeshes->DrawTaperedCylinderMesh();
	/****************************************************************/

	// set the XYZ scale of the leaves for the left branch
	scaleXYZ = glm::vec3(3.5f, 3.5f, 3.5f);

	// set the XYZ rotation of the leaves for the left branch
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the leaves for the left branch
	positionXYZ = glm::vec3(15.0f, 16.0f, 9.5f);

	// set the transformations of the leaves
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the leaves
	SetShaderTexture("leaves");

	// draw the leaves for the left branch
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// set the XYZ scale of the right branch using a tapered cylinder
	scaleXYZ = glm::vec3(1.0f, 6.5f, 1.0f);

	// set the XYZ rotation of the right branch
	XrotationDegrees = -60.0f;
	YrotationDegrees = -95.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the right branch
	positionXYZ = glm::vec3(20.8f, 11.8f, 10.1f);

	// set the transformations of the right branch
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the right branch
	SetShaderTexture("bark");

	// draw the right branch
	m_basicMeshes->DrawTaperedCylinderMesh();
	/****************************************************************/

	// set the XYZ scale of the leaves for the right branch
	scaleXYZ = glm::vec3(3.5f, 3.5f, 3.5f);

	// set the XYZ rotation of the leaves for the right branch
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the leaves for the right branch
	positionXYZ = glm::vec3(25.0f, 16.0f, 9.5f);

	// set the transformations of the leaves
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the leaves
	SetShaderTexture("leaves");

	// draw the leaves for the right branch
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// set the XYZ scale of the back branch using a tapered cylinder
	scaleXYZ = glm::vec3(1.0f, 6.5f, 1.0f);

	// set the XYZ rotation of the back branch
	XrotationDegrees = -60.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the back branch
	positionXYZ = glm::vec3(19.8f, 11.0f, 9.4f);

	// set the transformations of the back branch
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the back branch
	SetShaderTexture("bark");

	// draw the back branch
	m_basicMeshes->DrawTaperedCylinderMesh();
	/****************************************************************/

	// set the XYZ scale of the leaves for the back branch
	scaleXYZ = glm::vec3(3.5f, 3.5f, 3.5f);

	// set the XYZ rotation of the leaves for the back branch
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the leaves for the back branch
	positionXYZ = glm::vec3(19.8f, 16.0f, 3.5f);

	// set the transformations of the leaves
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the leaves
	SetShaderTexture("leaves");

	// draw the leaves for the back branch
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// end creation of trees
}

// method for rendering the sun object
void SceneManager::RenderSun()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// create sphere light source

	// set the scale of the sphere
	scaleXYZ = glm::vec3(6.5f, 6.5f, 6.5f);

	// set the rotation of the sphere
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the position of the sphere
	positionXYZ = glm::vec3(35.0f, 32.0f, -1.0f);

	// set the transformations of the sphere
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set color of the sphere
	SetShaderColor(5, 5, 0, 1);

	// set the lighting of the sphere
	SetShaderMaterial("sun");

	// draw the sphere
	m_basicMeshes->DrawSphereMesh();

	// finish creating the sun
}

void SceneManager::RenderPond()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// set the XYZ scale of the pond using a sphere
	scaleXYZ = glm::vec3(8.5f, 0.2f, 4.8f);

	// set the XYZ rotation of the pond
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the pond
	positionXYZ = glm::vec3(19.7f, 0.3f, 0.0f);

	// set the transformations of the sphere
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the pond
	SetShaderTexture("pond");
	// set the lighting material of the pond
	SetShaderMaterial("shiny");

	// draw the pond
	m_basicMeshes->DrawSphereMesh();
}

// method creates giraffe objects
void SceneManager::RenderGiraffe()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// set the XYZ scale of the front left nail of the giraffe using a box shape
	scaleXYZ = glm::vec3(1.0f, 0.8f, 0.8f);

	// set the XYZ rotation of the front left nail of the giraffe
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the front left nail of the giraffe
	positionXYZ = glm::vec3(-24.1f, 0.45f, 0.0f);

	// set the transformations of the front left nail
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color of the front left nail
	SetShaderColor(5, 0, 0, 1);

	// draw the front left nail
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	// set the XYZ scale of the front right nail of the giraffe using a box shape
	scaleXYZ = glm::vec3(1.0f, 0.8f, 0.8f);

	// set the XYZ rotation of the front right nail of the giraffe
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the front right nail of the giraffe
	positionXYZ = glm::vec3(-24.1f, 0.45f, 2.4f);

	// set the transformations of the front right nail
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color of the front right nail
	SetShaderColor(5, 0, 0, 1);

	// draw the front right nail
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	// set the XYZ scale of the back left nail of the giraffe using a box shape
	scaleXYZ = glm::vec3(1.0f, 0.8f, 0.8f);

	// set the XYZ rotation of the back left nail of the giraffe
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the back left nail of the giraffe
	positionXYZ = glm::vec3(-27.0f, 0.45f, 0.0f);

	// set the transformations of the back left nail
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color of the back left nail
	SetShaderColor(5, 0, 0, 1);

	// draw the back left nail
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	// set the XYZ scale of the back right nail of the giraffe using a box shape
	scaleXYZ = glm::vec3(1.0f, 0.8f, 0.8f);

	// set the XYZ rotation of the back right nail of the giraffe
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the back right nail of the giraffe
	positionXYZ = glm::vec3(-27.0f, 0.45f, 2.4f);

	// set the transformations of the back right nail
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color of the back right nail
	SetShaderColor(5, 0, 0, 1);

	// draw the back right nail
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	// start creation of the giraffe's legs

	// set the XYZ scale of the giraffe's bottom half front left leg using a box shape
	scaleXYZ = glm::vec3(1.0f, 2.6f, 0.8f);

	// set the XYZ rotation of the giraffe's bottom half front left leg
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the giraffe's bottom half front left leg
	positionXYZ = glm::vec3(-24.1f, 2.1f, 0.0f);

	// set the transformations of the giraffe's bottom half front left leg
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the bottom half front left leg
	SetShaderTexture("giraffe");
	// set the lighting material
	SetShaderMaterial("shade");

	// draw bottom half front left leg
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	// set the XYZ scale of the giraffe's bottom half front right leg using a box shape
	scaleXYZ = glm::vec3(1.0f, 2.6f, 0.8f);

	// set the XYZ rotation of the giraffe's bottom half front right leg
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the giraffe's bottom half front right leg
	positionXYZ = glm::vec3(-24.1f, 2.1f, 2.4f);

	// set the transformations of the giraffe's bottom half front right leg
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the bottom half front right leg
	SetShaderTexture("giraffe");
	// set the lighting material
	SetShaderMaterial("shade");

	// draw bottom half front right leg
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	// set the XYZ scale of the giraffe's bottom half back left leg using a box shape
	scaleXYZ = glm::vec3(1.0f, 2.6f, 0.8f);

	// set the XYZ rotation of the giraffe's bottom half back left leg
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the giraffe's bottom half back left leg
	positionXYZ = glm::vec3(-27.0f, 2.1f, 0.0f);

	// set the transformations of the giraffe's bottom half back left leg
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the bottom half back left leg
	SetShaderTexture("giraffe");
	// set the lighting material
	SetShaderMaterial("shade");

	// draw bottom half back left leg
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	// set the XYZ scale of the giraffe's bottom half back right leg using a box shape
	scaleXYZ = glm::vec3(1.0f, 2.6f, 0.8f);

	// set the XYZ rotation of the giraffe's bottom half back right leg
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the giraffe's bottom half back right leg
	positionXYZ = glm::vec3(-27.0f, 2.1f, 2.4f);

	// set the transformations of the giraffe's bottom half back right leg
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the bottom half back right leg
	SetShaderTexture("giraffe");
	// set the lighting material
	SetShaderMaterial("shade");

	// draw bottom half back right leg
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	// start creation of the giraffe's thighs

	// set the XYZ scale of the giraffe's front left thigh using a tapered cylinder
	scaleXYZ = glm::vec3(1.2f, 2.0f, 1.0f);

	// set the XYZ rotation of the giraffe's front left thigh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 180.0f;

	// set the XYZ position of the giraffe's front left thigh
	positionXYZ = glm::vec3(-24.1f, 5.4f, 0.0f);

	// set the tranformations of the giraffe's front left thigh
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the front left thigh
	SetShaderTexture("giraffe");
	// set the lighting material
	SetShaderMaterial("shade");

	// draw the giraffe's front left thigh
	m_basicMeshes->DrawTaperedCylinderMesh();
	/****************************************************************/

	// set the XYZ scale of the giraffe's front right thigh using a tapered cylinder
	scaleXYZ = glm::vec3(1.2f, 2.0f, 1.0f);

	// set the XYZ rotation of the giraffe's front right thigh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 180.0f;

	// set the XYZ position of the giraffe's front right thigh
	positionXYZ = glm::vec3(-24.1f, 5.4f, 2.4f);

	// set the tranformations of the giraffe's front right thigh
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the front right thigh
	SetShaderTexture("giraffe");
	// set the lighting material
	SetShaderMaterial("shade");

	// draw the giraffe's front right thigh
	m_basicMeshes->DrawTaperedCylinderMesh();
	/****************************************************************/

	// set the XYZ scale of the giraffe's back left thigh using a tapered cylinder
	scaleXYZ = glm::vec3(1.2f, 2.0f, 1.0f);

	// set the XYZ rotation of the giraffe's back left thigh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 175.0f;

	// set the XYZ position of the giraffe's back left thigh
	positionXYZ = glm::vec3(-27.0f, 5.4f, 0.0f);

	// set the tranformations of the giraffe's back left thigh
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the back left thigh
	SetShaderTexture("giraffe");
	// set the lighting material
	SetShaderMaterial("shade");

	// draw the giraffe's back left thigh
	m_basicMeshes->DrawTaperedCylinderMesh();
	/****************************************************************/

	// set the XYZ scale of the giraffe's back right thigh using a tapered cylinder
	scaleXYZ = glm::vec3(1.2f, 2.0f, 1.0f);

	// set the XYZ rotation of the giraffe's back right thigh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 175.0f;

	// set the XYZ position of the giraffe's back right thigh
	positionXYZ = glm::vec3(-27.0f, 5.4f, 2.4f);

	// set the tranformations of the giraffe's back right thigh
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the back right thigh
	SetShaderTexture("giraffe");
	// set the lighting material
	SetShaderMaterial("shade");

	// draw the giraffe's back right thigh
	m_basicMeshes->DrawTaperedCylinderMesh();
	/****************************************************************/

	// start the creation of the giraffe's body using a cylinder

	// set the XYZ scale of the giraffe's body
	scaleXYZ = glm::vec3(2.0f, 5.6f, 3.0f);

	// set the XYZ rotation of the giraffe's body
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position of the giraffe's body
	positionXYZ = glm::vec3(-22.7f, 6.8f, 1.1f);

	// set the transformations of the giraffe's body
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the body
	SetShaderTexture("giraffe");
	// set the lighting material
	SetShaderMaterial("shade");

	// draw the giraffe's body
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	// start creation of the giraffe's neck

	// set the XYZ scale of the giraffe's neck using a tapered cylinder
	scaleXYZ = glm::vec3(2.2f, 6.6f, 1.6f);

	// set the XYZ rotation of the giraffe's neck
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -55.0f;

	// set the XYZ position of the giraffe's neck
	positionXYZ = glm::vec3(-24.0f, 6.8f, 0.8f);

	// set the transformations of the giraffe's neck
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the neck
	SetShaderTexture("giraffe");
	// set the lighting material
	SetShaderMaterial("shade");

	// draw the giraffe's neck
	m_basicMeshes->DrawTaperedCylinderMesh();
	/****************************************************************/

	// start creation of the giraffe's head

	// set the XYZ scale of the giraffe's head using a prism shape
	scaleXYZ = glm::vec3(2.2f, 2.0f, 4.4f);

	// set the XYZ rotation of the giraffe's head
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 40.0f;

	// set the XYZ position of the giraffe's head
	positionXYZ = glm::vec3(-17.4f, 11.8f, 0.8f);

	// set the transformations of the giraffe's head
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the head
	SetShaderTexture("giraffe");
	// set the lighting material
	SetShaderMaterial("shade");

	// draw the giraffe's head
	m_basicMeshes->DrawPrismMesh();
	/****************************************************************/

	// start creation of the giraffe's left ossicone

	// set the XYZ scale of the giraffe's left ossicone using a tapered cylinder
	scaleXYZ = glm::vec3(0.4, 1.4, 0.2);

	// set the XYZ rotation of the giraffe's left ossicone
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 40.0f;

	// set the XYZ position of the giraffe's left ossicone
	positionXYZ = glm::vec3(-19.2f, 11.6f, 0.5f);

	// set the transformations of the giraffe's left ossicone
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the left ossicone
	SetShaderTexture("giraffe");

	// draw the giraffe's left ossicone
	m_basicMeshes->DrawTaperedCylinderMesh();
	/****************************************************************/

	// set the XYZ scale of the giraffe's right ossicone using a tapered cylinder
	scaleXYZ = glm::vec3(0.4, 1.4, 0.2);

	// set the XYZ rotation of the giraffe's right ossicone
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 40.0f;

	// set the XYZ position of the giraffe's right ossicone
	positionXYZ = glm::vec3(-19.2f, 11.6f, 1.1f);

	// set the transformations of the giraffe's right ossicone
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the right ossicone
	SetShaderTexture("giraffe");

	// draw the giraffe's right ossicone
	m_basicMeshes->DrawTaperedCylinderMesh();
	/****************************************************************/

	// start creation of the giraffe's ears

	// set the XYZ scale of the giraffe's left ear using a prism
	scaleXYZ = glm::vec3(3.6f, 0.4f, 1.2f);

	// set the XYZ rotation of the giraffe's left ear
	XrotationDegrees = 0.0f;
	YrotationDegrees = 170.0f;
	ZrotationDegrees = 40.0f;

	// set the XYZ position of the giraffe's left ear
	positionXYZ = glm::vec3(-17.9f, 11.9f, -0.3f);

	// set the transformations of the giraffe's left ear
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the left ear
	SetShaderTexture("giraffe");

	// draw the giraffe's left ear
	m_basicMeshes->DrawPrismMesh();
	/****************************************************************/

	// set the XYZ scale of the giraffe's right ear using a prism
	scaleXYZ = glm::vec3(3.6f, 0.4f, 1.2f);

	// set the XYZ rotation of the giraffe's right ear
	XrotationDegrees = 0.0f;
	YrotationDegrees = 14.0f;
	ZrotationDegrees = 40.0f;

	// set the XYZ position of the giraffe's right ear
	positionXYZ = glm::vec3(-17.9f, 11.9f, 2.0f);

	// set the transformations of the giraffe's right ear
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the right ear
	SetShaderTexture("giraffe");

	// draw the giraffe's right ear
	m_basicMeshes->DrawPrismMesh();
	/****************************************************************/

	// start creation of the giraffe's tail

	// set the XYZ scale of the giraffe's tail using a tapered cylinder
	scaleXYZ = glm::vec3(0.4f, 5.0f, 0.4f);

	// set the XYZ rotation of the giraffe's tail
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 118.0f;

	// set the XYZ position of the giraffe's tail
	positionXYZ = glm::vec3(-27.8f, 7.5f, 0.9f);

	// set the transformations of the giraffe's tail
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the tail
	SetShaderTexture("giraffe");

	// draw the giraffe's tail
	m_basicMeshes->DrawTaperedCylinderMesh();
	/****************************************************************/

	// start creation of the giraffe's left eye

	// set the XYZ scale of the giraffe's left eye
	scaleXYZ = glm::vec3(0.19f, 0.19f, 0.19f);

	// set the XYZ rotation of the giraffe's left eye
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the giraffe's left eye
	positionXYZ = glm::vec3(-17.9f, 12.8f, 0.5f);

	// set the transformations of the giraffe's left eye
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color of the giraffe's left eye
	SetShaderColor(1, 1, 1, 1);

	// draw the giraffe's left eye
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// start creation of the giraffe's right eye

	// set the XYZ scale of the giraffe's right eye
	scaleXYZ = glm::vec3(0.19f, 0.19f, 0.19f);

	// set the XYZ rotation of the giraffe's right eye
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the giraffe's right eye
	positionXYZ = glm::vec3(-17.9f, 12.8f, 1.1f);

	// set the transformations of the giraffe's right eye
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color of the giraffe's right eye
	SetShaderColor(1, 1, 1, 1);

	// draw the giraffe's right eye
	m_basicMeshes->DrawSphereMesh();

	// finish giraffe object creation
}


void SceneManager::RenderRocks()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// set the XYZ scale of the lower right rock using a sphere
	scaleXYZ = glm::vec3(1.3f, 0.7f, 1.5f);

	// set the XYZ rotation of the lower right rock
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the lower right rock
	positionXYZ = glm::vec3(20.0f, 0.7f, 14.7f);

	// set the transformations of the lower right rock
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the rock
	SetShaderTexture("rock");
	// set the lighting material
	SetShaderMaterial("shade");

	// draw the lower right rock
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// set the XYZ scale of the middle right rock using a sphere
	scaleXYZ = glm::vec3(1.3f, 0.7f, 1.5f);

	// set the XYZ rotation of the middle right rock
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the middle right rock
	positionXYZ = glm::vec3(15.0f, 0.7f, -6.7f);

	// set the transformations of the middle right rock
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the rock
	SetShaderTexture("rock");
	// set the lighting material
	SetShaderMaterial("rough");

	// draw the middle right rock
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// set the XYZ scale of the middle left rock using a sphere
	scaleXYZ = glm::vec3(1.3f, 0.7f, 1.5f);

	// set the XYZ rotation of the middle left rock
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the middle left rock
	positionXYZ = glm::vec3(-16.0f, 0.7f, 0.7f);

	// set the transformations of the middle left rock
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the rock
	SetShaderTexture("rock");
	// set the lighting material
	SetShaderMaterial("shade");

	// draw the middle left rock
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// set the XYZ scale of the upper left using a sphere
	scaleXYZ = glm::vec3(1.3f, 0.7f, 1.5f);

	// set the XYZ rotation of the upper left rock
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the upper left rock
	positionXYZ = glm::vec3(-23.0f, 0.7f, -15.7f);

	// set the transformations of the upper left rock
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the rock
	SetShaderTexture("rock");
	// set the lighting material
	SetShaderMaterial("shade");

	// draw the middle upper left rock
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/

	// set the XYZ scale of the lower left rock using a sphere
	scaleXYZ = glm::vec3(1.3f, 0.7f, 1.5f);

	// set the XYZ rotation of the lower left rock
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position of the lower left rock
	positionXYZ = glm::vec3(0.0f, 0.7f, 12.9f);

	// set the transformations of the lower left rock
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the texture of the rock
	SetShaderTexture("rock");
	// set the lighting material
	SetShaderMaterial("grass");

	// draw the lower left rock
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/
}