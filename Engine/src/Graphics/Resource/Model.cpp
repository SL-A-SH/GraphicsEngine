#include "model.h"
#include <algorithm>
#include "../../Core/System/Logger.h"

Model::Model()
{
	m_vertexBuffer = 0;
	m_indexBuffer = 0;
	m_Texture = 0;
	m_model = 0;
	m_hasFBXMaterial = false;
	m_materialInfo.diffuseColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_materialInfo.ambientColor = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	m_materialInfo.specularColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_materialInfo.shininess = 32.0f;
	m_materialInfo.metallic = 0.0f;
	m_materialInfo.roughness = 0.5f;
	m_materialInfo.ao = 1.0f;
	m_materialInfo.emissionStrength = 0.0f;
	m_currentFBXPath = "";
	
	// Initialize PBR texture pointers
	m_diffuseTexture = nullptr;
	m_normalTexture = nullptr;
	m_metallicTexture = nullptr;
	m_roughnessTexture = nullptr;
	m_emissionTexture = nullptr;
	m_aoTexture = nullptr;
}





Model::~Model()
{
}


bool Model::Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, char* modelFilename, char* textureFilename)
{
	bool result;

	// Load in the model data.
	result = LoadModel(modelFilename);
	if (!result)
	{
		return false;
	}

	// Initialize the vertex and index buffers.
	result = InitializeBuffers(device);
	if (!result)
	{
		return false;
	}

	// Load the textures for this model.
	result = LoadTexture(device, deviceContext, textureFilename);
	if (!result)
	{
		return false;
	}

	return true;
}


bool Model::Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, char* modelFilename, char* textureFilename1, char* textureFilename2)
{
	bool result;


	// Load in the model data.
	result = LoadModel(modelFilename);
	if (!result)
	{
		return false;
	}

	// Calculate the tangent and binormal vectors for the model.
	CalculateModelVectors();

	// Initialize the vertex and index buffers.
	result = InitializeBuffers(device);
	if (!result)
	{
		return false;
	}

	// Load the textures for this model.
	result = LoadTextures(device, deviceContext, textureFilename1, textureFilename2);
	if (!result)
	{
		return false;
	}

	return true;
}

bool Model::Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, char* modelFilename, char* textureFilename1, char* textureFilename2, char* textureFilename3)
{
	bool result;


	// Load in the model data.
	result = LoadModel(modelFilename);
	if (!result)
	{
		return false;
	}

	// Initialize the vertex and index buffers.
	result = InitializeBuffers(device);
	if (!result)
	{
		return false;
	}

	// Load the textures for this model.
	result = LoadTextures(device, deviceContext, textureFilename1, textureFilename2, textureFilename3);
	if (!result)
	{
		return false;
	}

	return true;
}

bool Model::InitializeFBX(ID3D11Device* device, ID3D11DeviceContext* deviceContext, char* modelFilename)
{
	bool result;

	LOG("Initializing FBX model: " + std::string(modelFilename));

	// Load in the model data.
	result = LoadModel(modelFilename);
	if (!result)
	{
		LOG_ERROR("Failed to load FBX model");
		return false;
	}

	CalculateModelVectors();

	// Initialize the vertex and index buffers.
	result = InitializeBuffers(device);
	if (!result)
	{
		LOG_ERROR("Failed to initialize buffers");
		return false;
	}

	// Load textures from FBX materials
	if (m_hasFBXMaterial)
	{
		LOG("Loading textures from FBX materials...");
		result = LoadFBXTextures(device, deviceContext);
		if (!result)
		{
			LOG_ERROR("Failed to load FBX textures");
			return false;
		}
	}
	else
	{
		LOG_WARNING("No FBX materials found, model will be rendered without textures");
	}

	return true;
}


void Model::Shutdown()
{
	// Release the model textures.
	ReleaseTextures();

	// Shutdown the vertex and index buffers.
	ShutdownBuffers();

	// Release the model data.
	ReleaseModel();

	return;
}


void Model::Render(ID3D11DeviceContext* deviceContext)
{
	// Put the vertex and index buffers on the graphics pipeline to prepare them for drawing.
	RenderBuffers(deviceContext);

	return;
}





ID3D11ShaderResourceView* Model::GetTexture() const
{
	if (m_Texture)
	{
		return m_Texture->GetTexture();
	}
	return nullptr;
}


ID3D11ShaderResourceView* Model::GetTexture(int index) const
{
	return m_Textures[index].GetTexture();
}


ID3D11ShaderResourceView* Model::GetDiffuseTexture() const
{
	if (m_diffuseTexture)
	{
		return m_diffuseTexture->GetTexture();
	}
	return nullptr;
}

ID3D11ShaderResourceView* Model::GetNormalTexture() const
{
	if (m_normalTexture)
	{
		return m_normalTexture->GetTexture();
	}
	return nullptr;
}

ID3D11ShaderResourceView* Model::GetMetallicTexture() const
{
	if (m_metallicTexture)
	{
		return m_metallicTexture->GetTexture();
	}
	return nullptr;
}

ID3D11ShaderResourceView* Model::GetRoughnessTexture() const
{
	if (m_roughnessTexture)
	{
		return m_roughnessTexture->GetTexture();
	}
	return nullptr;
}

ID3D11ShaderResourceView* Model::GetEmissionTexture() const
{
	if (m_emissionTexture)
	{
		return m_emissionTexture->GetTexture();
	}
	return nullptr;
}

ID3D11ShaderResourceView* Model::GetAOTexture() const
{
	if (m_aoTexture)
	{
		return m_aoTexture->GetTexture();
	}
	return nullptr;
}


bool Model::InitializeBuffers(ID3D11Device* device)
{
	VertexType* vertices;
	unsigned long* indices;
	D3D11_BUFFER_DESC vertexBufferDesc, indexBufferDesc;
	D3D11_SUBRESOURCE_DATA vertexData, indexData;
	HRESULT result;
	int i;

	// Create the vertex array.
	vertices = new VertexType[m_vertexCount];
	if (!vertices)
	{
		return false;
	}

	// Create the index array.
	indices = new unsigned long[m_indexCount];
	if (!indices)
	{
		return false;
	}

	// Load the vertex array and index array with data.
	for (i = 0; i < m_vertexCount; i++)
	{
		vertices[i].position = XMFLOAT3(m_model[i].x, m_model[i].y, m_model[i].z);
		vertices[i].texture = XMFLOAT2(m_model[i].tu, m_model[i].tv);
		vertices[i].normal = XMFLOAT3(m_model[i].nx, m_model[i].ny, m_model[i].nz);
		vertices[i].tangent = XMFLOAT3(m_model[i].tx, m_model[i].ty, m_model[i].tz);
		vertices[i].binormal = XMFLOAT3(m_model[i].bx, m_model[i].by, m_model[i].bz);

		indices[i] = i;
	}

	// Set up the description of the static vertex buffer.
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(VertexType) * m_vertexCount;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	// Give the subresource structure a pointer to the vertex data.
	vertexData.pSysMem = vertices;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	// Now create the vertex buffer.
	result = device->CreateBuffer(&vertexBufferDesc, &vertexData, &m_vertexBuffer);
	if (FAILED(result))
	{
		return false;
	}

	// Set up the description of the static index buffer.
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(unsigned long) * m_indexCount;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	// Give the subresource structure a pointer to the index data.
	indexData.pSysMem = indices;
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	// Create the index buffer.
	result = device->CreateBuffer(&indexBufferDesc, &indexData, &m_indexBuffer);
	if (FAILED(result))
	{
		return false;
	}

	// Release the arrays now that the vertex and index buffers have been created and loaded.
	delete[] vertices;
	vertices = 0;

	delete[] indices;
	indices = 0;

	return true;
}


