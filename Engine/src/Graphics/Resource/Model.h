#ifndef MODEL_H
#define MODEL_H

#include <d3d11.h>
#include <directxmath.h>
#include <fstream>
#include <string>
#include <vector>
#include <fbxsdk.h>

#include "../../Core/Common/EngineTypes.h"
#include "./Texture.h"

using namespace DirectX;

// FBX property names
#define FBXSDK_CURRENTNAMESPACE fbxsdk
#define FBXSDK_NAMESPACE_USE fbxsdk

class Model
{
public:
	using AABB = EngineTypes::BoundingBox;
	using VertexType = EngineTypes::VertexType;
	using MaterialInfo = EngineTypes::MaterialInfo;

private:
	struct ModelType
	{
		float x, y, z;
		float tu, tv;
		float nx, ny, nz;
		float tx, ty, tz;
		float bx, by, bz;
	};

	using TempVertexType = EngineTypes::TempVertexType;
	using VectorType = EngineTypes::VectorType;

public:
	Model();
	~Model();

	// Initialization methods
	bool Initialize(ID3D11Device* device, ID3D11DeviceContext* context, char* modelFilename, char* textureFilename);
	bool Initialize(ID3D11Device* device, ID3D11DeviceContext* context, char* modelFilename, char* textureFilename1, char* textureFilename2);
	bool Initialize(ID3D11Device* device, ID3D11DeviceContext* context, char* modelFilename, char* textureFilename1, char* textureFilename2, char* textureFilename3);
	bool InitializeFBX(ID3D11Device* device, ID3D11DeviceContext* context, char* modelFilename);
	
	void Shutdown();
	void Render(ID3D11DeviceContext* context);

	// Getters
	int GetIndexCount() const { return m_indexCount; }
	ID3D11ShaderResourceView* GetTexture() const;
	ID3D11ShaderResourceView* GetTexture(int index) const;
	bool HasFBXMaterial() const { return m_hasFBXMaterial; }
	const AABB& GetBoundingBox() const { return m_boundingBox; }
	const MaterialInfo& GetMaterialInfo() const { return m_materialInfo; }

	// PBR texture getters
	ID3D11ShaderResourceView* GetDiffuseTexture() const;
	ID3D11ShaderResourceView* GetNormalTexture() const;
	ID3D11ShaderResourceView* GetMetallicTexture() const;
	ID3D11ShaderResourceView* GetRoughnessTexture() const;
	ID3D11ShaderResourceView* GetEmissionTexture() const;
	ID3D11ShaderResourceView* GetAOTexture() const;
	
	// PBR material properties
	XMFLOAT4 GetBaseColor() const { return m_materialInfo.diffuseColor; }
	float GetMetallic() const { return m_materialInfo.metallic; }
	float GetRoughness() const { return m_materialInfo.roughness; }
	float GetAO() const { return m_materialInfo.ao; }
	float GetEmissionStrength() const { return m_materialInfo.emissionStrength; }

private:
	// Buffer management
	bool InitializeBuffers(ID3D11Device* device);
	void ShutdownBuffers();
	void RenderBuffers(ID3D11DeviceContext* context);

	// Texture loading
	bool LoadTexture(ID3D11Device* device, ID3D11DeviceContext* context, char* filename);
	bool LoadTextures(ID3D11Device* device, ID3D11DeviceContext* context, char* filename1, char* filename2);
	bool LoadTextures(ID3D11Device* device, ID3D11DeviceContext* context, char* filename1, char* filename2, char* filename3);
	bool LoadFBXTextures(ID3D11Device* device, ID3D11DeviceContext* context);
	void ReleaseTextures();

	// Model loading
	bool LoadModel(char* filename);
	bool LoadTextModel(char* filename);
	bool LoadFBXModel(char* filename);
	
	// FBX processing
	void ProcessNode(FbxNode* pNode);
	void ProcessMesh(FbxNode* pNode);
	void ProcessMaterials(FbxNode* pNode);
	void ExtractMaterialInfo(FbxSurfaceMaterial* material);
	const FbxFileTexture* FindConnectedFileTexture(const FbxProperty& property);
	void SearchSceneForTextures(FbxScene* scene);
	void SearchDirectoryForTextures();
	void ListAllMaterialProperties(FbxSurfaceMaterial* material);
	std::string ConvertTexturePath(const std::string& originalPath);
	
	// Model processing
	void ReleaseModel();
	void CalculateBoundingBox();
	void CalculateModelVectors();
	void CalculateTangentBinormal(TempVertexType vertex1, TempVertexType vertex2, TempVertexType vertex3, VectorType& tangent, VectorType& binormal);

private:
	// DirectX resources
	ID3D11Buffer* m_vertexBuffer;
	ID3D11Buffer* m_indexBuffer;
	int m_vertexCount;
	int m_indexCount;

	// Textures
	Texture* m_Texture;
	std::vector<Texture> m_Textures;
	
	// PBR textures
	Texture* m_diffuseTexture;
	Texture* m_normalTexture;
	Texture* m_metallicTexture;
	Texture* m_roughnessTexture;
	Texture* m_emissionTexture;
	Texture* m_aoTexture;
	
	// Model data
	ModelType* m_model;
	MaterialInfo m_materialInfo;
	bool m_hasFBXMaterial;
	AABB m_boundingBox;
	std::string m_currentFBXPath;
};

#endif // MODEL_H