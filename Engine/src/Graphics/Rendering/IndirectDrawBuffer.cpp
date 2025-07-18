#include "IndirectDrawBuffer.h"
#include "../../Core/System/Logger.h"

IndirectDrawBuffer::IndirectDrawBuffer()
    : m_objectDataBuffer(nullptr)
    , m_lodLevelsBuffer(nullptr)
    , m_frustumBuffer(nullptr)
    , m_drawCommandBuffer(nullptr)
    , m_visibleObjectCountBuffer(nullptr)
    , m_worldMatrixBuffer(nullptr)
    , m_objectDataSRV(nullptr)
    , m_lodLevelsSRV(nullptr)
    , m_frustumSRV(nullptr)
    , m_drawCommandUAV(nullptr)
    , m_visibleObjectCountUAV(nullptr)
    , m_worldMatrixSRV(nullptr)
    , m_worldMatrixUAV(nullptr)
    , m_maxObjects(0)
    , m_visibleObjectCount(0)
    , m_objectCount(0)
{
}

IndirectDrawBuffer::~IndirectDrawBuffer()
{
    Shutdown();
}

bool IndirectDrawBuffer::Initialize(ID3D11Device* device, UINT maxObjects)
{
    m_maxObjects = maxObjects;
    
    bool result = CreateBuffers(device, maxObjects);
    if (!result)
    {
        LOG_ERROR("Failed to create indirect draw buffers");
        return false;
    }
    
    // Set up default LOD levels - using full model index count for all LODs
    // In a real implementation, you would have different LOD meshes with different index counts
    m_lodLevels.resize(4);
    m_lodLevels[0] = { 0.0f, 61260, 0, 0, 0 };      // High detail (full model)
    m_lodLevels[1] = { 50.0f, 61260, 0, 0, 0 };     // Medium detail (full model for now)
    m_lodLevels[2] = { 100.0f, 61260, 0, 0, 0 };   // Low detail (full model for now)
    m_lodLevels[3] = { 200.0f, 61260, 0, 0, 0 };   // Very low detail (full model for now)
    
    // Upload LOD levels to GPU buffer
    UpdateLODLevels();
    
    LOG("Indirect draw buffer initialized with " + std::to_string(maxObjects) + " max objects");
    return true;
}

void IndirectDrawBuffer::Shutdown()
{
    ReleaseBuffers();
}