void Model::ShutdownBuffers()
{
	// Release the index buffer.
	if (m_indexBuffer)
	{
		m_indexBuffer->Release();
		m_indexBuffer = 0;
	}

	// Release the vertex buffer.
	if (m_vertexBuffer)
	{
		m_vertexBuffer->Release();
		m_vertexBuffer = 0;
	}

	return;
}


void Model::RenderBuffers(ID3D11DeviceContext* deviceContext)
{
	unsigned int stride;
	unsigned int offset;


	// Set vertex buffer stride and offset.
	stride = sizeof(VertexType);
	offset = 0;

	// Set the vertex buffer to active in the input assembler so it can be rendered.
	deviceContext->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);

	// Set the index buffer to active in the input assembler so it can be rendered.
	deviceContext->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	return;
}


bool Model::LoadTexture(ID3D11Device* device, ID3D11DeviceContext* deviceContext, char* filename)
{
	if (!filename) {
		// No texture to load
		m_Texture = nullptr;
		return true;
	}

	bool result;

	// Debug output for texture loading
	LOG("Attempting to load texture: " + std::string(filename));

	// Check if file exists
	FILE* file;
	errno_t err = fopen_s(&file, filename, "rb");
	if (err != 0)
	{
		LOG_ERROR("Failed to open texture file. Error code: " + std::to_string(err));
		return false;
	}
	fclose(file);
	LOG("Texture file exists and is accessible");

	// Create and initialize the texture object.
	m_Texture = new Texture;

	result = m_Texture->Initialize(device, deviceContext, filename);
	if (!result)
	{
		LOG_ERROR("Failed to initialize texture object");
		return false;
	}

	LOG("Texture loaded successfully");
	return true;
}


bool Model::LoadTextures(ID3D11Device* device, ID3D11DeviceContext* deviceContext, char* filename1, char* filename2)
{
	bool result;


	// Create and initialize the texture object array.
	m_Textures.resize(2);

	result = m_Textures[0].Initialize(device, deviceContext, filename1);
	if (!result)
	{
		return false;
	}

	result = m_Textures[1].Initialize(device, deviceContext, filename2);
	if (!result)
	{
		return false;
	}

	return true;
}


bool Model::LoadTextures(ID3D11Device* device, ID3D11DeviceContext* deviceContext, char* filename1, char* filename2, char* filename3)
{
	bool result;


	// Create and initialize the texture object array.
	m_Textures.resize(3);

	result = m_Textures[0].Initialize(device, deviceContext, filename1);
	if (!result)
	{
		return false;
	}

	result = m_Textures[1].Initialize(device, deviceContext, filename2);
	if (!result)
	{
		return false;
	}

	result = m_Textures[2].Initialize(device, deviceContext, filename3);
	if (!result)
	{
		return false;
	}

	return true;
}


void Model::ReleaseTextures()
{
	// Release the texture object array.
	for (auto& texture : m_Textures)
	{
		texture.Shutdown();
	}
	m_Textures.clear();

	// Release the texture object.
	if (m_Texture)
	{
		m_Texture->Shutdown();
		delete m_Texture;
		m_Texture = 0;
	}

	// Release PBR textures
	if (m_diffuseTexture)
	{
		m_diffuseTexture->Shutdown();
		delete m_diffuseTexture;
		m_diffuseTexture = nullptr;
	}

	if (m_normalTexture)
	{
		m_normalTexture->Shutdown();
		delete m_normalTexture;
		m_normalTexture = nullptr;
	}

	if (m_metallicTexture)
	{
		m_metallicTexture->Shutdown();
		delete m_metallicTexture;
		m_metallicTexture = nullptr;
	}

	if (m_roughnessTexture)
	{
		m_roughnessTexture->Shutdown();
		delete m_roughnessTexture;
		m_roughnessTexture = nullptr;
	}

	if (m_emissionTexture)
	{
		m_emissionTexture->Shutdown();
		delete m_emissionTexture;
		m_emissionTexture = nullptr;
	}

	if (m_aoTexture)
	{
		m_aoTexture->Shutdown();
		delete m_aoTexture;
		m_aoTexture = nullptr;
	}

	return;
}


bool Model::LoadModel(char* filename)
{
	// Check if the file is an FBX file
	std::string fileStr(filename);
	if (fileStr.substr(fileStr.find_last_of(".") + 1) == "fbx")
	{
		bool result = LoadFBXModel(filename);
		if (result)
		{
			CalculateBoundingBox();
		}
		return result;
	}
	else
	{
		bool result = LoadTextModel(filename);
		if (result)
		{
			CalculateBoundingBox();
		}
		return result;
	}
}


bool Model::LoadTextModel(char* filename)
{
	ifstream fin;
	char input;
	int i;

	// Open the model file.
	fin.open(filename);

	// If it could not open the file then exit.
	if (fin.fail())
	{
		return false;
	}

	// Read up to the value of vertex count.
	fin.get(input);
	while (input != ':')
	{
		fin.get(input);
	}

	// Read in the vertex count.
	fin >> m_vertexCount;

	// Set the number of indices to be the same as the vertex count.
	m_indexCount = m_vertexCount;

	// Create the model using the vertex count that was read in.
	m_model = new ModelType[m_vertexCount];

	// Read up to the beginning of the data.
	fin.get(input);
	while (input != ':')
	{
		fin.get(input);
	}
	fin.get(input);
	fin.get(input);

	// Read in the vertex data.
	for (i = 0; i < m_vertexCount; i++)
	{
		fin >> m_model[i].x >> m_model[i].y >> m_model[i].z;
		fin >> m_model[i].tu >> m_model[i].tv;
		fin >> m_model[i].nx >> m_model[i].ny >> m_model[i].nz;
	}

	// Close the model file.
	fin.close();

	return true;
}


