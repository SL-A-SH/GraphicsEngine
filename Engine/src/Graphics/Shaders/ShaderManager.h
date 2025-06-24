#ifndef _SHADERMANAGER_H_
#define _SHADERMANAGER_H_

#include "TextureShader.h"
#include "LightShader.h"
#include "NormalMapShader.h"
#include "SpecularMapShader.h"
#include "FontShader.h"
#include "./Environment/SkyboxShader.h"
#include "ColorShader.h"
#include "PBRShader.h"

class ShaderManager
{
public:
    ShaderManager();
    ShaderManager(const ShaderManager&);
    ~ShaderManager();

    bool Initialize(ID3D11Device*, HWND);
    void Shutdown();

    bool RenderTextureShader(ID3D11DeviceContext*, int, XMMATRIX, XMMATRIX, XMMATRIX, ID3D11ShaderResourceView*);
    bool RenderLightShader(ID3D11DeviceContext*, int, const XMMATRIX&, const XMMATRIX&, const XMMATRIX&, ID3D11ShaderResourceView*,
						   const XMFLOAT3&, const XMFLOAT4&, const XMFLOAT4&, const XMFLOAT3&, const XMFLOAT4&, float);
    bool RenderNormalMapShader(ID3D11DeviceContext*, int, XMMATRIX, XMMATRIX, XMMATRIX, ID3D11ShaderResourceView*, ID3D11ShaderResourceView*, XMFLOAT3, XMFLOAT4);
    bool RenderSpecularMapShader(ID3D11DeviceContext*, int, XMMATRIX, XMMATRIX, XMMATRIX, ID3D11ShaderResourceView*, ID3D11ShaderResourceView*, ID3D11ShaderResourceView*, XMFLOAT3, XMFLOAT4, XMFLOAT3, XMFLOAT4, float);
    bool RenderFontShader(ID3D11DeviceContext*, int, XMMATRIX, XMMATRIX, XMMATRIX, ID3D11ShaderResourceView*, XMFLOAT4);
    bool RenderSkyboxShader(ID3D11DeviceContext*, int, XMMATRIX, XMMATRIX, XMMATRIX, ID3D11ShaderResourceView*[6]);
    bool RenderColorShader(ID3D11DeviceContext*, int, const XMMATRIX&, const XMMATRIX&, const XMMATRIX&, const XMFLOAT4&);
    bool RenderPBRShader(ID3D11DeviceContext*, int, XMMATRIX, XMMATRIX, XMMATRIX, 
                        ID3D11ShaderResourceView*, ID3D11ShaderResourceView*, ID3D11ShaderResourceView*, 
                        ID3D11ShaderResourceView*, ID3D11ShaderResourceView*, ID3D11ShaderResourceView*,
                        XMFLOAT3, XMFLOAT4, XMFLOAT4, XMFLOAT4, float, float, float, float, XMFLOAT3);

private:
    TextureShader* m_TextureShader;
    LightShader* m_LightShader;
    NormalMapShader* m_NormalMapShader;
    SpecMapShader* m_SpecMapShader;
    FontShader* m_FontShader;
    SkyboxShader* m_SkyboxShader;
    ColorShader* m_ColorShader;
    PBRShader* m_PBRShader;
};

#endif