bool IndirectDrawBuffer::CreateBuffers(ID3D11Device* device, UINT maxObjects)
{
    HRESULT result;
    D3D11_BUFFER_DESC bufferDesc;
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
    
    LOG("Creating indirect draw buffers for " + std::to_string(maxObjects) + " objects");
    LOG("ObjectData size: " + std::to_string(sizeof(ObjectData)) + " bytes");
    LOG("LODLevel size: " + std::to_string(sizeof(LODLevel)) + " bytes");
    LOG("FrustumData size: " + std::to_string(sizeof(FrustumData)) + " bytes");
    LOG("DrawCommand size: " + std::to_string(sizeof(DrawCommand)) + " bytes");
    
    // Create object data buffer
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.ByteWidth = sizeof(ObjectData) * maxObjects;
    bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    bufferDesc.StructureByteStride = sizeof(ObjectData);
    
    LOG("Creating object data buffer - ByteWidth: " + std::to_string(bufferDesc.ByteWidth) + 
        ", StructureByteStride: " + std::to_string(bufferDesc.StructureByteStride));
    
    result = device->CreateBuffer(&bufferDesc, nullptr, &m_objectDataBuffer);
    if (FAILED(result))
    {
        LOG_ERROR("Failed to create object data buffer - HRESULT: " + std::to_string(result));
        return false;
    }
    LOG("Object data buffer created successfully");
    
    // Create LOD levels buffer
    bufferDesc.ByteWidth = sizeof(LODLevel) * 4; // 4 LOD levels
    bufferDesc.StructureByteStride = sizeof(LODLevel);
    
    result = device->CreateBuffer(&bufferDesc, nullptr, &m_lodLevelsBuffer);
    if (FAILED(result))
    {
        LOG_ERROR("Failed to create LOD levels buffer - HRESULT: " + std::to_string(result));
        return false;
    }
    
    // Create frustum buffer
    bufferDesc.ByteWidth = sizeof(FrustumData);
    bufferDesc.StructureByteStride = sizeof(FrustumData);
    
    result = device->CreateBuffer(&bufferDesc, nullptr, &m_frustumBuffer);
    if (FAILED(result))
    {
        LOG_ERROR("Failed to create frustum buffer - HRESULT: " + std::to_string(result));
        return false;
    }
    
    // Create draw command buffer (UAV)
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth = sizeof(DrawCommand) * maxObjects;
    bufferDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
    bufferDesc.CPUAccessFlags = 0;
    bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    bufferDesc.StructureByteStride = sizeof(DrawCommand);
    
    result = device->CreateBuffer(&bufferDesc, nullptr, &m_drawCommandBuffer);
    if (FAILED(result))
    {
        LOG_ERROR("Failed to create draw command buffer - HRESULT: " + std::to_string(result));
        return false;
    }
    
    // Create visible object count buffer (UAV) - expanded for debug support
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth = sizeof(UINT) * 1000; // Support 1000 UINT elements for debug
    bufferDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
    bufferDesc.CPUAccessFlags = 0;
    bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    bufferDesc.StructureByteStride = sizeof(UINT);
    
    result = device->CreateBuffer(&bufferDesc, nullptr, &m_visibleObjectCountBuffer);
    if (FAILED(result))
    {
        LOG_ERROR("Failed to create visible object count buffer - HRESULT: " + std::to_string(result));
        return false;
    }
    
    // Create staging buffer for reading visible object count
    bufferDesc.Usage = D3D11_USAGE_STAGING;
    bufferDesc.ByteWidth = sizeof(UINT) * 1000; // Match the expanded buffer size
    bufferDesc.BindFlags = 0;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    bufferDesc.MiscFlags = 0;
    
    result = device->CreateBuffer(&bufferDesc, nullptr, &m_visibleObjectCountStagingBuffer);
    if (FAILED(result))
    {
        LOG_ERROR("Failed to create visible object count staging buffer - HRESULT: " + std::to_string(result));
        return false;
    }
    
    // Create world matrix buffer
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth = sizeof(XMFLOAT4X4) * maxObjects;
    bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
    bufferDesc.CPUAccessFlags = 0;
    bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    bufferDesc.StructureByteStride = sizeof(XMFLOAT4X4);
    
    result = device->CreateBuffer(&bufferDesc, nullptr, &m_worldMatrixBuffer);
    if (FAILED(result))
    {
        LOG_ERROR("Failed to create world matrix buffer - HRESULT: " + std::to_string(result));
        return false;
    }
    
    // Create SRVs
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = maxObjects;
    
    result = device->CreateShaderResourceView(m_objectDataBuffer, &srvDesc, &m_objectDataSRV);
    if (FAILED(result))
    {
        LOG_ERROR("Failed to create object data SRV - HRESULT: " + std::to_string(result));
        return false;
    }
    
    srvDesc.Buffer.NumElements = 4; // 4 LOD levels
    result = device->CreateShaderResourceView(m_lodLevelsBuffer, &srvDesc, &m_lodLevelsSRV);
    if (FAILED(result))
    {
        LOG_ERROR("Failed to create LOD levels SRV - HRESULT: " + std::to_string(result));
        return false;
    }
    
    srvDesc.Buffer.NumElements = 1;
    result = device->CreateShaderResourceView(m_frustumBuffer, &srvDesc, &m_frustumSRV);
    if (FAILED(result))
    {
        LOG_ERROR("Failed to create frustum SRV - HRESULT: " + std::to_string(result));
        return false;
    }
    
    // Create SRV for world matrix buffer
    srvDesc.Buffer.NumElements = maxObjects;
    result = device->CreateShaderResourceView(m_worldMatrixBuffer, &srvDesc, &m_worldMatrixSRV);
    if (FAILED(result))
    {
        LOG_ERROR("Failed to create world matrix SRV - HRESULT: " + std::to_string(result));
        return false;
    }
    
    // Create UAVs
    uavDesc.Format = DXGI_FORMAT_UNKNOWN;
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.NumElements = maxObjects;
    uavDesc.Buffer.Flags = 0;
    
    result = device->CreateUnorderedAccessView(m_drawCommandBuffer, &uavDesc, &m_drawCommandUAV);
    if (FAILED(result))
    {
        LOG_ERROR("Failed to create draw command UAV - HRESULT: " + std::to_string(result));
        return false;
    }
    
    // Create UAV for visible object count buffer (structured buffer)
    uavDesc.Format = DXGI_FORMAT_UNKNOWN;
    uavDesc.Buffer.NumElements = 1000; // Match the expanded buffer size
    uavDesc.Buffer.Flags = 0;
    
    result = device->CreateUnorderedAccessView(m_visibleObjectCountBuffer, &uavDesc, &m_visibleObjectCountUAV);
    if (FAILED(result))
    {
        LOG_ERROR("Failed to create visible object count UAV - HRESULT: " + std::to_string(result));
        return false;
    }
    
    // Create UAV for world matrix buffer
    uavDesc.Buffer.NumElements = maxObjects;
    result = device->CreateUnorderedAccessView(m_worldMatrixBuffer, &uavDesc, &m_worldMatrixUAV);
    if (FAILED(result))
    {
        LOG_ERROR("Failed to create world matrix UAV - HRESULT: " + std::to_string(result));
        return false;
    }
    
    return true;
}