bool Model::LoadFBXModel(char* filename)
{
	LOG("LoadFBXModel - Starting to load FBX file: " + std::string(filename));
	
	// Store the current FBX file path for texture searching
	m_currentFBXPath = filename;

	// Initialize the FBX SDK manager
	FbxManager* lSdkManager = FbxManager::Create();
	if (!lSdkManager)
	{
		LOG_ERROR("LoadFBXModel - Failed to create FBX SDK manager");
		return false;
	}
	LOG("LoadFBXModel - FBX SDK manager created successfully");

	// Create an IOSettings object
	FbxIOSettings* ios = FbxIOSettings::Create(lSdkManager, IOSROOT);
	lSdkManager->SetIOSettings(ios);
	LOG("LoadFBXModel - FBX IOSettings created");

	// Create an importer
	FbxImporter* lImporter = FbxImporter::Create(lSdkManager, "");
	if (!lImporter->Initialize(filename, -1, lSdkManager->GetIOSettings()))
	{
		LOG_ERROR("LoadFBXModel - Failed to initialize FBX importer");
		lImporter->Destroy();
		lSdkManager->Destroy();
		return false;
	}
	LOG("LoadFBXModel - FBX importer initialized successfully");

	// Create a new scene
	FbxScene* lScene = FbxScene::Create(lSdkManager, "myScene");
	if (!lScene)
	{
		LOG_ERROR("LoadFBXModel - Failed to create FBX scene");
		lImporter->Destroy();
		lSdkManager->Destroy();
		return false;
	}
	LOG("LoadFBXModel - FBX scene created successfully");

	// Import the scene
	LOG("LoadFBXModel - Importing scene...");
	lImporter->Import(lScene);
	lImporter->Destroy();
	LOG("LoadFBXModel - Scene imported successfully");

	// Convert the scene to triangle meshes
	LOG("LoadFBXModel - Converting scene to triangle meshes...");
	FbxGeometryConverter lGeomConverter(lSdkManager);
	lGeomConverter.Triangulate(lScene, true);
	LOG("LoadFBXModel - Scene triangulated successfully");

	// Get the root node
	FbxNode* lRootNode = lScene->GetRootNode();
	if (!lRootNode)
	{
		LOG_ERROR("LoadFBXModel - Failed to get root node");
		lScene->Destroy();
		lSdkManager->Destroy();
		return false;
	}
	LOG("LoadFBXModel - Root node obtained successfully");

	// Process the scene
	LOG("LoadFBXModel - Processing scene nodes...");
	ProcessNode(lRootNode);
	LOG("LoadFBXModel - Scene processing completed");

	// Clean up
	lScene->Destroy();
	lSdkManager->Destroy();
	LOG("LoadFBXModel - FBX loading completed successfully");

	return true;
}


void Model::ProcessNode(FbxNode* pNode)
{
	if (!pNode)
	{
		return;
	}

	// Process materials first
	ProcessMaterials(pNode);

	// Process the node's mesh if it has one
	FbxNodeAttribute* lNodeAttribute = pNode->GetNodeAttribute();
	if (lNodeAttribute && lNodeAttribute->GetAttributeType() == FbxNodeAttribute::eMesh)
	{
		ProcessMesh(pNode);
	}

	// Process all child nodes
	for (int i = 0; i < pNode->GetChildCount(); i++)
	{
		ProcessNode(pNode->GetChild(i));
	}
}


void Model::ProcessMaterials(FbxNode* pNode)
{
	if (!pNode) return;

	LOG("--- Processing Node: " + std::string(pNode->GetName()) + " ---");

	// First, try to find textures connected to materials
	int materialCount = pNode->GetMaterialCount();
	if (materialCount > 0)
	{
		for (int i = 0; i < materialCount; ++i)
		{
			if (FbxSurfaceMaterial* material = pNode->GetMaterial(i))
			{
				if (m_materialInfo.diffuseTexturePath.empty())
				{
					ExtractMaterialInfo(material);
				}
			}
		}
	}

	// If no textures found in materials, try to search the entire scene for textures
	if (m_materialInfo.diffuseTexturePath.empty())
	{
		LOG("  -> No textures found in materials, searching scene for textures...");
		SearchSceneForTextures(pNode->GetScene());
	}

	// If still no textures found, try to find textures in the same directory as the FBX file
	if (m_materialInfo.diffuseTexturePath.empty())
	{
		LOG("  -> No textures found in scene, trying to find textures in FBX directory...");
		SearchDirectoryForTextures();
	}

	if (!m_materialInfo.diffuseTexturePath.empty() || !m_materialInfo.normalTexturePath.empty())
	{
		m_hasFBXMaterial = true;
	}
}

