#include "model.h"
#include <algorithm>

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
}


Model::Model(const Model& other)
{
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

	// Load the texture for this model.
	if (m_hasFBXMaterial && !m_materialInfo.diffuseTexturePath.empty())
	{
		OutputDebugStringA(("Using FBX texture: " + m_materialInfo.diffuseTexturePath + "\n").c_str());
		
		// Convert the texture path to a relative path if it's absolute
		string texturePath = m_materialInfo.diffuseTexturePath;
		size_t pos = texturePath.find("Engine\\assets\\");
		if (pos != string::npos)
		{
			// Extract just the assets/models/... part
			texturePath = "../" + texturePath.substr(pos);
			// Replace backslashes with forward slashes
			replace(texturePath.begin(), texturePath.end(), '\\', '/');
			OutputDebugStringA(("Converted texture path: " + texturePath + "\n").c_str());
		}

		// Use the FBX texture path
		result = LoadTexture(device, deviceContext, (char*)texturePath.c_str());
		if (!result)
		{
			OutputDebugStringA("Failed to load FBX texture, falling back to default texture\n");
			result = LoadTexture(device, deviceContext, textureFilename);
		}
	}
	else
	{
		OutputDebugStringA(("Using fallback texture: " + std::string(textureFilename) + "\n").c_str());
		// Use the provided texture
		result = LoadTexture(device, deviceContext, textureFilename);
	}

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


int Model::GetIndexCount()
{
	return m_indexCount;
}


ID3D11ShaderResourceView* Model::GetTexture()
{
	return m_Texture->GetTexture();
}


ID3D11ShaderResourceView* Model::GetTexture(int index)
{
	return m_Textures[index].GetTexture();
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
	bool result;

	// Debug output for texture loading
	OutputDebugStringA(("Attempting to load texture: " + std::string(filename) + "\n").c_str());

	// Check if file exists
	FILE* file;
	errno_t err = fopen_s(&file, filename, "rb");
	if (err != 0)
	{
		OutputDebugStringA(("Failed to open texture file. Error code: " + std::to_string(err) + "\n").c_str());
		return false;
	}
	fclose(file);
	OutputDebugStringA("Texture file exists and is accessible\n");

	// Create and initialize the texture object.
	m_Texture = new Texture;

	result = m_Texture->Initialize(device, deviceContext, filename);
	if (!result)
	{
		OutputDebugStringA("Failed to initialize texture object\n");
		return false;
	}

	OutputDebugStringA("Texture loaded successfully\n");
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

	return;
}


bool Model::LoadModel(char* filename)
{
	// Check if the file is an FBX file
	std::string fileStr(filename);
	if (fileStr.substr(fileStr.find_last_of(".") + 1) == "fbx")
	{
		return LoadFBXModel(filename);
	}
	else
	{
		return LoadTextModel(filename);
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
	// Initialize the FBX SDK manager
	FbxManager* lSdkManager = FbxManager::Create();
	if (!lSdkManager)
	{
		return false;
	}

	// Create an IOSettings object
	FbxIOSettings* ios = FbxIOSettings::Create(lSdkManager, IOSROOT);
	lSdkManager->SetIOSettings(ios);

	// Create an importer
	FbxImporter* lImporter = FbxImporter::Create(lSdkManager, "");
	if (!lImporter->Initialize(filename, -1, lSdkManager->GetIOSettings()))
	{
		lImporter->Destroy();
		lSdkManager->Destroy();
		return false;
	}

	// Create a new scene
	FbxScene* lScene = FbxScene::Create(lSdkManager, "myScene");
	if (!lScene)
	{
		lImporter->Destroy();
		lSdkManager->Destroy();
		return false;
	}

	// Import the scene
	lImporter->Import(lScene);
	lImporter->Destroy();

	// Convert the scene to triangle meshes
	FbxGeometryConverter lGeomConverter(lSdkManager);
	lGeomConverter.Triangulate(lScene, true);

	// Get the root node
	FbxNode* lRootNode = lScene->GetRootNode();
	if (!lRootNode)
	{
		lScene->Destroy();
		lSdkManager->Destroy();
		return false;
	}

	// Process the scene
	ProcessNode(lRootNode);

	// Clean up
	lScene->Destroy();
	lSdkManager->Destroy();

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
	if (!pNode)
		return;

	// Get the number of materials
	int materialCount = pNode->GetMaterialCount();
	OutputDebugStringA(("Number of materials found: " + std::to_string(materialCount) + "\n").c_str());

	if (materialCount > 0)
	{
		// Get the first material
		FbxSurfaceMaterial* material = pNode->GetMaterial(0);
		if (material)
		{
			OutputDebugStringA(("Material name: " + std::string(material->GetName()) + "\n").c_str());
			ExtractMaterialInfo(material);
			m_hasFBXMaterial = true;
		}
	}
}


void Model::ExtractMaterialInfo(FbxSurfaceMaterial* material)
{
	if (!material)
		return;

	OutputDebugStringA("Extracting material info...\n");

	// Get the material type
	FbxSurfacePhong* phongMaterial = nullptr;
	FbxSurfaceLambert* lambertMaterial = nullptr;

	if (material->GetClassId().Is(FbxSurfacePhong::ClassId))
	{
		phongMaterial = (FbxSurfacePhong*)material;
		OutputDebugStringA("Material type: Phong\n");
	}
	else if (material->GetClassId().Is(FbxSurfaceLambert::ClassId))
	{
		lambertMaterial = (FbxSurfaceLambert*)material;
		OutputDebugStringA("Material type: Lambert\n");
	}

	// Extract diffuse color
	if (phongMaterial)
	{
		FbxDouble3 diffuse = phongMaterial->Diffuse.Get();
		m_materialInfo.diffuseColor = XMFLOAT4((float)diffuse[0], (float)diffuse[1], (float)diffuse[2], 1.0f);
		OutputDebugStringA(("Diffuse color: " + std::to_string(diffuse[0]) + ", " + std::to_string(diffuse[1]) + ", " + std::to_string(diffuse[2]) + "\n").c_str());
	}
	else if (lambertMaterial)
	{
		FbxDouble3 diffuse = lambertMaterial->Diffuse.Get();
		m_materialInfo.diffuseColor = XMFLOAT4((float)diffuse[0], (float)diffuse[1], (float)diffuse[2], 1.0f);
		OutputDebugStringA(("Diffuse color: " + std::to_string(diffuse[0]) + ", " + std::to_string(diffuse[1]) + ", " + std::to_string(diffuse[2]) + "\n").c_str());
	}

	// Extract ambient color
	if (phongMaterial)
	{
		FbxDouble3 ambient = phongMaterial->Ambient.Get();
		m_materialInfo.ambientColor = XMFLOAT4((float)ambient[0], (float)ambient[1], (float)ambient[2], 1.0f);
		OutputDebugStringA(("Ambient color: " + std::to_string(ambient[0]) + ", " + std::to_string(ambient[1]) + ", " + std::to_string(ambient[2]) + "\n").c_str());
	}
	else if (lambertMaterial)
	{
		FbxDouble3 ambient = lambertMaterial->Ambient.Get();
		m_materialInfo.ambientColor = XMFLOAT4((float)ambient[0], (float)ambient[1], (float)ambient[2], 1.0f);
		OutputDebugStringA(("Ambient color: " + std::to_string(ambient[0]) + ", " + std::to_string(ambient[1]) + ", " + std::to_string(ambient[2]) + "\n").c_str());
	}

	// Extract specular color and shininess (Phong only)
	if (phongMaterial)
	{
		FbxDouble3 specular = phongMaterial->Specular.Get();
		m_materialInfo.specularColor = XMFLOAT4((float)specular[0], (float)specular[1], (float)specular[2], 1.0f);
		m_materialInfo.shininess = (float)phongMaterial->Shininess.Get();
		OutputDebugStringA(("Specular color: " + std::to_string(specular[0]) + ", " + std::to_string(specular[1]) + ", " + std::to_string(specular[2]) + "\n").c_str());
		OutputDebugStringA(("Shininess: " + std::to_string(m_materialInfo.shininess) + "\n").c_str());
	}

	// Extract textures
	FbxProperty diffuseProperty = material->FindProperty("DiffuseColor");
	m_materialInfo.diffuseTexturePath = GetTexturePath(diffuseProperty);
	OutputDebugStringA(("Diffuse texture path: " + m_materialInfo.diffuseTexturePath + "\n").c_str());

	FbxProperty normalProperty = material->FindProperty("NormalMap");
	m_materialInfo.normalTexturePath = GetTexturePath(normalProperty);
	OutputDebugStringA(("Normal texture path: " + m_materialInfo.normalTexturePath + "\n").c_str());
}


string Model::GetTexturePath(FbxProperty& property)
{
	if (property.IsValid())
	{
		int textureCount = property.GetSrcObjectCount<FbxTexture>();
		OutputDebugStringA(("Number of textures found for property: " + std::to_string(textureCount) + "\n").c_str());
		
		if (textureCount > 0)
		{
			FbxTexture* texture = property.GetSrcObject<FbxTexture>(0);
			if (texture)
			{
				FbxFileTexture* fileTexture = FbxCast<FbxFileTexture>(texture);
				if (fileTexture)
				{
					string texturePath = fileTexture->GetFileName();
					OutputDebugStringA(("Found texture: " + texturePath + "\n").c_str());
					return texturePath;
				}
			}
		}
	}
	return "";
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