void IndirectDrawBuffer::ReleaseBuffers()
{
    if (m_objectDataBuffer) { m_objectDataBuffer->Release(); m_objectDataBuffer = nullptr; }
    if (m_lodLevelsBuffer) { m_lodLevelsBuffer->Release(); m_lodLevelsBuffer = nullptr; }
    if (m_frustumBuffer) { m_frustumBuffer->Release(); m_frustumBuffer = nullptr; }
    if (m_drawCommandBuffer) { m_drawCommandBuffer->Release(); m_drawCommandBuffer = nullptr; }
    if (m_visibleObjectCountBuffer) { m_visibleObjectCountBuffer->Release(); m_visibleObjectCountBuffer = nullptr; }
    if (m_visibleObjectCountStagingBuffer) { m_visibleObjectCountStagingBuffer->Release(); m_visibleObjectCountStagingBuffer = nullptr; }
    if (m_worldMatrixBuffer) { m_worldMatrixBuffer->Release(); m_worldMatrixBuffer = nullptr; }
    
    if (m_objectDataSRV) { m_objectDataSRV->Release(); m_objectDataSRV = nullptr; }
    if (m_lodLevelsSRV) { m_lodLevelsSRV->Release(); m_lodLevelsSRV = nullptr; }
    if (m_frustumSRV) { m_frustumSRV->Release(); m_frustumSRV = nullptr; }
    if (m_worldMatrixSRV) { m_worldMatrixSRV->Release(); m_worldMatrixSRV = nullptr; }
    if (m_drawCommandUAV) { m_drawCommandUAV->Release(); m_drawCommandUAV = nullptr; }
    if (m_visibleObjectCountUAV) { m_visibleObjectCountUAV->Release(); m_visibleObjectCountUAV = nullptr; }
    if (m_worldMatrixUAV) { m_worldMatrixUAV->Release(); m_worldMatrixUAV = nullptr; }
}

