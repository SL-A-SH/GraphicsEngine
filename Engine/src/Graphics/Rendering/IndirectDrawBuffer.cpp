#include "IndirectDrawBuffer.h"
#include "../../Core/System/Logger.h"

IndirectDrawBuffer::IndirectDrawBuffer()
    : m_objectDataBuffer(nullptr)
    , m_worldMatrixBuffer(nullptr)
    , m_objectDataSRV(nullptr)
    , m_worldMatrixSRV(nullptr)
    , m_worldMatrixUAV(nullptr)
    , m_maxObjects(0)
    , m_objectCount(0)
{
    LOG("IndirectDrawBuffer: Constructor - Minimal indirect draw buffer created");
}

IndirectDrawBuffer::~IndirectDrawBuffer()
{
    Shutdown();
}

bool IndirectDrawBuffer::Initialize(ID3D11Device* device, UINT maxObjects)
{
    LOG("IndirectDrawBuffer: Initialize - Starting minimal indirect draw buffer initialization");
    LOG("IndirectDrawBuffer: Initialize - Max objects: " + std::to_string(maxObjects));
    
    m_maxObjects = maxObjects;
    
    bool result = CreateBuffers(device, maxObjects);
    if (!result)
    {
        LOG_ERROR("IndirectDrawBuffer: Failed to create indirect draw buffers");
        return false;
    }
    
    LOG("IndirectDrawBuffer: Minimal indirect draw buffer initialized with " + std::to_string(maxObjects) + " max objects");
    return true;
}

void IndirectDrawBuffer::Shutdown()
{
    LOG("IndirectDrawBuffer: Shutdown - Releasing minimal buffers");
    ReleaseBuffers();
    LOG("IndirectDrawBuffer: Shutdown completed");
}