void Model::SearchSceneForTextures(FbxScene* scene)
{
	if (!scene) return;

	LOG("    -> Searching scene for textures...");
	
	// Get all textures in the scene
	int textureCount = scene->GetTextureCount();
	LOG("    -> Found " + std::to_string(textureCount) + " textures in scene");
	
	// Store all found textures for better assignment
	vector<string> diffuseTextures;
	vector<string> normalTextures;
	vector<string> specularTextures;
	vector<string> emissionTextures;
	vector<string> metallicTextures;
	vector<string> roughnessTextures;
	vector<string> aoTextures;
	
	for (int i = 0; i < textureCount; i++)
	{
		FbxTexture* texture = scene->GetTexture(i);
		if (texture)
		{
			FbxFileTexture* fileTexture = FbxCast<FbxFileTexture>(texture);
			if (fileTexture)
			{
				string texturePath = fileTexture->GetFileName();
				LOG("    -> Found texture: " + texturePath);
				
				// Try to determine texture type from filename
				std::string filename = texturePath;
				std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);
				
				// Categorize textures by type
				if (filename.find("diffuse") != std::string::npos || 
					filename.find("color") != std::string::npos || 
					filename.find("albedo") != std::string::npos ||
					filename.find("textura-color") != std::string::npos)
				{
					diffuseTextures.push_back(texturePath);
					LOG("    -> Categorized as diffuse texture");
				}
				else if (filename.find("normal") != std::string::npos ||
						 filename.find("textura-normal") != std::string::npos)
				{
					normalTextures.push_back(texturePath);
					LOG("    -> Categorized as normal texture");
				}
				else if (filename.find("specular") != std::string::npos)
				{
					specularTextures.push_back(texturePath);
					LOG("    -> Categorized as specular texture");
				}
				else if (filename.find("glow") != std::string::npos ||
						 filename.find("emission") != std::string::npos ||
						 filename.find("textura-emission") != std::string::npos)
				{
					emissionTextures.push_back(texturePath);
					LOG("    -> Categorized as emission texture");
				}
				else if (filename.find("metallic") != std::string::npos ||
						 filename.find("textura-metallic") != std::string::npos)
				{
					metallicTextures.push_back(texturePath);
					LOG("    -> Categorized as metallic texture");
				}
				else if (filename.find("roughness") != std::string::npos ||
						 filename.find("textura-roughness") != std::string::npos)
				{
					roughnessTextures.push_back(texturePath);
					LOG("    -> Categorized as roughness texture");
				}
				else if (filename.find("ao") != std::string::npos ||
						 filename.find("ambient") != std::string::npos)
				{
					aoTextures.push_back(texturePath);
					LOG("    -> Categorized as AO texture");
				}
				else
				{
					LOG("    -> Could not categorize texture: " + texturePath);
				}
			}
		}
	}
	
	// Assign the first texture of each type found
	if (!diffuseTextures.empty() && m_materialInfo.diffuseTexturePath.empty())
	{
		m_materialInfo.diffuseTexturePath = diffuseTextures[0];
		LOG("    -> Assigned diffuse texture: " + diffuseTextures[0]);
		if (diffuseTextures.size() > 1)
		{
			LOG("    -> Note: " + std::to_string(diffuseTextures.size()) + " diffuse textures found, using first one");
		}
	}
	
	if (!normalTextures.empty() && m_materialInfo.normalTexturePath.empty())
	{
		m_materialInfo.normalTexturePath = normalTextures[0];
		LOG("    -> Assigned normal texture: " + normalTextures[0]);
	}
	
	if (!specularTextures.empty() && m_materialInfo.specularTexturePath.empty())
	{
		m_materialInfo.specularTexturePath = specularTextures[0];
		LOG("    -> Assigned specular texture: " + specularTextures[0]);
	}
	
	if (!emissionTextures.empty() && m_materialInfo.emissionTexturePath.empty())
	{
		m_materialInfo.emissionTexturePath = emissionTextures[0];
		LOG("    -> Assigned emission texture: " + emissionTextures[0]);
	}
	
	if (!metallicTextures.empty() && m_materialInfo.metallicTexturePath.empty())
	{
		m_materialInfo.metallicTexturePath = metallicTextures[0];
		LOG("    -> Assigned metallic texture: " + metallicTextures[0]);
	}
	
	if (!roughnessTextures.empty() && m_materialInfo.roughnessTexturePath.empty())
	{
		m_materialInfo.roughnessTexturePath = roughnessTextures[0];
		LOG("    -> Assigned roughness texture: " + roughnessTextures[0]);
	}
	
	if (!aoTextures.empty() && m_materialInfo.aoTexturePath.empty())
	{
		m_materialInfo.aoTexturePath = aoTextures[0];
		LOG("    -> Assigned AO texture: " + aoTextures[0]);
	}
}

void Model::SearchDirectoryForTextures()
{
	// Get the directory of the FBX file
	string fbxPath = m_currentFBXPath;
	size_t lastSlash = fbxPath.find_last_of("/\\");
	if (lastSlash == string::npos) return;
	
	string fbxDir = fbxPath.substr(0, lastSlash + 1);
	string texturesDir = fbxDir + "textures/";
	
	LOG("    -> Looking for textures in: " + texturesDir);
	
	// Check if textures directory exists and look for common texture files
	vector<string> possibleTextures = {
		"color.png",
		"color.tga",
		"color.jpg",
		"diffuse.png",
		"diffuse.tga",
		"diffuse.jpg",
		"albedo.png",
		"albedo.tga",
		"albedo.jpg"
	};
	
	for (const string& textureName : possibleTextures)
	{
		string fullPath = texturesDir + textureName;
		FILE* file;
		errno_t err = fopen_s(&file, fullPath.c_str(), "rb");
		if (err == 0)
		{
			fclose(file);
			if (m_materialInfo.diffuseTexturePath.empty())
			{
				m_materialInfo.diffuseTexturePath = fullPath;
				LOG("    -> Found diffuse texture: " + fullPath);
				break;
			}
		}
	}
	
	// Look for normal maps
	vector<string> normalTextures = {
		"normal.png",
		"normal.tga",
		"normal.jpg"
	};
	
	for (const string& textureName : normalTextures)
	{
		string fullPath = texturesDir + textureName;
		FILE* file;
		errno_t err = fopen_s(&file, fullPath.c_str(), "rb");
		if (err == 0)
		{
			fclose(file);
			if (m_materialInfo.normalTexturePath.empty())
			{
				m_materialInfo.normalTexturePath = fullPath;
				LOG("    -> Found normal texture: " + fullPath);
				break;
			}
		}
	}
	
	// Look for metallic textures
	vector<string> metallicTextures = {
		"metallic.png",
		"metallic.tga",
		"metallic.jpg"
	};
	
	for (const string& textureName : metallicTextures)
	{
		string fullPath = texturesDir + textureName;
		FILE* file;
		errno_t err = fopen_s(&file, fullPath.c_str(), "rb");
		if (err == 0)
		{
			fclose(file);
			if (m_materialInfo.metallicTexturePath.empty())
			{
				m_materialInfo.metallicTexturePath = fullPath;
				LOG("    -> Found metallic texture: " + fullPath);
				break;
			}
		}
	}
	
	// Look for roughness textures
	vector<string> roughnessTextures = {
		"roughness.png",
		"roughness.tga",
		"roughness.jpg"
	};
	
	for (const string& textureName : roughnessTextures)
	{
		string fullPath = texturesDir + textureName;
		FILE* file;
		errno_t err = fopen_s(&file, fullPath.c_str(), "rb");
		if (err == 0)
		{
			fclose(file);
			if (m_materialInfo.roughnessTexturePath.empty())
			{
				m_materialInfo.roughnessTexturePath = fullPath;
				LOG("    -> Found roughness texture: " + fullPath);
				break;
			}
		}
	}
	
	// Look for emission textures
	vector<string> emissionTextures = {
		"emission.png",
		"emission.tga",
		"emission.jpg"
	};
	
	for (const string& textureName : emissionTextures)
	{
		string fullPath = texturesDir + textureName;
		FILE* file;
		errno_t err = fopen_s(&file, fullPath.c_str(), "rb");
		if (err == 0)
		{
			fclose(file);
			if (m_materialInfo.emissionTexturePath.empty())
			{
				m_materialInfo.emissionTexturePath = fullPath;
				LOG("    -> Found emission texture: " + fullPath);
				break;
			}
		}
	}
	
	// Look for AO (Ambient Occlusion) textures
	vector<string> aoTextures = {
		"internal_ground_ao_texture.jpeg",
		"ao.png",
		"ao.tga",
		"ao.jpg"
	};
	
	for (const string& textureName : aoTextures)
	{
		string fullPath = texturesDir + textureName;
		FILE* file;
		errno_t err = fopen_s(&file, fullPath.c_str(), "rb");
		if (err == 0)
		{
			fclose(file);
			if (m_materialInfo.aoTexturePath.empty())
			{
				m_materialInfo.aoTexturePath = fullPath;
				LOG("    -> Found AO texture: " + fullPath);
				break;
			}
		}
	}
}

