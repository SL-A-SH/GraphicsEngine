#ifndef GPU_DRIVEN_RENDERER_H
#define GPU_DRIVEN_RENDERER_H

#include <d3d11.h>
#include <directxmath.h>
#include <vector>
#include "IndirectDrawBuffer.h"
#include "../Shaders/ComputeShader.h"
#include "../../Core/System/Logger.h"

using namespace DirectX;

class GPUDrivenRenderer
{
public:
    GPUDrivenRenderer();
    ~GPUDrivenRenderer();

    bool Initialize(ID3D11Device* device, HWND hwnd, UINT maxObjects);
    void Shutdown();

    // Update object data for rendering
    void UpdateObjects(ID3D11DeviceContext* context, const std::vector<ObjectData>& objects);
    
    // Update camera data
    void UpdateCamera(ID3D11DeviceContext* context, const XMFLOAT3& cameraPos, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix);
    
    // Perform GPU-driven rendering (SIMPLIFIED VERSION)
    void Render(ID3D11DeviceContext* context, ID3D11Buffer* vertexBuffer, ID3D11Buffer* indexBuffer,
                class Model* model, class PBRShader* pbrShader, class Light* light, class Camera* camera, class D3D11Device* direct3D);
    
    // Get rendering statistics
    int GetRenderCount() const { return m_renderCount; }
    
    // Set rendering mode
    void SetRenderingMode(bool enableGPUDriven) { m_enableGPUDriven = enableGPUDriven; }
    bool IsGPUDrivenEnabled() const { return m_enableGPUDriven; }
    
    // Check if compute shaders are properly initialized
    bool AreComputeShadersInitialized() const 
    { 
        bool worldMatrixValid = m_worldMatrixGenerationCS && m_worldMatrixGenerationCS->GetComputeShader();
        
        if (!worldMatrixValid)
            LOG_ERROR("GPUDrivenRenderer: World matrix generation compute shader is not valid");
        
        return worldMatrixValid;
    }
    
    // Check if indirect buffer is properly initialized
    bool IsIndirectBufferInitialized() const
    {
        bool objectDataValid = m_indirectBuffer.GetObjectDataSRV() != nullptr;
        bool worldMatrixValid = m_indirectBuffer.GetWorldMatrixUAV() != nullptr;
        
        if (!objectDataValid)
            LOG_ERROR("GPUDrivenRenderer: Object data SRV is not valid");
        if (!worldMatrixValid)
            LOG_ERROR("GPUDrivenRenderer: World matrix UAV is not valid");
        
        return objectDataValid && worldMatrixValid;
    }

private:
    bool InitializeComputeShaders(ID3D11Device* device, HWND hwnd);
    void ReleaseComputeShaders();
    
    // Initialize GPU-driven rendering shaders
    bool InitializeGPUDrivenShaders(ID3D11Device* device, HWND hwnd);
    void ReleaseGPUDrivenShaders();

private:
    // SIMPLIFIED: Only world matrix generation compute shader
    ComputeShader* m_worldMatrixGenerationCS;
    
    // GPU-driven rendering shaders
    ID3D11VertexShader* m_gpuDrivenVertexShader;
    ID3D11PixelShader* m_gpuDrivenPixelShader;
    ID3D11InputLayout* m_gpuDrivenInputLayout;
    
    // Indirect rendering buffers
    IndirectDrawBuffer m_indirectBuffer;
    
    // Rendering state
    bool m_enableGPUDriven;
    UINT m_maxObjects;
    int m_renderCount;
    
    // Camera data
    XMFLOAT3 m_cameraPosition;
    XMMATRIX m_viewMatrix;
    XMMATRIX m_projectionMatrix;
};

#endif // GPU_DRIVEN_RENDERER_H 