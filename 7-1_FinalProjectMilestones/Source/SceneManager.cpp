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
			// pass the material properties into the shader
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

/***************************************************************
 * DefineObjectMaterials()
 *
 * Method for defining various material settings for all objects
 * within the 3D scene.
 ***************************************************************/
void SceneManager::DefineObjectMaterials() {
	
	// shiny material definition
	OBJECT_MATERIAL shinyMaterial;
	shinyMaterial.ambientColor = glm::vec3(0.3f, 0.1f, 0.1f);
	shinyMaterial.ambientStrength = 0.1;
	shinyMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	shinyMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);
	shinyMaterial.shininess = 64.0f;
	shinyMaterial.tag = "metal";

	m_objectMaterials.push_back(shinyMaterial);

	// dull material definition
	OBJECT_MATERIAL dullMaterial;
	dullMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	dullMaterial.ambientStrength = 0.1;
	dullMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	dullMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	dullMaterial.shininess = 16.0f;
	dullMaterial.tag = "wood";

	m_objectMaterials.push_back(dullMaterial);
}
/*******************************************************
 * SetupSceneLights()
 * 
 * Method for adding and configuring light sources, 4 max
 *******************************************************/
void SceneManager::SetupSceneLights() {

	// using default OpenGL lighting
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// camera position at (0.0f, 5.0f, 12.0f) making sure all light positions aren't blocked by camera

	// light 1
	m_pShaderManager->setVec3Value("lightSources[0].position", -7.0f, 7.0f, 10.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.8f, 0.8f, 0.7f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 1.0f, 0.95f, 0.85f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.7f);

	// light 2
	m_pShaderManager->setVec3Value("lightSources[1].position", 7.0f, -6.0f, 1.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.8f, 0.8f, 0.7f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 1.0f, 0.95f, 0.85f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.7f);

	// light 3
	m_pShaderManager->setVec3Value("lightSources[2].position", 7.0f, 7.0f, 5.0f);
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", 0.8f, 0.8f, 0.7f);
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", 1.0f, 0.95f, 0.85f);
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 16.0f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.7f);

	// light 4
	m_pShaderManager->setVec3Value("lightSources[3].position", -7.0f,-6.0f, 1.0f);
	m_pShaderManager->setVec3Value("lightSources[3].ambientColor", 0.8f, 0.8f, 0.7f);
	m_pShaderManager->setVec3Value("lightSources[3].diffuseColor", 1.0f, 0.95f, 0.85f);
	m_pShaderManager->setVec3Value("lightSources[3].specularColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setFloatValue("lightSources[3].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[3].specularIntensity", 0.7f);
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

	// load meshes
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh(); //adding box mesh to PrepareScene
	m_basicMeshes->LoadTaperedCylinderMesh(); //adding tapered cylinder to PrepareScene
	m_basicMeshes->LoadPrismMesh(); // adding prism mesh to PrepareScene
	m_basicMeshes->LoadPyramid3Mesh(); // adding pyramid mesh to PrepareScene

	// load textures
	bool bReturn = false;

	// brick texture
	bReturn = CreateGLTexture("textures/Brick.jpg", "brick");
	// regular wood texture
	bReturn = CreateGLTexture("textures/Wood Test.jpg", "wood");
	// stucco texture
	bReturn = CreateGLTexture("textures/Wall.jpg", "wall");
	// grass texture
	bReturn = CreateGLTexture("textures/Grass.jpg", "grass");
	// cement texture
	bReturn = CreateGLTexture("textures/PatternCement.jpeg", "cement");
	// light tan beam texture
	bReturn = CreateGLTexture("textures/LightTan.jpg", "beam");
	// front door texture
	bReturn = CreateGLTexture("textures/door.jpg", "door");
	// outside industrial green texture
	bReturn = CreateGLTexture("textures/outergreen.jpg", "outergreen");
	// smooth concrete texture
	bReturn = CreateGLTexture("textures/concrete.jpeg", "concrete");
	// black roof texture
	bReturn = CreateGLTexture("textures/roof.jpg", "roof");
	// window texture
	bReturn = CreateGLTexture("textures/glass.jpg", "window");
	// garage door texture
	bReturn = CreateGLTexture("textures/garagedoor.jpg", "garage");

	// binding loaded textures into texture slots (16 max)
	BindGLTextures();
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
	// first plane (ground level)
	// set the XYZ scale for the mesh 
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh (no rotation)
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
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

	SetShaderColor(0.5, 0.5, 0.5, 1); // grey color
	SetShaderTexture("grass"); // calling grass texture
	SetTextureUVScale(1, 1); // mapping texture across entire plane
	SetShaderMaterial("metal");
	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh(); // draw plane

	// drawing second plane (behind the house)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f; // x rotation to place plane behind the house
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 9.0f, -10.0f); // moving plane 9 y units and -10 z units to align planes, same x value

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.55, 0.55, 0.55, 1); // lighter grey color for contrast
	SetShaderMaterial("metal"); // call material definition
	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh(); // draw plane
	/****************************************************************/

	//Support Column #1
	// set the XYZ scale for the box mesh
	scaleXYZ = glm::vec3(1.0f, 0.5f, 1.0f); // small box

	// set the XYZ rotation for the mesh (no rotation)
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(6.75f, 0.25f, 3.0f); // furthest column to the right

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.25, 0.17, 0.07, 1); // dark brown color for the base
	SetShaderTexture("wood"); // calling wood texture
	SetTextureUVScale(1, 0.5); // texture stretched across width and compressed vertically
	SetShaderMaterial("wood"); // call material definition
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh(); // draw box


	// set the XYZ scale for the tapered cylinder mesh
	scaleXYZ = glm::vec3(0.3f, 3.0f, 0.3f); // thin and tall tapered cylinder

	// set the XYZ rotation for the mesh (no rotation)
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(6.75f, 0.5f, 3.0f); // increase Y value to stack tapered cylinder on top of box, matching X and Z values

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.4, 0.2, 0.1, 1); // medium brown color for the pillar
	SetShaderTexture("wood"); // calling wood texture
	SetTextureUVScale(1.8, 3.0); // scale uv based on tapered cylinder dimensions
	SetShaderMaterial("wood"); // call material definition
	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh(); // draw tapered cylinder
	/****************************************************************/

	//Support Column #2
	// set the XYZ scale for the box mesh
	scaleXYZ = glm::vec3(1.0f, 0.5f, 1.0f); // same sized box

	// set the XYZ rotation for the mesh (no rotation)
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.75f, 0.25f, 3.0f); // reduce X value to move box to the left, matching Y and Z values

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.25, 0.17, 0.07, 1); // dark brown color for the base
	SetShaderTexture("wood"); // calling wood texture
	SetTextureUVScale(1, 0.5); // texture stretched across width and compressed vertically
	SetShaderMaterial("wood"); // call material definition
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh(); // draw box


	// set the XYZ scale for the tapered cylinder mesh
	scaleXYZ = glm::vec3(0.3f, 3.0f, 0.3f); // same sized tapered cylinder

	// set the XYZ rotation for the mesh (no rotation)
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.75f, 0.5f, 3.0f); // increase Y value to stack tapered cylinder on top of box, matching X and Z values

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.4, 0.2, 0.1, 1); // medium brown color for the pillar
	SetShaderTexture("wood"); // calling wood texture
	SetTextureUVScale(1.8, 3.0); // scale uv based on tapered cylinder dimensions
	SetShaderMaterial("wood"); // call material definition
	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh(); // draw tapered cylinder
	/****************************************************************/

	//Support Column #3
	// set the XYZ scale for the box mesh
	scaleXYZ = glm::vec3(1.0f, 0.5f, 1.0f); // same sized box

	// set the XYZ rotation for the mesh (no rotation)
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.25f, 0.25f, 3.0f); // decrease X value to move box to the left, matching Y and Z values

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.25, 0.17, 0.07, 1); // dark brown color for the base
	SetShaderTexture("wood"); // calling wood texture
	SetTextureUVScale(1, 0.5); // texture stretched across width and compressed vertically
	SetShaderMaterial("wood"); // call material definition
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh(); // draw box


	// set the XYZ scale for the tapered cylinder mesh
	scaleXYZ = glm::vec3(0.3f, 3.0f, 0.3f); // same sized tapered cylinder

	// set the XYZ rotation for the mesh (no rotation)
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.25f, 0.5f, 3.0f); // increase Y value to stack tapered cylinder on top of box, matching X and Z values

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.4, 0.2, 0.1, 1); // medium brown color for the pillar
	SetShaderTexture("wood"); // calling wood texture
	SetTextureUVScale(1.8, 3.0); // scale uv based on tapered cylinder dimensions
	SetShaderMaterial("wood"); // call material definition
	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh(); // draw tapered cylinder
	/****************************************************************/

	//Support Column #4
	// set the XYZ scale for the box mesh
	scaleXYZ = glm::vec3(1.0f, 0.5f, 1.0f); // same sized box

	// set the XYZ rotation for the mesh (no rotation)
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.0f, 0.25f, 3.0f); // decrease X value to move box to the left, matching Y and Z values

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.25, 0.17, 0.07, 1); // dark brown color for the base
	SetShaderTexture("wood"); // calling wood texture
	SetTextureUVScale(1, 0.5); // texture stretched across width and compressed vertically
	SetShaderMaterial("wood"); // call material definition
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh(); // draw box


	// set the XYZ scale for the tapered cylinder mesh
	scaleXYZ = glm::vec3(0.3f, 3.0f, 0.3f); // same sized tapered cylinder

	// set the XYZ rotation for the mesh (no rotation)
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh 
	positionXYZ = glm::vec3(-2.0f, 0.5f, 3.0f); // increase Y value to stack tapered cylinder on top of box, matching X and Z values

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.4, 0.2, 0.1, 1); // medium brown color for the pillar
	SetShaderTexture("wood"); // calling wood texture
	SetTextureUVScale(1.8, 3.0); // scale uv based on tapered cylinder dimensions
	SetShaderMaterial("wood"); // call material definition
	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh(); // draw tapered cylinder
	/****************************************************************/

	// Horizontal Beam
	// set the XYZ scale for the box mesh
	scaleXYZ = glm::vec3(10.0f, 0.5f, 1.0f); // long rectangular box

	// set the XYZ rotation for the mesh (no rotation)
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.25f, 3.75f, 3.0f); // set X value to cover the width of all columns evenly, set Y value to stack on top of all columns, matching Z value

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.8, 0.5, 0.3, 1); // light brown color for the beam
	SetShaderTexture("roof"); // calling roof texture
	SetTextureUVScale(8, 1); // scale uv based on box dimensions
	SetShaderMaterial("wood"); // call material definition
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh(); // draw box

	// house body (left)
	scaleXYZ = glm::vec3(15.0f, 3.5f, 8.0f); // long and tall rectangular box

	// set the XYZ rotation for the mesh (no rotation)
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-10.25f, 1.85f, -4.5f); 

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.8, 0.5, 0.3, 1); // light brown color for the beam
	SetShaderTexture("outergreen"); // calling outergreen texture
	SetTextureUVScale(4, 1); // scale uv based on box dimensions
	SetShaderMaterial("wood"); // call material definition
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh(); // draw box

	// house body(right)
	// house body
	scaleXYZ = glm::vec3(7.0f, 3.5f, 8.0f); // long and tall rectangular box

	// set the XYZ rotation for the mesh (no rotation)
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.25f, 1.5f, -4.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.8, 0.5, 0.3, 1); // light brown color for the beam
	SetShaderTexture("brick"); // calling brick texture
	SetTextureUVScale(8, 1); // scale uv based on shape dimensions
	SetShaderMaterial("wood"); // call material definition
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh(); // draw box

	// porch walkway
	scaleXYZ = glm::vec3(3.75f, 0.1f, 15.5f);

	// set the XYZ rotation for the mesh (no rotation)
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.0f, 0.0f, 2.20f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.8, 0.5, 0.3, 1); // light brown color for the beam
	SetShaderTexture("concrete"); // calling concrete texture
	SetTextureUVScale(1, 1); // scale uv based on shape dimensions
	SetShaderMaterial("wood"); // call material definition
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh(); // draw box

	// right side of porch area
	// porch walkway
	scaleXYZ = glm::vec3(9.75f, 0.1f, 5.25f); 

	// set the XYZ rotation for the mesh (no rotation)
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.90f, 0.0f, 0.95f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.8, 0.5, 0.3, 1); // light brown color for the beam
	SetShaderTexture("concrete"); // calling concrete texture
	SetTextureUVScale(1, 1); // scale uv based on shape dimensions
	SetShaderMaterial("wood"); // call material definition
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh(); // draw box

	// front door
	scaleXYZ = glm::vec3(3.75f, 3.4f, 4.0f); 

	// set the XYZ rotation for the mesh (no rotation)
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.0f, 1.95f, -6.45f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.8, 0.5, 0.3, 1); // light brown color for the beam
	SetShaderTexture("door"); // calling door texture
	SetTextureUVScale(1, 1); // scale uv based on shape dimensions
	SetShaderMaterial("wood"); // call material definition
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh(); // draw box

	

	// upper house body (2nd story)
	scaleXYZ = glm::vec3(25.5f, 3.5f, 5.0f); // long and tall rectangular box

	// set the XYZ rotation for the mesh (no rotation)
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.00f, 5.35f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.8, 0.5, 0.3, 1); // light brown color for the beam
	SetShaderTexture("wall"); // calling wall texture
	SetTextureUVScale(8, 1); // scale uv based on shape dimensions
	SetShaderMaterial("wood"); // call material definition
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh(); // draw box

	// upper left house pyramid (2nd story)
	scaleXYZ = glm::vec3(3.0f, 3.0f, 3.0f); 

	// set the XYZ rotation for the mesh 
	XrotationDegrees = 270.0f; // x rotation to align with body of house
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-16.25f, 5.0f, -2.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.8, 0.5, 0.3, 1); // light brown color for the beam
	SetShaderTexture("outergreen"); // calling outergreen texture
	SetTextureUVScale(1, 1); // scale uv based on shape dimensions
	SetShaderMaterial("wood"); // call material definition
	// draw the mesh with transformation values
	m_basicMeshes->DrawPrismMesh(); // draw prism

	// upper roof (2nd story)
	scaleXYZ = glm::vec3(13.0f, 2.5f, 4.0f); 

	// set the XYZ rotation for the mesh (no rotation)
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.00f, 7.25f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.8, 0.5, 0.3, 1); // light brown color for the beam
	SetShaderTexture("roof"); // calling roof texture
	SetTextureUVScale(8, 1); // scale uv based on shape dimensions
	SetShaderMaterial("wood"); // call material definition
	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh(); // draw plane

	// second upper left house pyramid (2nd story)
	scaleXYZ = glm::vec3(3.0f, 3.0f, 3.0f); 

	// set the XYZ rotation for the mesh 
	XrotationDegrees = 270.0f; // x rotation for shape to align with house body
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.25f, 5.0f, -2.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.8, 0.5, 0.3, 1); // light brown color for the beam
	SetShaderTexture("outergreen"); // calling outergreen texture
	SetTextureUVScale(1, 1); // scale uv based on shape dimensions
	SetShaderMaterial("wood"); // call material definition
	// draw the mesh with transformation values
	m_basicMeshes->DrawPrismMesh(); // draw prism

	// second story window #1 left 
	scaleXYZ = glm::vec3(2.0f, 1.0f, 1.0f);

	// set the XYZ rotation for the mesh 
	XrotationDegrees = 90.0f; // x rotation to align with body of house
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-10.0f, 5.25f, -3.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.5, 0.5, 0.5, 1); // grey color
	SetShaderTexture("window"); // calling grass texture
	SetTextureUVScale(1, 1); // mapping texture across entire plane
	SetShaderMaterial("metal"); // calling material definition
	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh(); // draw plane


	// second story window #2 right 
	scaleXYZ = glm::vec3(2.0f, 1.0f, 1.0f);

	// set the XYZ rotation for the mesh 
	XrotationDegrees = 90.0f; // x rotation to align with body of house
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.0f, 5.25f, -3.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.5, 0.5, 0.5, 1); // grey color
	SetShaderTexture("window"); // calling grass texture
	SetTextureUVScale(1, 1); // mapping texture across entire plane
	SetShaderMaterial("metal"); // calling material definition
	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh(); // draw plane

	// first story window #1 right 
	scaleXYZ = glm::vec3(2.0f, 1.0f, 1.0f);

	// set the XYZ rotation for the mesh (no rotation)
	XrotationDegrees = 90.0f; // x rotation to align with body of house 
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.0f, 2.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.5, 0.5, 0.5, 1); // grey color
	SetShaderTexture("window"); // calling grass texture
	SetTextureUVScale(1, 1); // mapping texture across entire plane
	SetShaderMaterial("metal"); // calling material definition
	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh(); // draw plane

	// garage door
	scaleXYZ = glm::vec3(5.5f, 1.0f, 1.50f);

	// set the XYZ rotation for the mesh (no rotation)
	XrotationDegrees = 90.0f; // x rotation to align with body of house 
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-10.0f, 1.75f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.5, 0.5, 0.5, 1); // grey color
	SetShaderTexture("garage"); // calling grass texture
	SetTextureUVScale(0, 1); // mapping texture across entire plane
	SetShaderMaterial("metal"); // calling material definition
	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh(); // draw plane

	// first story roof (left)
	scaleXYZ = glm::vec3(8.0f, 1.0f, 4.0f);

	// set the XYZ rotation for the mesh (no rotation)
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-10.75f, 3.7f, -4.25f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.8, 0.5, 0.3, 1); // light brown color for the beam
	SetShaderTexture("roof");
	SetTextureUVScale(4, 1); // scale uv based on shape dimensions
	SetShaderMaterial("wood"); // call material definition
	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh(); // draw plane

	// first story roof right side
	scaleXYZ = glm::vec3(5.0f, 1.0f, 5.0f); 

	// set the XYZ rotation for the mesh (no rotation)
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.25f, 3.5f, -1.75f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.8, 0.5, 0.3, 1); // light brown color for the beam
	SetShaderTexture("roof"); // calling roof texture
	SetTextureUVScale(8, 1); // scale uv based on shape dimensions
	SetShaderMaterial("wood"); // call material definition
	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh(); // draw plane

	// driveway
	scaleXYZ = glm::vec3(5.5f, 1.0f, 7.0f);

	// set the XYZ rotation for the mesh (no rotation)
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-10.0f, 0.01f, 3.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.5, 0.5, 0.5, 1); // grey color
	SetShaderTexture("concrete"); // calling grass texture
	SetTextureUVScale(1, 1); // mapping texture across entire plane
	SetShaderMaterial("metal"); // calling material definition
	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh(); // draw plane

}
