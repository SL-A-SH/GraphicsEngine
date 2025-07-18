#ifndef INDIRECT_DRAW_BUFFER_H
#define INDIRECT_DRAW_BUFFER_H

#include <d3d11.h>
#include <directxmath.h>
#include <vector>

using namespace DirectX;

// Object data structure for GPU processing (MINIMAL VERSION)
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

// World matrix structure for GPU-driven rendering
// Using XMFLOAT4X4 to match the shader expectation
typedef XMFLOAT4X4 WorldMatrix;

class IndirectDrawBuffer
{
public:
    IndirectDrawBuffer();
    ~IndirectDrawBuffer();

    bool Initialize(ID3D11Device* device, UINT maxObjects);
    void Shutdown();

    // Update object data
    void UpdateObjectData(ID3D11DeviceContext* context, const std::vector<ObjectData>& objects);
    
    // Get buffers for minimal GPU-driven rendering
    ID3D11Buffer* GetObjectDataBuffer() const { return m_objectDataBuffer; }
    ID3D11Buffer* GetWorldMatrixBuffer() const { return m_worldMatrixBuffer; }
    
    // Get SRVs and UAVs for minimal compute shader pipeline
    ID3D11ShaderResourceView* GetObjectDataSRV() const { return m_objectDataSRV; }
    ID3D11ShaderResourceView* GetWorldMatrixSRV() const { return m_worldMatrixSRV; }
    ID3D11UnorderedAccessView* GetWorldMatrixUAV() const { return m_worldMatrixUAV; }
    
    // Get the number of objects currently stored
    UINT GetObjectCount() const { return m_objectCount; }

private:
    bool CreateBuffers(ID3D11Device* device, UINT maxObjects);
    void ReleaseBuffers();

private:
    // MINIMAL BUFFERS: Only object data and world matrices
    ID3D11Buffer* m_objectDataBuffer;
    ID3D11Buffer* m_worldMatrixBuffer;
    
    // MINIMAL VIEWS: Only what we need for basic GPU-driven rendering
    ID3D11ShaderResourceView* m_objectDataSRV;
    ID3D11ShaderResourceView* m_worldMatrixSRV;
    ID3D11UnorderedAccessView* m_worldMatrixUAV;
    
    // Data
    UINT m_maxObjects;
    UINT m_objectCount;
};

#endif // INDIRECT_DRAW_BUFFER_H 