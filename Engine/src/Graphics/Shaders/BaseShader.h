#ifndef BASE_SHADER_H
#define BASE_SHADER_H

#include <d3d11.h>
#include <directxmath.h>
#include <string>

using namespace DirectX;

// Matrix buffer structure for shaders
struct MatrixBufferType
{
    XMMATRIX world;
    XMMATRIX view;
    XMMATRIX projection;
};

class BaseShader
{
public:
    BaseShader();
    virtual ~BaseShader();

    // Common initialization and shutdown
    virtual bool Initialize(ID3D11Device* device, HWND hwnd);
    virtual void Shutdown();
    
    // Common rendering methods
    virtual bool Render(ID3D11DeviceContext* context, int indexCount, XMMATRIX worldMatrix, XMMATRIX viewMatrix, XMMATRIX projectionMatrix);
    virtual bool SetShaderParameters(ID3D11DeviceContext* context, XMMATRIX worldMatrix, XMMATRIX viewMatrix, XMMATRIX projectionMatrix) = 0;
    virtual void RenderShader(ID3D11DeviceContext* context, int indexCount) = 0;

protected:
    // Common shader compilation and creation
    bool InitializeShader(ID3D11Device* device, HWND hwnd, const WCHAR* vsFilename, const WCHAR* psFilename);
    void ShutdownShader();
    
    // Error handling
    void OutputShaderErrorMessage(ID3D10Blob* errorMessage, HWND hwnd, const WCHAR* shaderFilename);
    
    // Common shader resources
    ID3D11VertexShader* m_vertexShader;
    ID3D11PixelShader* m_pixelShader;
    ID3D11InputLayout* m_layout;
    ID3D11Buffer* m_matrixBuffer;
    ID3D11SamplerState* m_sampleState;

private:
    // Shader compilation helpers
    bool CompileVertexShader(ID3D11Device* device, HWND hwnd, const WCHAR* filename, const char* entryPoint, ID3D10Blob** shaderBuffer);
    bool CompilePixelShader(ID3D11Device* device, HWND hwnd, const WCHAR* filename, const char* entryPoint, ID3D10Blob** shaderBuffer);
    bool CreateMatrixBuffer(ID3D11Device* device);
    bool CreateSamplerState(ID3D11Device* device);
};

#endif // BASE_SHADER_H 