bool IndirectDrawBuffer::CreateBuffers(ID3D11Device* device, UINT maxObjects)
{
    HRESULT result;
    D3D11_BUFFER_DESC bufferDesc;
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
    
    LOG("IndirectDrawBuffer: CreateBuffers - Creating minimal buffers for " + std::to_string(maxObjects) + " objects");
    LOG("IndirectDrawBuffer: CreateBuffers - ObjectData size: " + std::to_string(sizeof(ObjectData)) + " bytes");
    LOG("IndirectDrawBuffer: CreateBuffers - XMFLOAT4X4 size: " + std::to_string(sizeof(XMFLOAT4X4)) + " bytes");
    
    // Create object data buffer (input for compute shader)
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.ByteWidth = sizeof(ObjectData) * maxObjects;
    bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    bufferDesc.StructureByteStride = sizeof(ObjectData);
    
    LOG("IndirectDrawBuffer: CreateBuffers - Creating object data buffer - ByteWidth: " + std::to_string(bufferDesc.ByteWidth) + 
        ", StructureByteStride: " + std::to_string(bufferDesc.StructureByteStride));
    
    result = device->CreateBuffer(&bufferDesc, nullptr, &m_objectDataBuffer);
    if (FAILED(result))
    {
        LOG_ERROR("IndirectDrawBuffer: CreateBuffers - Failed to create object data buffer - HRESULT: " + std::to_string(result));
        return false;
    }
    LOG("IndirectDrawBuffer: CreateBuffers - Object data buffer created successfully");
    
    // Create world matrix buffer (output from compute shader, input to vertex shader)
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth = sizeof(XMFLOAT4X4) * maxObjects;
    bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
    bufferDesc.CPUAccessFlags = 0;
    bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    bufferDesc.StructureByteStride = sizeof(XMFLOAT4X4);
    
    LOG("IndirectDrawBuffer: CreateBuffers - Creating world matrix buffer - ByteWidth: " + std::to_string(bufferDesc.ByteWidth) + 
        ", StructureByteStride: " + std::to_string(bufferDesc.StructureByteStride));
    
    result = device->CreateBuffer(&bufferDesc, nullptr, &m_worldMatrixBuffer);
    if (FAILED(result))
    {
        LOG_ERROR("IndirectDrawBuffer: CreateBuffers - Failed to create world matrix buffer - HRESULT: " + std::to_string(result));
        return false;
    }
    LOG("IndirectDrawBuffer: CreateBuffers - World matrix buffer created successfully");
    
    // Create object data SRV
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = maxObjects;
    
    result = device->CreateShaderResourceView(m_objectDataBuffer, &srvDesc, &m_objectDataSRV);
    if (FAILED(result))
    {
        LOG_ERROR("IndirectDrawBuffer: CreateBuffers - Failed to create object data SRV - HRESULT: " + std::to_string(result));
        return false;
    }
    LOG("IndirectDrawBuffer: CreateBuffers - Object data SRV created successfully");
    
    // Create world matrix SRV (for vertex shader)
    srvDesc.Buffer.NumElements = maxObjects;
    result = device->CreateShaderResourceView(m_worldMatrixBuffer, &srvDesc, &m_worldMatrixSRV);
    if (FAILED(result))
    {
        LOG_ERROR("IndirectDrawBuffer: CreateBuffers - Failed to create world matrix SRV - HRESULT: " + std::to_string(result));
        return false;
    }
    LOG("IndirectDrawBuffer: CreateBuffers - World matrix SRV created successfully");
    
    // Create world matrix UAV (for compute shader)
    uavDesc.Format = DXGI_FORMAT_UNKNOWN;
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.NumElements = maxObjects;
    uavDesc.Buffer.Flags = 0;
    
    result = device->CreateUnorderedAccessView(m_worldMatrixBuffer, &uavDesc, &m_worldMatrixUAV);
    if (FAILED(result))
    {
        LOG_ERROR("IndirectDrawBuffer: CreateBuffers - Failed to create world matrix UAV - HRESULT: " + std::to_string(result));
        return false;
    }
    LOG("IndirectDrawBuffer: CreateBuffers - World matrix UAV created successfully");
    
    LOG("IndirectDrawBuffer: CreateBuffers - All minimal buffers created successfully");
    return true;
}

void IndirectDrawBuffer::ReleaseBuffers()
{
    if (m_objectDataBuffer) { m_objectDataBuffer->Release(); m_objectDataBuffer = nullptr; }
    if (m_worldMatrixBuffer) { m_worldMatrixBuffer->Release(); m_worldMatrixBuffer = nullptr; }
    
    if (m_objectDataSRV) { m_objectDataSRV->Release(); m_objectDataSRV = nullptr; }
    if (m_worldMatrixSRV) { m_worldMatrixSRV->Release(); m_worldMatrixSRV = nullptr; }
    if (m_worldMatrixUAV) { m_worldMatrixUAV->Release(); m_worldMatrixUAV = nullptr; }
}

void IndirectDrawBuffer::UpdateObjectData(ID3D11DeviceContext* context, const std::vector<ObjectData>& objects)
{
    if (objects.empty()) 
    {
        LOG_WARNING("IndirectDrawBuffer: UpdateObjectData - No objects provided");
        return;
    }
    
    // Store the actual number of objects
    m_objectCount = static_cast<UINT>(objects.size());
    // Removed frequent logging - was causing FPS drops in GPU mode
    
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT result = context->Map(m_objectDataBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (SUCCEEDED(result))
    {
        ObjectData* dataPtr = static_cast<ObjectData*>(mappedResource.pData);
        memcpy(dataPtr, objects.data(), sizeof(ObjectData) * objects.size());
        context->Unmap(m_objectDataBuffer, 0);
        // Removed frequent logging - was causing FPS drops in GPU mode
    }
    else
    {
        LOG_ERROR("IndirectDrawBuffer: UpdateObjectData - Failed to map object data buffer - HRESULT: " + std::to_string(result));
    }
} 