void Model::ListAllMaterialProperties(FbxSurfaceMaterial* material)
{
	if (!material)
		return;

	LOG("=== Listing All Material Properties ===");
	
	// Get all properties from the material
	FbxProperty prop = material->GetFirstProperty();
	while (prop.IsValid())
	{
		string propName = prop.GetName().Buffer();
		string propType = prop.GetPropertyDataType().GetName();
		
		LOG("Property: " + propName + " (Type: " + propType + ")");
		
		// Check if this property has textures
		int textureCount = prop.GetSrcObjectCount<FbxTexture>();
		if (textureCount > 0)
		{
			LOG("  -> Has " + std::to_string(textureCount) + " texture(s)");
			for (int i = 0; i < textureCount; i++)
			{
				FbxTexture* texture = prop.GetSrcObject<FbxTexture>(i);
				if (texture)
				{
					FbxFileTexture* fileTexture = FbxCast<FbxFileTexture>(texture);
					if (fileTexture)
					{
						string texturePath = fileTexture->GetFileName();
						LOG("  -> Texture " + std::to_string(i) + ": " + texturePath);
					}
					else
					{
						LOG("  -> Texture " + std::to_string(i) + ": (not a file texture)");
					}
				}
			}
		}
		
		prop = material->GetNextProperty(prop);
	}
	
	LOG("=== End Material Properties ===");
}

void Model::ExtractMaterialInfo(FbxSurfaceMaterial* material)
{
	if (!material)
		return;

	LOG("=== Extracting Material Info ===");
	LOG("Material name: " + std::string(material->GetName()));
	LOG("Material type: " + std::string(material->GetClassId().GetName()));

	// List all properties first for debugging
	ListAllMaterialProperties(material);

	// Get the material type
	FbxSurfacePhong* phongMaterial = nullptr;
	FbxSurfaceLambert* lambertMaterial = nullptr;

	if (material->GetClassId().Is(FbxSurfacePhong::ClassId))
	{
		phongMaterial = (FbxSurfacePhong*)material;
		LOG("Material type: Phong");
	}
	else if (material->GetClassId().Is(FbxSurfaceLambert::ClassId))
	{
		lambertMaterial = (FbxSurfaceLambert*)material;
		LOG("Material type: Lambert");
	}

	// Extract diffuse color
	if (phongMaterial)
	{
		FbxDouble3 diffuse = phongMaterial->Diffuse.Get();
		m_materialInfo.diffuseColor = XMFLOAT4((float)diffuse[0], (float)diffuse[1], (float)diffuse[2], 1.0f);
		LOG("Diffuse color: " + std::to_string(diffuse[0]) + ", " + std::to_string(diffuse[1]) + ", " + std::to_string(diffuse[2]));
	}
	else if (lambertMaterial)
	{
		FbxDouble3 diffuse = lambertMaterial->Diffuse.Get();
		m_materialInfo.diffuseColor = XMFLOAT4((float)diffuse[0], (float)diffuse[1], (float)diffuse[2], 1.0f);
		LOG("Diffuse color: " + std::to_string(diffuse[0]) + ", " + std::to_string(diffuse[1]) + ", " + std::to_string(diffuse[2]));
	}

	// Extract ambient color
	if (phongMaterial)
	{
		FbxDouble3 ambient = phongMaterial->Ambient.Get();
		m_materialInfo.ambientColor = XMFLOAT4((float)ambient[0], (float)ambient[1], (float)ambient[2], 1.0f);
		LOG("Ambient color: " + std::to_string(ambient[0]) + ", " + std::to_string(ambient[1]) + ", " + std::to_string(ambient[2]));
	}
	else if (lambertMaterial)
	{
		FbxDouble3 ambient = lambertMaterial->Ambient.Get();
		m_materialInfo.ambientColor = XMFLOAT4((float)ambient[0], (float)ambient[1], (float)ambient[2], 1.0f);
		LOG("Ambient color: " + std::to_string(ambient[0]) + ", " + std::to_string(ambient[1]) + ", " + std::to_string(ambient[2]));
	}

	// Extract specular color and shininess (Phong only)
	if (phongMaterial)
	{
		FbxDouble3 specular = phongMaterial->Specular.Get();
		m_materialInfo.specularColor = XMFLOAT4((float)specular[0], (float)specular[1], (float)specular[2], 1.0f);
		m_materialInfo.shininess = (float)phongMaterial->Shininess.Get();
		LOG("Specular color: " + std::to_string(specular[0]) + ", " + std::to_string(specular[1]) + ", " + std::to_string(specular[2]));
		LOG("Shininess: " + std::to_string(m_materialInfo.shininess));
	}

	// Brute-force search for textures by iterating all properties
	LOG("=== Starting Brute-Force Texture Search ===");
	FbxProperty prop = material->GetFirstProperty();
	while(prop.IsValid())
	{
		const FbxFileTexture* foundTexture = FindConnectedFileTexture(prop);
		if(foundTexture)
		{
			LOG("  ----> SUCCESS! Found a texture!");
			LOG("    -> Property Name: " + std::string(prop.GetName()));
			LOG("    -> Texture Path: " + std::string(foundTexture->GetFileName()));

			// Naive assignment based on filename. We'll refine this later.
			std::string filename = foundTexture->GetFileName();
			std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);

			if (m_materialInfo.diffuseTexturePath.empty() && (filename.find("color") != std::string::npos || filename.find("albedo") != std::string::npos || filename.find("diffuse") != std::string::npos))
			{
				m_materialInfo.diffuseTexturePath = foundTexture->GetFileName();
			}
			if (m_materialInfo.normalTexturePath.empty() && filename.find("normal") != std::string::npos)
			{
				m_materialInfo.normalTexturePath = foundTexture->GetFileName();
			}
			if (m_materialInfo.specularTexturePath.empty() && (filename.find("specular") != std::string::npos || filename.find("metallic") != std::string::npos))
			{
				m_materialInfo.specularTexturePath = foundTexture->GetFileName();
			}
		}
		prop = material->GetNextProperty(prop);
	}
	
	LOG("Diffuse texture: " + (m_materialInfo.diffuseTexturePath.empty() ? "NOT FOUND" : m_materialInfo.diffuseTexturePath));
	LOG("Normal texture: " + (m_materialInfo.normalTexturePath.empty() ? "NOT FOUND" : m_materialInfo.normalTexturePath));
	LOG("Specular texture: " + (m_materialInfo.specularTexturePath.empty() ? "NOT FOUND" : m_materialInfo.specularTexturePath));

	LOG("=== End Material Info ===");
}


