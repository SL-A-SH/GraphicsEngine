#ifndef COMPUTE_SHADER_H
#define COMPUTE_SHADER_H

#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <string>

using namespace DirectX;

class ComputeShader
{
public:
    ComputeShader();
    virtual ~ComputeShader();

    bool Initialize(ID3D11Device* device, HWND hwnd, const WCHAR* filename, const char* entryPoint);
    void Shutdown();
    
    // Dispatch the compute shader
    void Dispatch(ID3D11DeviceContext* context, UINT threadGroupCountX, UINT threadGroupCountY = 1, UINT threadGroupCountZ = 1);
    
    // Set compute shader resources
    void SetShaderResourceView(ID3D11DeviceContext* context, UINT slot, ID3D11ShaderResourceView* srv);
    void SetUnorderedAccessView(ID3D11DeviceContext* context, UINT slot, ID3D11UnorderedAccessView* uav);
    void SetConstantBuffer(ID3D11DeviceContext* context, UINT slot, ID3D11Buffer* buffer);
    
    // Get the compute shader
    ID3D11ComputeShader* GetComputeShader() const { return m_computeShader; }

protected:
    bool CompileComputeShader(ID3D11Device* device, HWND hwnd, const WCHAR* filename, const char* entryPoint);
    void OutputShaderErrorMessage(ID3D10Blob* errorMessage, HWND hwnd, const WCHAR* shaderFilename);

private:
    ID3D11ComputeShader* m_computeShader;
    ID3D10Blob* m_computeShaderBuffer;
};

#endif // COMPUTE_SHADER_H 