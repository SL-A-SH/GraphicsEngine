#ifndef _MODEL_H_
#define _MODEL_H_

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

class Model
{
public:
	struct AABB
	{
		XMFLOAT3 min;
		XMFLOAT3 max;
		float radius;  // For backward compatibility with sphere culling
	};

private:
	struct VertexType
	{
		XMFLOAT3 position;
		XMFLOAT2 texture;
		XMFLOAT3 normal;
		XMFLOAT3 tangent;
		XMFLOAT3 binormal;
	};

	struct ModelType
	{
		float x, y, z;
		float tu, tv;
		float nx, ny, nz;
		float tx, ty, tz;
		float bx, by, bz;
	};

	struct TempVertexType
	{
		float x, y, z;
		float tu, tv;
		float nx, ny, nz;
	};

	struct VectorType
	{
		float x, y, z;
	};

	struct MaterialInfo
	{
		string diffuseTexturePath;
		string normalTexturePath;
		string specularTexturePath;
		string roughnessTexturePath;
		string metallicTexturePath;
		string emissionTexturePath;
		string aoTexturePath;
		XMFLOAT4 diffuseColor;
		XMFLOAT4 ambientColor;
		XMFLOAT4 specularColor;
		float shininess;
		float metallic;
		float roughness;
		float ao;
		float emissionStrength;
	};

public:
	Model();
	Model(const Model&);
	~Model();

	bool Initialize(ID3D11Device*, ID3D11DeviceContext*, char*, char*);
	bool Initialize(ID3D11Device*, ID3D11DeviceContext*, char*, char*, char*);
	bool Initialize(ID3D11Device*, ID3D11DeviceContext*, char*, char*, char*, char*);
	bool InitializeFBX(ID3D11Device*, ID3D11DeviceContext*, char*);
	void Shutdown();
	void Render(ID3D11DeviceContext*);

	int GetIndexCount();
	ID3D11ShaderResourceView* GetTexture();
	ID3D11ShaderResourceView* GetTexture(int index);
	bool HasFBXMaterial() const { return m_hasFBXMaterial; }
	const AABB& GetBoundingBox() const { return m_boundingBox; }
	const MaterialInfo& GetMaterialInfo() const { return m_materialInfo; }

	// PBR texture getters
	ID3D11ShaderResourceView* GetDiffuseTexture();
	ID3D11ShaderResourceView* GetNormalTexture();
	ID3D11ShaderResourceView* GetMetallicTexture();
	ID3D11ShaderResourceView* GetRoughnessTexture();
	ID3D11ShaderResourceView* GetEmissionTexture();
	ID3D11ShaderResourceView* GetAOTexture();
	
	// PBR material properties
	XMFLOAT4 GetBaseColor() const { return m_materialInfo.diffuseColor; }
	float GetMetallic() const { return m_materialInfo.metallic; }
	float GetRoughness() const { return m_materialInfo.roughness; }
	float GetAO() const { return m_materialInfo.ao; }
	float GetEmissionStrength() const { return m_materialInfo.emissionStrength; }

private:
	bool InitializeBuffers(ID3D11Device*);
	void ShutdownBuffers();
	void RenderBuffers(ID3D11DeviceContext*);

	bool LoadTexture(ID3D11Device*, ID3D11DeviceContext*, char*);
	bool LoadTextures(ID3D11Device*, ID3D11DeviceContext*, char*, char*);
	bool LoadTextures(ID3D11Device*, ID3D11DeviceContext*, char*, char*, char*);
	bool LoadFBXTextures(ID3D11Device*, ID3D11DeviceContext*);
	void ReleaseTextures();

	bool LoadModel(char*);
	bool LoadTextModel(char*);
	bool LoadFBXModel(char*);
	void ProcessNode(FbxNode* pNode);
	void ProcessMesh(FbxNode* pNode);
	void ProcessMaterials(FbxNode* pNode);
	void ExtractMaterialInfo(FbxSurfaceMaterial* material);
	const FbxFileTexture* FindConnectedFileTexture(const FbxProperty& property);
	void SearchSceneForTextures(FbxScene* scene);
	void SearchDirectoryForTextures();
	void ListAllMaterialProperties(FbxSurfaceMaterial* material);
	string ConvertTexturePath(const string& originalPath);
	void ReleaseModel();
	void CalculateBoundingBox();

	void CalculateModelVectors();
	void CalculateTangentBinormal(TempVertexType, TempVertexType, TempVertexType, VectorType&, VectorType&);

private:
	ID3D11Buffer* m_vertexBuffer, * m_indexBuffer;
	int m_vertexCount, m_indexCount;
	Texture* m_Texture;
	vector<Texture> m_Textures;
	
	// PBR textures
	Texture* m_diffuseTexture;
	Texture* m_normalTexture;
	Texture* m_metallicTexture;
	Texture* m_roughnessTexture;
	Texture* m_emissionTexture;
	Texture* m_aoTexture;
	
	ModelType* m_model;
	MaterialInfo m_materialInfo;
	bool m_hasFBXMaterial;
	AABB m_boundingBox;
	string m_currentFBXPath;
};

#endif