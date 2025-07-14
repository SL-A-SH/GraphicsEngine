#ifndef _PBRSHADER_H_
#define _PBRSHADER_H_

#include <d3d11.h>
#include <directxmath.h>
#include <d3dcompiler.h>
#include <fstream>
#include <vector>
#include "../Resource/Texture.h"

using namespace DirectX;

class PBRShader
{
private:
	struct MatrixBufferType
	{
		XMMATRIX world;
		XMMATRIX view;
		XMMATRIX projection;
		UINT useGPUDrivenRendering;
		UINT padding[3];
	};

	struct LightBufferType
	{
		XMFLOAT4 ambientColor;
		XMFLOAT4 diffuseColor;
		XMFLOAT3 lightDirection;
		float padding;
		XMFLOAT3 cameraPosition;
		float padding2;
	};

	struct MaterialBufferType
	{
		XMFLOAT4 baseColor;
		XMFLOAT4 materialProperties; // metallic, roughness, ao, emissionStrength packed into XMFLOAT4
		XMFLOAT4 materialPadding; // Ensure 16-byte alignment
	};

public:
	PBRShader();
	PBRShader(const PBRShader&);
	~PBRShader();

	bool Initialize(ID3D11Device*, HWND);
	void Shutdown();
	bool Render(ID3D11DeviceContext*, int, XMMATRIX, XMMATRIX, XMMATRIX, 
				ID3D11ShaderResourceView*, ID3D11ShaderResourceView*, ID3D11ShaderResourceView*, 
				ID3D11ShaderResourceView*, ID3D11ShaderResourceView*, ID3D11ShaderResourceView*,
				XMFLOAT3, XMFLOAT4, XMFLOAT4, XMFLOAT4, float, float, float, float, XMFLOAT3, bool);
	
	// GPU-driven rendering support
	ID3D11VertexShader* GetVertexShader() const { return m_vertexShader; }
	ID3D11PixelShader* GetPixelShader() const { return m_pixelShader; }
	ID3D11InputLayout* GetInputLayout() const { return m_layout; }
	
	// GPU-driven rendering setup
	bool SetupShaderParameters(ID3D11DeviceContext*, XMMATRIX, XMMATRIX, XMMATRIX,
							 ID3D11ShaderResourceView*, ID3D11ShaderResourceView*, ID3D11ShaderResourceView*,
							 ID3D11ShaderResourceView*, ID3D11ShaderResourceView*, ID3D11ShaderResourceView*,
							 XMFLOAT3, XMFLOAT4, XMFLOAT4, XMFLOAT4, float, float, float, float, XMFLOAT3, bool);

private:
	bool InitializeShader(ID3D11Device*, HWND, WCHAR*, WCHAR*);
	void ShutdownShader();
	void OutputShaderErrorMessage(ID3D10Blob*, HWND, WCHAR*);

	bool SetShaderParameters(ID3D11DeviceContext*, XMMATRIX, XMMATRIX, XMMATRIX,
							ID3D11ShaderResourceView*, ID3D11ShaderResourceView*, ID3D11ShaderResourceView*,
							ID3D11ShaderResourceView*, ID3D11ShaderResourceView*, ID3D11ShaderResourceView*,
							XMFLOAT3, XMFLOAT4, XMFLOAT4, XMFLOAT4, float, float, float, float, XMFLOAT3, bool);
	void RenderShader(ID3D11DeviceContext*, int);

private:
	ID3D11VertexShader* m_vertexShader;
	ID3D11PixelShader* m_pixelShader;
	ID3D11InputLayout* m_layout;
	ID3D11Buffer* m_matrixBuffer;
	ID3D11Buffer* m_lightBuffer;
	ID3D11Buffer* m_materialBuffer;
	ID3D11SamplerState* m_sampleState;
};

#endif 