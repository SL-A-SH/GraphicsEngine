#ifndef _SHADERMANAGER_H_
#define _SHADERMANAGER_H_

#include "textureshader.h"
#include "lightshader.h"
#include "normalmapshader.h"
#include "specularmapshader.h"
#include "fontshader.h"

class ShaderManager
{
public:
    ShaderManager();
    ShaderManager(const ShaderManager&);
    ~ShaderManager();

    bool Initialize(ID3D11Device*, HWND);
    void Shutdown();

    bool RenderTextureShader(ID3D11DeviceContext*, int, XMMATRIX, XMMATRIX, XMMATRIX, ID3D11ShaderResourceView*);
    bool RenderLightShader(ID3D11DeviceContext*, int, XMMATRIX, XMMATRIX, XMMATRIX, ID3D11ShaderResourceView*, XMFLOAT3, XMFLOAT4, XMFLOAT4, XMFLOAT3, XMFLOAT4, float);
    bool RenderNormalMapShader(ID3D11DeviceContext*, int, XMMATRIX, XMMATRIX, XMMATRIX, ID3D11ShaderResourceView*, ID3D11ShaderResourceView*, XMFLOAT3, XMFLOAT4);
    bool RenderSpecularMapShader(ID3D11DeviceContext*, int, XMMATRIX, XMMATRIX, XMMATRIX, ID3D11ShaderResourceView*, ID3D11ShaderResourceView*, ID3D11ShaderResourceView*, XMFLOAT3, XMFLOAT4, XMFLOAT3, XMFLOAT4, float);
    bool RenderFontShader(ID3D11DeviceContext*, int, XMMATRIX, XMMATRIX, XMMATRIX, ID3D11ShaderResourceView*, XMFLOAT4);

private:
    TextureShader* m_TextureShader;
    LightShader* m_LightShader;
    NormalMapShader* m_NormalMapShader;
    SpecMapShader* m_SpecMapShader;
    FontShader* m_FontShader;
};

#endif
