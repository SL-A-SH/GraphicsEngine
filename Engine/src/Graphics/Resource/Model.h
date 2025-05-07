#ifndef _MODELCLASS_H_
#define _MODELCLASS_H_

#include <d3d11.h>
#include <directxmath.h>
#include <fstream>
#include <string>
#include <vector>
#include "./Texture.h"
#include <fbxsdk.h>

using namespace DirectX;
using namespace std;

// FBX property names
#define FBXSDK_CURRENTNAMESPACE fbxsdk
#define FBXSDK_NAMESPACE_USE fbxsdk

class ModelClass
{
private:
	struct VertexType
	{
		XMFLOAT3 position;
		XMFLOAT2 texture;
		XMFLOAT3 normal;
	};

	struct ModelType
	{
		float x, y, z;
		float tu, tv;
		float nx, ny, nz;
	};

	struct MaterialInfo
	{
		string diffuseTexturePath;
		string normalTexturePath;
		XMFLOAT4 diffuseColor;
		XMFLOAT4 ambientColor;
		XMFLOAT4 specularColor;
		float shininess;
	};

public:
	ModelClass();
	ModelClass(const ModelClass&);
	~ModelClass();

	bool Initialize(ID3D11Device*, ID3D11DeviceContext*, char*, char*);
	void Shutdown();
	void Render(ID3D11DeviceContext*);

	int GetIndexCount();
	ID3D11ShaderResourceView* GetTexture();
	bool HasFBXMaterial() const { return m_hasFBXMaterial; }

private:
	bool InitializeBuffers(ID3D11Device*);
	void ShutdownBuffers();
	void RenderBuffers(ID3D11DeviceContext*);

	bool LoadTexture(ID3D11Device*, ID3D11DeviceContext*, char*);
	void ReleaseTexture();

	bool LoadModel(char*);
	bool LoadTextModel(char*);
	bool LoadFBXModel(char*);
	void ProcessNode(FbxNode* pNode);
	void ProcessMesh(FbxNode* pNode);
	void ProcessMaterials(FbxNode* pNode);
	void ExtractMaterialInfo(FbxSurfaceMaterial* material);
	string GetTexturePath(FbxProperty& property);
	void ReleaseModel();

private:
	ID3D11Buffer* m_vertexBuffer, * m_indexBuffer;
	int m_vertexCount, m_indexCount;
	TextureClass* m_Texture;
	ModelType* m_model;
	MaterialInfo m_materialInfo;
	bool m_hasFBXMaterial;
};

#endif