const FbxFileTexture* Model::FindConnectedFileTexture(const FbxProperty& property)
{
    if (!property.IsValid())
    {
        return nullptr;
    }

	LOG("  -> Searching property: " + std::string(property.GetName()));

    // Check for direct file texture connection
    int fileTextureCount = property.GetSrcObjectCount<FbxFileTexture>();
    if (fileTextureCount > 0)
    {
        const FbxFileTexture* fileTexture = property.GetSrcObject<FbxFileTexture>(0);
		LOG("    --> Found a direct file texture connection: " + std::string(fileTexture->GetFileName()));
        return fileTexture;
    }

    // Check for layered texture connection
    int layeredTextureCount = property.GetSrcObjectCount<FbxLayeredTexture>();
    if (layeredTextureCount > 0)
    {
        const FbxLayeredTexture* layeredTexture = property.GetSrcObject<FbxLayeredTexture>(0);
        if (layeredTexture && layeredTexture->GetSrcObjectCount<FbxFileTexture>() > 0)
        {
            const FbxFileTexture* fileTexture = layeredTexture->GetSrcObject<FbxFileTexture>(0);
			LOG("    --> Found a layered texture connection: " + std::string(fileTexture->GetFileName()));
            return fileTexture;
        }
    }

    // If no direct texture, search upstream in the connection graph
    int srcObjectCount = property.GetSrcObjectCount<FbxObject>();
    if (srcObjectCount > 0)
    {
		LOG("    --> Property is connected to " + std::to_string(srcObjectCount) + " upstream object(s). Traversing...");
        for (int i = 0; i < srcObjectCount; ++i)
        {
            const FbxObject* srcObject = property.GetSrcObject<FbxObject>(i);
            if (srcObject)
            {
				LOG("      --> Checking connected object: " + std::string(srcObject->GetName()));
                // Recursively search the properties of the connected object
                FbxProperty srcProp = srcObject->GetFirstProperty();
                while (srcProp.IsValid())
                {
                    const FbxFileTexture* texture = FindConnectedFileTexture(srcProp);
                    if (texture)
                    {
						LOG("        --> Found texture through recursion!");
                        return texture;
                    }
                    srcProp = srcObject->GetNextProperty(srcProp);
                }
            }
        }
    }

    return nullptr;
}


void Model::ProcessMesh(FbxNode* pNode)
{
	FbxMesh* lMesh = pNode->GetMesh();
	if (!lMesh)
	{
		return;
	}

	// Temporary storage for all vertices and indices
	static std::vector<ModelType> vertices;
	static std::vector<unsigned long> indices;

	// Get the control points (positions)
	FbxVector4* lControlPoints = lMesh->GetControlPoints();

	// Get the first UV set name
	FbxStringList lUVSetNameList;
	lMesh->GetUVSetNames(lUVSetNameList);
	const char* uvSetName = lUVSetNameList.GetCount() > 0 ? lUVSetNameList.GetStringAt(0) : nullptr;

	int polygonCount = lMesh->GetPolygonCount();
	for (int polyIdx = 0; polyIdx < polygonCount; ++polyIdx)
	{
		int polySize = lMesh->GetPolygonSize(polyIdx);
		// Triangulated, so should always be 3
		for (int vertIdx = 0; vertIdx < polySize; ++vertIdx)
		{
			ModelType v = {};
			int ctrlPointIdx = lMesh->GetPolygonVertex(polyIdx, vertIdx);
			FbxVector4 pos = lControlPoints[ctrlPointIdx];
			v.x = (float)pos[0];
			v.y = (float)pos[1];
			v.z = (float)pos[2];

			// Normal (per polygon-vertex)
			FbxVector4 normal;
			lMesh->GetPolygonVertexNormal(polyIdx, vertIdx, normal);
			v.nx = (float)normal[0];
			v.ny = (float)normal[1];
			v.nz = (float)normal[2];

			// UV (per polygon-vertex)
			v.tu = 0.0f;
			v.tv = 0.0f;
			if (uvSetName)
			{
				FbxVector2 uv;
				bool unmapped;
				if (lMesh->GetPolygonVertexUV(polyIdx, vertIdx, uvSetName, uv, unmapped))
				{
					v.tu = (float)uv[0];
					v.tv = (float)uv[1];
				}
			}

			// Add vertex and index
			vertices.push_back(v);
			indices.push_back((unsigned long)vertices.size() - 1);
		}
	}

	// After all meshes are processed, copy to m_model (do this only once, after all ProcessMesh calls)
	// We'll do this in LoadFBXModel after ProcessNode
	m_vertexCount = (int)vertices.size();
	m_indexCount = (int)indices.size();

	if (m_model)
	{
		delete[] m_model;
	}

	m_model = new ModelType[m_vertexCount];
	for (int i = 0; i < m_vertexCount; ++i)
	{
		m_model[i] = vertices[i];
	}
		
	// Optionally, you can store indices if you want to use them elsewhere
	// But your InitializeBuffers currently assumes index = vertex order
	// So this is fine for now

	// Clear static vectors for next model load
	if (pNode->GetParent() == nullptr) // root node, last call
	{
		vertices.clear();
		indices.clear();
	}
}


void Model::ReleaseModel()
{
	if (m_model)
	{
		delete[] m_model;
		m_model = 0;
	}

	return;
}

