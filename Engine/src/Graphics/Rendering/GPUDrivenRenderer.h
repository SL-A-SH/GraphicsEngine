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
    long long GetLastFrustumCullingTimeMicroseconds() const { return m_lastFrustumCullingTime; }
    
    // Set rendering mode
    void SetRenderingMode(bool enableGPUDriven) { m_enableGPUDriven = enableGPUDriven; }
    bool IsGPUDrivenEnabled() const { return m_enableGPUDriven; }
    
    // Check if compute shaders are properly initialized
    bool AreComputeShadersInitialized() const 
    { 
        bool worldMatrixValid = m_worldMatrixGenerationCS && m_worldMatrixGenerationCS->GetComputeShader();
        bool frustumCullingValid = m_frustumCullingCS && m_frustumCullingCS->GetComputeShader();
        
        if (!worldMatrixValid)
            LOG_ERROR("GPUDrivenRenderer: World matrix generation compute shader is not valid");
        if (!frustumCullingValid)
            LOG_ERROR("GPUDrivenRenderer: Frustum culling compute shader is not valid");
        
        return worldMatrixValid && frustumCullingValid;
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
    
    // Initialize frustum culling buffers
    bool InitializeVisibilityBuffer(ID3D11Device* device, UINT maxObjects);
    void ReleaseVisibilityBuffer();
    
    // TRUE GPU-DRIVEN: Initialize indirect draw and stream compaction buffers
    bool InitializeIndirectDrawBuffers(ID3D11Device* device, UINT maxObjects);
    void ReleaseIndirectDrawBuffers();
    
    // Initialize reusable constant buffers for performance
    bool InitializeConstantBuffers(ID3D11Device* device);
    void ReleaseConstantBuffers();
    
    // Extract frustum planes from view-projection matrix
    void ExtractFrustumPlanes(const XMMATRIX& viewProjectionMatrix);

private:
    // Compute shaders for GPU-driven rendering
    ComputeShader* m_worldMatrixGenerationCS;
    ComputeShader* m_frustumCullingCS;
    ComputeShader* m_streamCompactionCS;           // TRUE GPU-DRIVEN: Compact visible objects
    ComputeShader* m_updateDrawArgsCS;             // TRUE GPU-DRIVEN: Update draw arguments
    
    // GPU-driven rendering shaders
    ID3D11VertexShader* m_gpuDrivenVertexShader;
    ID3D11PixelShader* m_gpuDrivenPixelShader;
    ID3D11InputLayout* m_gpuDrivenInputLayout;
    
    // Indirect rendering buffers
    IndirectDrawBuffer m_indirectBuffer;
    
    // Frustum culling buffers
    ID3D11Buffer* m_visibilityBuffer;
    ID3D11ShaderResourceView* m_visibilitySRV;
    ID3D11UnorderedAccessView* m_visibilityUAV;
    ID3D11Buffer* m_visibilityReadbackBuffer; // CPU-readable copy for conditional rendering
    
    // TRUE GPU-DRIVEN RENDERING: Indirect draw buffers
    ID3D11Buffer* m_drawArgumentsBuffer;           // DrawIndexedInstancedIndirect arguments
    ID3D11UnorderedAccessView* m_drawArgumentsUAV; // For compute shader to update
    
    // Stream compaction buffers for visible objects
    ID3D11Buffer* m_visibleObjectsBuffer;          // Compacted array of visible object indices
    ID3D11ShaderResourceView* m_visibleObjectsSRV;
    ID3D11UnorderedAccessView* m_visibleObjectsUAV;
    
    // Counter buffer for visible object count
    ID3D11Buffer* m_visibleCountBuffer;
    ID3D11UnorderedAccessView* m_visibleCountUAV;
    ID3D11Buffer* m_visibleCountStagingBuffer;     // For reading back count if needed
    
    // PERFORMANCE: Reusable constant buffers (created once, reused every frame)
    ID3D11Buffer* m_frustumConstantBuffer;
    ID3D11Buffer* m_objectCountBuffer;
    ID3D11Buffer* m_viewProjectionBuffer;
    ID3D11Buffer* m_lightBuffer;
    ID3D11Buffer* m_materialBuffer;
    
    // Rendering state
    bool m_enableGPUDriven;
    UINT m_maxObjects;
    int m_renderCount;
    long long m_lastFrustumCullingTime; // Last frustum culling time in microseconds
    
    // Camera data
    XMFLOAT3 m_cameraPosition;
    XMMATRIX m_viewMatrix;
    XMMATRIX m_projectionMatrix;
    
    // Frustum data
    XMFLOAT4 m_frustumPlanes[6]; // Left, Right, Top, Bottom, Near, Far
    
    // PERFORMANCE OPTIMIZATION: Cached previous frame data to avoid unnecessary updates
    XMMATRIX m_prevViewMatrix;
    XMMATRIX m_prevProjectionMatrix;
    XMFLOAT3 m_prevCameraPosition;
    XMFLOAT4 m_prevAmbientColor;
    XMFLOAT4 m_prevDiffuseColor;
    XMFLOAT3 m_prevLightDirection;
    XMFLOAT4 m_prevBaseColor;
    XMFLOAT4 m_prevMaterialProperties;
    bool m_constantBuffersInitialized;
};

#endif // GPU_DRIVEN_RENDERER_H 