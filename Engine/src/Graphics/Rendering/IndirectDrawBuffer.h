#ifndef INDIRECT_DRAW_BUFFER_H
#define INDIRECT_DRAW_BUFFER_H

#include <d3d11.h>
#include <directxmath.h>
#include <vector>

using namespace DirectX;

// Object data structure for GPU processing
struct ObjectData
{
    XMFLOAT3 position;
    XMFLOAT3 scale;
    XMFLOAT3 rotation;
    XMFLOAT3 boundingBoxMin;
    XMFLOAT3 boundingBoxMax;
    UINT objectIndex;
    UINT padding[2];
};

// LOD level structure
struct LODLevel
{
    float distanceThreshold;
    UINT indexCount;
    UINT vertexOffset;
    UINT indexOffset;
    UINT padding;
};

// Draw command structure for indirect rendering
struct DrawCommand
{
    UINT indexCountPerInstance;
    UINT instanceCount;
    UINT startIndexLocation;
    INT baseVertexLocation;
    UINT startInstanceLocation;
};

// Frustum and camera data for compute shaders
struct FrustumData
{
    XMFLOAT4 frustumPlanes[6];
    XMFLOAT4 cameraPosition;
    XMFLOAT4 cameraForward;
    XMFLOAT4 lodDistances[4];
    UINT maxLODLevels;
    UINT padding[3];
};

class IndirectDrawBuffer
{
public:
    IndirectDrawBuffer();
    ~IndirectDrawBuffer();

    bool Initialize(ID3D11Device* device, UINT maxObjects);
    void Shutdown();

    // Update object data
    void UpdateObjectData(ID3D11DeviceContext* context, const std::vector<ObjectData>& objects);
    
    // Update frustum data
    void UpdateFrustumData(ID3D11DeviceContext* context, const FrustumData& frustumData);
    
    // Get buffers for compute shaders
    ID3D11Buffer* GetObjectDataBuffer() const { return m_objectDataBuffer; }
    ID3D11Buffer* GetLODLevelsBuffer() const { return m_lodLevelsBuffer; }
    ID3D11Buffer* GetFrustumBuffer() const { return m_frustumBuffer; }
    ID3D11Buffer* GetDrawCommandBuffer() const { return m_drawCommandBuffer; }
    ID3D11Buffer* GetVisibleObjectCountBuffer() const { return m_visibleObjectCountBuffer; }
    
    // Get UAVs for compute shaders
    ID3D11UnorderedAccessView* GetDrawCommandUAV() const { return m_drawCommandUAV; }
    ID3D11UnorderedAccessView* GetVisibleObjectCountUAV() const { return m_visibleObjectCountUAV; }
    
    // Get SRVs for compute shaders
    ID3D11ShaderResourceView* GetObjectDataSRV() const { return m_objectDataSRV; }
    ID3D11ShaderResourceView* GetLODLevelsSRV() const { return m_lodLevelsSRV; }
    ID3D11ShaderResourceView* GetFrustumSRV() const { return m_frustumSRV; }
    
    // Get the number of visible objects
    UINT GetVisibleObjectCount() const;
    
    // Set LOD levels
    void SetLODLevels(const std::vector<LODLevel>& lodLevels);
    
    // Update LOD levels on GPU
    void UpdateLODLevels();
    
    // Reset visible object count buffer
    void ResetVisibleObjectCount(ID3D11DeviceContext* context);

private:
    bool CreateBuffers(ID3D11Device* device, UINT maxObjects);
    void ReleaseBuffers();

private:
    // Buffers
    ID3D11Buffer* m_objectDataBuffer;
    ID3D11Buffer* m_lodLevelsBuffer;
    ID3D11Buffer* m_frustumBuffer;
    ID3D11Buffer* m_drawCommandBuffer;
    ID3D11Buffer* m_visibleObjectCountBuffer;
    ID3D11Buffer* m_visibleObjectCountStagingBuffer;
    
    // Views
    ID3D11ShaderResourceView* m_objectDataSRV;
    ID3D11ShaderResourceView* m_lodLevelsSRV;
    ID3D11ShaderResourceView* m_frustumSRV;
    ID3D11UnorderedAccessView* m_drawCommandUAV;
    ID3D11UnorderedAccessView* m_visibleObjectCountUAV;
    
    // Data
    std::vector<LODLevel> m_lodLevels;
    UINT m_maxObjects;
    UINT m_visibleObjectCount;
};

#endif // INDIRECT_DRAW_BUFFER_H 