void Model::CalculateModelVectors()
{
	int faceCount, i, index;
	TempVertexType vertex1, vertex2, vertex3;
	VectorType tangent, binormal;


	// Calculate the number of faces in the model.
	faceCount = m_vertexCount / 3;

	// Initialize the index to the model data.
	index = 0;

	// Go through all the faces and calculate the the tangent and binormal vectors.
	for (i = 0; i < faceCount; i++)
	{
		// Get the three vertices for this face from the model.
		vertex1.x = m_model[index].x;
		vertex1.y = m_model[index].y;
		vertex1.z = m_model[index].z;
		vertex1.tu = m_model[index].tu;
		vertex1.tv = m_model[index].tv;
		index++;

		vertex2.x = m_model[index].x;
		vertex2.y = m_model[index].y;
		vertex2.z = m_model[index].z;
		vertex2.tu = m_model[index].tu;
		vertex2.tv = m_model[index].tv;
		index++;

		vertex3.x = m_model[index].x;
		vertex3.y = m_model[index].y;
		vertex3.z = m_model[index].z;
		vertex3.tu = m_model[index].tu;
		vertex3.tv = m_model[index].tv;
		index++;

		// Calculate the tangent and binormal of that face.
		CalculateTangentBinormal(vertex1, vertex2, vertex3, tangent, binormal);

		// Store the tangent and binormal for this face back in the model structure.
		m_model[index - 1].tx = tangent.x;
		m_model[index - 1].ty = tangent.y;
		m_model[index - 1].tz = tangent.z;
		m_model[index - 1].bx = binormal.x;
		m_model[index - 1].by = binormal.y;
		m_model[index - 1].bz = binormal.z;

		m_model[index - 2].tx = tangent.x;
		m_model[index - 2].ty = tangent.y;
		m_model[index - 2].tz = tangent.z;
		m_model[index - 2].bx = binormal.x;
		m_model[index - 2].by = binormal.y;
		m_model[index - 2].bz = binormal.z;

		m_model[index - 3].tx = tangent.x;
		m_model[index - 3].ty = tangent.y;
		m_model[index - 3].tz = tangent.z;
		m_model[index - 3].bx = binormal.x;
		m_model[index - 3].by = binormal.y;
		m_model[index - 3].bz = binormal.z;
	}

	return;
}

void Model::CalculateTangentBinormal(TempVertexType vertex1, TempVertexType vertex2, TempVertexType vertex3, VectorType& tangent, VectorType& binormal)
{
	float vector1[3], vector2[3];
	float tuVector[2], tvVector[2];
	float den;
	float length;


	// Calculate the two vectors for this face.
	vector1[0] = vertex2.x - vertex1.x;
	vector1[1] = vertex2.y - vertex1.y;
	vector1[2] = vertex2.z - vertex1.z;

	vector2[0] = vertex3.x - vertex1.x;
	vector2[1] = vertex3.y - vertex1.y;
	vector2[2] = vertex3.z - vertex1.z;

	// Calculate the tu and tv texture space vectors.
	tuVector[0] = vertex2.tu - vertex1.tu;
	tvVector[0] = vertex2.tv - vertex1.tv;

	tuVector[1] = vertex3.tu - vertex1.tu;
	tvVector[1] = vertex3.tv - vertex1.tv;

	// Calculate the denominator of the tangent/binormal equation.
	den = 1.0f / (tuVector[0] * tvVector[1] - tuVector[1] * tvVector[0]);

	// Calculate the cross products and multiply by the coefficient to get the tangent and binormal.
	tangent.x = (tvVector[1] * vector1[0] - tvVector[0] * vector2[0]) * den;
	tangent.y = (tvVector[1] * vector1[1] - tvVector[0] * vector2[1]) * den;
	tangent.z = (tvVector[1] * vector1[2] - tvVector[0] * vector2[2]) * den;

	binormal.x = (tuVector[0] * vector2[0] - tuVector[1] * vector1[0]) * den;
	binormal.y = (tuVector[0] * vector2[1] - tuVector[1] * vector1[1]) * den;
	binormal.z = (tuVector[0] * vector2[2] - tuVector[1] * vector1[2]) * den;

	// Calculate the length of this normal.
	length = sqrt((tangent.x * tangent.x) + (tangent.y * tangent.y) + (tangent.z * tangent.z));

	// Normalize the normal and then store it
	tangent.x = tangent.x / length;
	tangent.y = tangent.y / length;
	tangent.z = tangent.z / length;

	// Calculate the length of this normal.
	length = sqrt((binormal.x * binormal.x) + (binormal.y * binormal.y) + (binormal.z * binormal.z));

	// Normalize the normal and then store it
	binormal.x = binormal.x / length;
	binormal.y = binormal.y / length;
	binormal.z = binormal.z / length;

	return;
}

void Model::CalculateBoundingBox()
{
	if (!m_model || m_vertexCount == 0)
	{
		// Set default values if no model data
		m_boundingBox.min = XMFLOAT3(0.0f, 0.0f, 0.0f);
		m_boundingBox.max = XMFLOAT3(0.0f, 0.0f, 0.0f);
		m_boundingBox.radius = 0.0f;
		LOG_WARNING("CalculateBoundingBox - No model data available, using default bounding box");
		return;
	}

	LOG("CalculateBoundingBox - Calculating bounding box for " + std::to_string(m_vertexCount) + " vertices");

	// Initialize min/max with the first vertex
	m_boundingBox.min = XMFLOAT3(m_model[0].x, m_model[0].y, m_model[0].z);
	m_boundingBox.max = XMFLOAT3(m_model[0].x, m_model[0].y, m_model[0].z);

	// Find the min/max bounds
	for (int i = 1; i < m_vertexCount; i++)
	{
		// Update min bounds
		m_boundingBox.min.x = min(m_boundingBox.min.x, m_model[i].x);
		m_boundingBox.min.y = min(m_boundingBox.min.y, m_model[i].y);
		m_boundingBox.min.z = min(m_boundingBox.min.z, m_model[i].z);

		// Update max bounds
		m_boundingBox.max.x = max(m_boundingBox.max.x, m_model[i].x);
		m_boundingBox.max.y = max(m_boundingBox.max.y, m_model[i].y);
		m_boundingBox.max.z = max(m_boundingBox.max.z, m_model[i].z);
	}

	// Calculate the radius for sphere culling (distance from center to furthest point)
	XMFLOAT3 center;
	center.x = (m_boundingBox.min.x + m_boundingBox.max.x) * 0.5f;
	center.y = (m_boundingBox.min.y + m_boundingBox.max.y) * 0.5f;
	center.z = (m_boundingBox.min.z + m_boundingBox.max.z) * 0.5f;

	float maxDistSq = 0.0f;
	for (int i = 0; i < m_vertexCount; i++)
	{
		float dx = m_model[i].x - center.x;
		float dy = m_model[i].y - center.y;
		float dz = m_model[i].z - center.z;
		float distSq = dx * dx + dy * dy + dz * dz;
		maxDistSq = max(maxDistSq, distSq);
	}

	m_boundingBox.radius = sqrt(maxDistSq);
	
	LOG("CalculateBoundingBox - Bounding box calculated:");
	LOG("  Min: (" + std::to_string(m_boundingBox.min.x) + ", " + std::to_string(m_boundingBox.min.y) + ", " + std::to_string(m_boundingBox.min.z) + ")");
	LOG("  Max: (" + std::to_string(m_boundingBox.max.x) + ", " + std::to_string(m_boundingBox.max.y) + ", " + std::to_string(m_boundingBox.max.z) + ")");
	LOG("  Center: (" + std::to_string(center.x) + ", " + std::to_string(center.y) + ", " + std::to_string(center.z) + ")");
	LOG("  Radius: " + std::to_string(m_boundingBox.radius));
}