void IndirectDrawBuffer::UpdateObjectData(ID3D11DeviceContext* context, const std::vector<ObjectData>& objects)
{
    if (objects.empty()) return;
    
    // Store the actual number of objects
    m_objectCount = static_cast<UINT>(objects.size());
    
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT result = context->Map(m_objectDataBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (SUCCEEDED(result))
    {
        ObjectData* dataPtr = static_cast<ObjectData*>(mappedResource.pData);
        memcpy(dataPtr, objects.data(), sizeof(ObjectData) * objects.size());
        context->Unmap(m_objectDataBuffer, 0);
    }
    
    // TEMPORARILY DISABLED: Reset visible object count to 0 before compute shader runs
    // ResetVisibleObjectCount(context);  // Disabled to prevent debug buffer corruption
}

void IndirectDrawBuffer::UpdateFrustumData(ID3D11DeviceContext* context, const FrustumData& frustumData)
{
    // Store the frustum data locally
    m_frustumData = frustumData;
    
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT result = context->Map(m_frustumBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (SUCCEEDED(result))
    {
        FrustumData* dataPtr = static_cast<FrustumData*>(mappedResource.pData);
        *dataPtr = frustumData;
        context->Unmap(m_frustumBuffer, 0);
    }
}

UINT IndirectDrawBuffer::GetVisibleObjectCount() const
{
    if (!m_visibleObjectCountBuffer || !m_visibleObjectCountStagingBuffer)
        return 0;
    
    // Create a temporary device context to read the buffer
    ID3D11Device* device = nullptr;
    m_visibleObjectCountBuffer->GetDevice(&device);
    if (!device)
        return 0;
    
    ID3D11DeviceContext* context = nullptr;
    device->GetImmediateContext(&context);
    if (!context)
    {
        device->Release();
        return 0;
    }
    
    // Copy from GPU buffer to staging buffer
    context->CopyResource(m_visibleObjectCountStagingBuffer, m_visibleObjectCountBuffer);
    
    // Map the staging buffer to read the visible object count
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT result = context->Map(m_visibleObjectCountStagingBuffer, 0, D3D11_MAP_READ, 0, &mappedResource);
    if (SUCCEEDED(result))
    {
        UINT count = *static_cast<UINT*>(mappedResource.pData);
        context->Unmap(m_visibleObjectCountStagingBuffer, 0);
        context->Release();
        device->Release();
        return count;
    }
    else
    {
        LOG_ERROR("Failed to read visible object count from staging buffer - HRESULT: " + std::to_string(result));
        context->Release();
        device->Release();
        return 0;
    }
}

void IndirectDrawBuffer::SetLODLevels(const std::vector<LODLevel>& lodLevels)
{
    m_lodLevels = lodLevels;
    UpdateLODLevels();
}

void IndirectDrawBuffer::UpdateLODLevels()
{
    if (!m_lodLevelsBuffer || m_lodLevels.empty())
        return;
    
    // For now, we'll use a simple approach - create a temporary device context
    // In a real implementation, you'd want to pass the device context as a parameter
    // or store it as a member variable
    
    // Create a temporary device context for uploading LOD data
    ID3D11Device* device = nullptr;
    m_lodLevelsBuffer->GetDevice(&device);
    if (!device)
        return;
    
    ID3D11DeviceContext* context = nullptr;
    device->GetImmediateContext(&context);
    if (!context)
    {
        device->Release();
        return;
    }
    
    // Map and upload LOD data
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT result = context->Map(m_lodLevelsBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (SUCCEEDED(result))
    {
        LODLevel* dataPtr = static_cast<LODLevel*>(mappedResource.pData);
        memcpy(dataPtr, m_lodLevels.data(), sizeof(LODLevel) * m_lodLevels.size());
        context->Unmap(m_lodLevelsBuffer, 0);
    }
    else
    {
        LOG_ERROR("Failed to upload LOD levels to GPU buffer - HRESULT: " + std::to_string(result));
    }
    
    context->Release();
    device->Release();
}

void IndirectDrawBuffer::ResetVisibleObjectCount(ID3D11DeviceContext* context)
{
    if (!m_visibleObjectCountBuffer)
        return;
    
    // Get the actual buffer size to match our expanded debug buffer
    D3D11_BUFFER_DESC actualBufferDesc = {};
    m_visibleObjectCountBuffer->GetDesc(&actualBufferDesc);
    
    // Create a temporary buffer with the same size and initial value of 0
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth = actualBufferDesc.ByteWidth; // Match the actual buffer size (4000 bytes)
    bufferDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
    bufferDesc.CPUAccessFlags = 0;
    bufferDesc.MiscFlags = 0;
    
    ID3D11Buffer* tempBuffer = nullptr;
    ID3D11Device* device = nullptr;
    m_visibleObjectCountBuffer->GetDevice(&device);
    if (!device)
        return;
    
    // Create zero data for the entire buffer
    std::vector<UINT> zeroData(actualBufferDesc.ByteWidth / sizeof(UINT), 0);
    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = zeroData.data();
    
    HRESULT result = device->CreateBuffer(&bufferDesc, &initData, &tempBuffer);
    if (SUCCEEDED(result))
    {
        // Copy the zeroed buffer to our visible object count buffer
        context->CopyResource(m_visibleObjectCountBuffer, tempBuffer);
        tempBuffer->Release();
        m_visibleObjectCount = 0;
    }
    else
    {
        LOG_ERROR("Failed to create temporary buffer for reset - HRESULT: " + std::to_string(result));
    }
    
    device->Release();
} 