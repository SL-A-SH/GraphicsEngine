#ifndef _SPACESKYBOXSHADER_H_
#define _SPACESKYBOXSHADER_H_

#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <fstream>

using namespace DirectX;
using namespace std;

class SpaceSkyboxShader
{
private:
    struct MatrixBufferType
    {
        XMMATRIX world;
        XMMATRIX view;
        XMMATRIX projection;
    };

    struct TimeBufferType
    {
        float time;
        float mainStarSize;
        XMFLOAT2 padding0; // pad to 16 bytes
        XMFLOAT4 mainStarDir;
        XMFLOAT4 mainStarColor;
        float mainStarIntensity;
        XMFLOAT3 padding1; // pad to 16 bytes
    };

public:
    SpaceSkyboxShader();
    SpaceSkyboxShader(const SpaceSkyboxShader&);
    ~SpaceSkyboxShader();

    bool Initialize(ID3D11Device*, HWND);
    void Shutdown();
    bool Render(ID3D11DeviceContext*, int, XMMATRIX, XMMATRIX, XMMATRIX, float, float, XMFLOAT3, XMFLOAT3, float);

private:
    bool InitializeShader(ID3D11Device*, HWND, WCHAR*, WCHAR*);
    void ShutdownShader();
    void OutputShaderErrorMessage(ID3D10Blob*, HWND, WCHAR*);

    bool SetShaderParameters(ID3D11DeviceContext*, XMMATRIX, XMMATRIX, XMMATRIX, float, float, XMFLOAT3, XMFLOAT3, float);
    void RenderShader(ID3D11DeviceContext*, int);

private:
    ID3D11VertexShader* m_vertexShader;
    ID3D11PixelShader* m_pixelShader;
    ID3D11InputLayout* m_layout;
    ID3D11Buffer* m_matrixBuffer;
    ID3D11Buffer* m_timeBuffer;
};

#endif 