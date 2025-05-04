#include "modelclass.h"
#include <vector>

ModelClass::ModelClass()
{
	m_vertexBuffer = 0;
	m_indexBuffer = 0;
	m_Texture = 0;
	m_model = 0;
}


ModelClass::ModelClass(const ModelClass& other)
{
}


ModelClass::~ModelClass()
{
}

bool ModelClass::Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, char* modelFilename, char* textureFilename)
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
	result = LoadTexture(device, deviceContext, textureFilename);
	if (!result)
	{
		return false;
	}

	return true;
}

void ModelClass::Shutdown()
{
	// Release the model texture.
	ReleaseTexture();

	// Shutdown the vertex and index buffers.
	ShutdownBuffers();

	// Release the model data.
	ReleaseModel();

	return;
}

void ModelClass::Render(ID3D11DeviceContext* deviceContext)
{
	// Put the vertex and index buffers on the graphics pipeline to prepare them for drawing.
	RenderBuffers(deviceContext);

	return;
}

int ModelClass::GetIndexCount()
{
	return m_indexCount;
}

ID3D11ShaderResourceView* ModelClass::GetTexture()
{
	return m_Texture->GetTexture();
}


bool ModelClass::InitializeBuffers(ID3D11Device* device)
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

void ModelClass::ShutdownBuffers()
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

void ModelClass::RenderBuffers(ID3D11DeviceContext* deviceContext)
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

bool ModelClass::LoadTexture(ID3D11Device* device, ID3D11DeviceContext* deviceContext, char* filename)
{
	bool result;


	// Create and initialize the texture object.
	m_Texture = new TextureClass;

	result = m_Texture->Initialize(device, deviceContext, filename);
	if (!result)
	{
		return false;
	}

	return true;
}

void ModelClass::ReleaseTexture()
{
	// Release the texture object.
	if (m_Texture)
	{
		m_Texture->Shutdown();
		delete m_Texture;
		m_Texture = 0;
	}

	return;
}

bool ModelClass::LoadModel(char* filename)
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

bool ModelClass::LoadTextModel(char* filename)
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

bool ModelClass::LoadFBXModel(char* filename)
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

void ModelClass::ProcessNode(FbxNode* pNode)
{
	if (!pNode)
	{
		return;
	}

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

void ModelClass::ProcessMesh(FbxNode* pNode)
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

void ModelClass::ReleaseModel()
{
	if (m_model)
	{
		delete[] m_model;
		m_model = 0;
	}

	return;
}