bool Model::LoadFBXTextures(ID3D11Device* device, ID3D11DeviceContext* deviceContext)
{
	bool result = true;
	int loadedTextures = 0;

	LOG("=== FBX Texture Loading Report ===");
	LOG("Diffuse texture: " + (m_materialInfo.diffuseTexturePath.empty() ? "NOT FOUND" : m_materialInfo.diffuseTexturePath));
	LOG("Normal texture: " + (m_materialInfo.normalTexturePath.empty() ? "NOT FOUND" : m_materialInfo.normalTexturePath));
	LOG("Specular texture: " + (m_materialInfo.specularTexturePath.empty() ? "NOT FOUND" : m_materialInfo.specularTexturePath));
	LOG("Metallic texture: " + (m_materialInfo.metallicTexturePath.empty() ? "NOT FOUND" : m_materialInfo.metallicTexturePath));
	LOG("Roughness texture: " + (m_materialInfo.roughnessTexturePath.empty() ? "NOT FOUND" : m_materialInfo.roughnessTexturePath));
	LOG("Emission texture: " + (m_materialInfo.emissionTexturePath.empty() ? "NOT FOUND" : m_materialInfo.emissionTexturePath));
	LOG("AO texture: " + (m_materialInfo.aoTexturePath.empty() ? "NOT FOUND" : m_materialInfo.aoTexturePath));

	// Load diffuse texture (most important)
	if (!m_materialInfo.diffuseTexturePath.empty())
	{
		string convertedPath = ConvertTexturePath(m_materialInfo.diffuseTexturePath);
		LOG("Attempting to load diffuse texture: " + convertedPath);
		
		m_diffuseTexture = new Texture();
		result = m_diffuseTexture->Initialize(device, deviceContext, (char*)convertedPath.c_str());
		if (result)
		{
			LOG("✓ Successfully loaded diffuse texture");
			loadedTextures++;
		}
		else
		{
			LOG_ERROR("✗ Failed to load diffuse texture: " + convertedPath);
			delete m_diffuseTexture;
			m_diffuseTexture = nullptr;
		}
	}

	// Load normal texture
	if (!m_materialInfo.normalTexturePath.empty())
	{
		string convertedPath = ConvertTexturePath(m_materialInfo.normalTexturePath);
		LOG("Attempting to load normal texture: " + convertedPath);
		
		m_normalTexture = new Texture();
		result = m_normalTexture->Initialize(device, deviceContext, (char*)convertedPath.c_str());
		if (result)
		{
			LOG("✓ Successfully loaded normal texture");
			loadedTextures++;
		}
		else
		{
			LOG_ERROR("✗ Failed to load normal texture: " + convertedPath);
			delete m_normalTexture;
			m_normalTexture = nullptr;
		}
	}

	// Load metallic texture
	if (!m_materialInfo.metallicTexturePath.empty())
	{
		string convertedPath = ConvertTexturePath(m_materialInfo.metallicTexturePath);
		LOG("Attempting to load metallic texture: " + convertedPath);
		
		m_metallicTexture = new Texture();
		result = m_metallicTexture->Initialize(device, deviceContext, (char*)convertedPath.c_str());
		if (result)
		{
			LOG("✓ Successfully loaded metallic texture");
			loadedTextures++;
		}
		else
		{
			LOG_ERROR("✗ Failed to load metallic texture: " + convertedPath);
			delete m_metallicTexture;
			m_metallicTexture = nullptr;
		}
	}

	// Load roughness texture
	if (!m_materialInfo.roughnessTexturePath.empty())
	{
		string convertedPath = ConvertTexturePath(m_materialInfo.roughnessTexturePath);
		LOG("Attempting to load roughness texture: " + convertedPath);
		
		m_roughnessTexture = new Texture();
		result = m_roughnessTexture->Initialize(device, deviceContext, (char*)convertedPath.c_str());
		if (result)
		{
			LOG("✓ Successfully loaded roughness texture");
			loadedTextures++;
		}
		else
		{
			LOG_ERROR("✗ Failed to load roughness texture: " + convertedPath);
			delete m_roughnessTexture;
			m_roughnessTexture = nullptr;
		}
	}

	// Load emission texture
	if (!m_materialInfo.emissionTexturePath.empty())
	{
		string convertedPath = ConvertTexturePath(m_materialInfo.emissionTexturePath);
		LOG("Attempting to load emission texture: " + convertedPath);
		
		m_emissionTexture = new Texture();
		result = m_emissionTexture->Initialize(device, deviceContext, (char*)convertedPath.c_str());
		if (result)
		{
			LOG("✓ Successfully loaded emission texture");
			loadedTextures++;
		}
		else
		{
			LOG_ERROR("✗ Failed to load emission texture: " + convertedPath);
			delete m_emissionTexture;
			m_emissionTexture = nullptr;
		}
	}

	// Load AO texture
	if (!m_materialInfo.aoTexturePath.empty())
	{
		string convertedPath = ConvertTexturePath(m_materialInfo.aoTexturePath);
		LOG("Attempting to load AO texture: " + convertedPath);
		
		m_aoTexture = new Texture();
		result = m_aoTexture->Initialize(device, deviceContext, (char*)convertedPath.c_str());
		if (result)
		{
			LOG("✓ Successfully loaded AO texture");
			loadedTextures++;
		}
		else
		{
			LOG_ERROR("✗ Failed to load AO texture: " + convertedPath);
			delete m_aoTexture;
			m_aoTexture = nullptr;
		}
	}

	LOG("Total textures loaded: " + std::to_string(loadedTextures));
	LOG("=== End FBX Texture Loading Report ===");

	// Don't fail if no textures were loaded - just log a warning
	if (loadedTextures == 0)
	{
		LOG_WARNING("No textures were loaded from FBX materials");
	}

	return true; // Always return true for now to see the debug output
}

string Model::ConvertTexturePath(const string& originalPath)
{
	string texturePath = originalPath;
	
	// Try to find the Engine/assets/ pattern
	size_t pos = texturePath.find("Engine\\assets\\");
	if (pos != string::npos)
	{
		// Extract just the assets/models/... part
		texturePath = "../" + texturePath.substr(pos);
		// Replace backslashes with forward slashes
		replace(texturePath.begin(), texturePath.end(), '\\', '/');
		LOG("Converted absolute path to relative: " + texturePath);
	}
	else
	{
		// If it's already a relative path, just normalize slashes
		replace(texturePath.begin(), texturePath.end(), '\\', '/');
		LOG("Normalized path slashes: " + texturePath);
	}

	return texturePath;
}