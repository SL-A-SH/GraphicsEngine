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
    
    // Update camera and frustum data
    void UpdateCamera(ID3D11DeviceContext* context, const XMFLOAT3& cameraPos, const XMFLOAT3& cameraForward, 
                     const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix);
    
    // Perform GPU-driven rendering
    void Render(ID3D11DeviceContext* context, ID3D11Buffer* vertexBuffer, ID3D11Buffer* indexBuffer,
                ID3D11VertexShader* vertexShader, ID3D11PixelShader* pixelShader, ID3D11InputLayout* inputLayout);
    
    // Get rendering statistics
    UINT GetVisibleObjectCount() const { return m_indirectBuffer.GetVisibleObjectCount(); }
    UINT GetDrawCallCount() const { return m_drawCallCount; }
    
    // Set rendering mode
    void SetRenderingMode(bool enableGPUDriven) { m_enableGPUDriven = enableGPUDriven; }
    bool IsGPUDrivenEnabled() const { return m_enableGPUDriven; }
    
    // Check if compute shaders are properly initialized
    bool AreComputeShadersInitialized() const 
    { 
        bool frustumValid = m_frustumCullingCS && m_frustumCullingCS->GetComputeShader();
        bool lodValid = m_lodSelectionCS && m_lodSelectionCS->GetComputeShader();
        bool commandValid = m_commandGenerationCS && m_commandGenerationCS->GetComputeShader();
        
        if (!frustumValid)
            LOG_ERROR("GPUDrivenRenderer: Frustum culling compute shader is not valid");
        if (!lodValid)
            LOG_ERROR("GPUDrivenRenderer: LOD selection compute shader is not valid");
        if (!commandValid)
            LOG_ERROR("GPUDrivenRenderer: Command generation compute shader is not valid");
        
        return frustumValid && lodValid && commandValid;
    }
    
    // Check if indirect buffer is properly initialized
    bool IsIndirectBufferInitialized() const
    {
        bool objectDataValid = m_indirectBuffer.GetObjectDataSRV() != nullptr;
        bool lodLevelsValid = m_indirectBuffer.GetLODLevelsSRV() != nullptr;
        bool frustumValid = m_indirectBuffer.GetFrustumSRV() != nullptr;
        bool drawCommandValid = m_indirectBuffer.GetDrawCommandUAV() != nullptr;
        bool visibleCountValid = m_indirectBuffer.GetVisibleObjectCountUAV() != nullptr;
        bool frustumBufferValid = m_indirectBuffer.GetFrustumBuffer() != nullptr;
        bool drawCommandBufferValid = m_indirectBuffer.GetDrawCommandBuffer() != nullptr;
        
        if (!objectDataValid)
            LOG_ERROR("GPUDrivenRenderer: Object data SRV is not valid");
        if (!lodLevelsValid)
            LOG_ERROR("GPUDrivenRenderer: LOD levels SRV is not valid");
        if (!frustumValid)
            LOG_ERROR("GPUDrivenRenderer: Frustum SRV is not valid");
        if (!drawCommandValid)
            LOG_ERROR("GPUDrivenRenderer: Draw command UAV is not valid");
        if (!visibleCountValid)
            LOG_ERROR("GPUDrivenRenderer: Visible object count UAV is not valid");
        if (!frustumBufferValid)
            LOG_ERROR("GPUDrivenRenderer: Frustum buffer is not valid");
        if (!drawCommandBufferValid)
            LOG_ERROR("GPUDrivenRenderer: Draw command buffer is not valid");
        
        return objectDataValid && lodLevelsValid && frustumValid && 
               drawCommandValid && visibleCountValid && frustumBufferValid && drawCommandBufferValid;
    }

private:
    bool InitializeComputeShaders(ID3D11Device* device, HWND hwnd);
    void ReleaseComputeShaders();
    
    // Generate frustum data from view and projection matrices
    FrustumData GenerateFrustumData(const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix);

private:
    // Compute shaders
    ComputeShader* m_frustumCullingCS;
    ComputeShader* m_lodSelectionCS;
    ComputeShader* m_commandGenerationCS;
    
    // Indirect rendering buffers
    IndirectDrawBuffer m_indirectBuffer;
    
    // Rendering state
    bool m_enableGPUDriven;
    UINT m_drawCallCount;
    UINT m_maxObjects;
    
    // Camera data
    XMFLOAT3 m_cameraPosition;
    XMFLOAT3 m_cameraForward;
    XMMATRIX m_viewMatrix;
    XMMATRIX m_projectionMatrix;
};

#endif // GPU_DRIVEN_RENDERER_H 