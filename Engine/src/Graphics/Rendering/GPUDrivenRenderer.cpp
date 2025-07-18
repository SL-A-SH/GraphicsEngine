#include "GPUDrivenRenderer.h"
#include "../../Core/System/Logger.h"
#include "../../Core/System/PerformanceProfiler.h"
#include "../../Core/Application/Application.h"
#include "../../Core/Common/EngineTypes.h"
#include "../Resource/Model.h"
#include "Light.h"
#include "../Rendering/Camera.h"
#include "../Shaders/PBRShader.h"
#include "../D3D11/D3D11Device.h"

GPUDrivenRenderer::GPUDrivenRenderer()
    : m_frustumCullingCS(nullptr)
    , m_lodSelectionCS(nullptr)
    , m_commandGenerationCS(nullptr)
    , m_worldMatrixGenerationCS(nullptr)
    , m_enableGPUDriven(true)
    , m_drawCallCount(0)
    , m_maxObjects(0)
    , m_cameraPosition(0.0f, 0.0f, 0.0f)
    , m_cameraForward(0.0f, 0.0f, 1.0f)
    , m_viewMatrix(XMMatrixIdentity())
    , m_projectionMatrix(XMMatrixIdentity())
{
}

GPUDrivenRenderer::~GPUDrivenRenderer()
{
    Shutdown();
}

bool GPUDrivenRenderer::Initialize(ID3D11Device* device, HWND hwnd, UINT maxObjects)
{
    m_maxObjects = maxObjects;
    
    // Initialize indirect draw buffer
    bool result = m_indirectBuffer.Initialize(device, maxObjects);
    if (!result)
    {
        LOG_ERROR("Failed to initialize indirect draw buffer");
        return false;
    }
    
    // Initialize compute shaders
    result = InitializeComputeShaders(device, hwnd);
    if (!result)
    {
        LOG_ERROR("Failed to initialize compute shaders");
        return false;
    }
    
    LOG("GPU-driven renderer initialized with " + std::to_string(maxObjects) + " max objects");
    return true;
}

void GPUDrivenRenderer::Shutdown()
{
    ReleaseComputeShaders();
    m_indirectBuffer.Shutdown();
}

bool GPUDrivenRenderer::InitializeComputeShaders(ID3D11Device* device, HWND hwnd)
{
    LOG("GPUDrivenRenderer: Starting compute shader initialization");
    
    // Create compute shaders
    m_frustumCullingCS = new ComputeShader();
    m_lodSelectionCS = new ComputeShader();
    m_commandGenerationCS = new ComputeShader();
    m_worldMatrixGenerationCS = new ComputeShader();
    
    // Initialize compute shaders with correct file paths
    LOG("GPUDrivenRenderer: Initializing frustum culling compute shader");
    bool result = m_frustumCullingCS->Initialize(device, hwnd, L"../Engine/assets/shaders/FrustumCullingComputeShader.hlsl", "main");
    if (!result)
    {
        LOG_ERROR("Failed to initialize frustum culling compute shader");
        return false;
    }
    LOG("GPUDrivenRenderer: Frustum culling compute shader initialized successfully");
    
    LOG("GPUDrivenRenderer: Initializing LOD selection compute shader");
    result = m_lodSelectionCS->Initialize(device, hwnd, L"../Engine/assets/shaders/LODSelectionComputeShader.hlsl", "main");
    if (!result)
    {
        LOG_ERROR("Failed to initialize LOD selection compute shader");
        return false;
    }
    LOG("GPUDrivenRenderer: LOD selection compute shader initialized successfully");
    
    LOG("GPUDrivenRenderer: Initializing command generation compute shader");
    result = m_commandGenerationCS->Initialize(device, hwnd, L"../Engine/assets/shaders/CommandGenerationComputeShader.hlsl", "main");
    if (!result)
    {
        LOG_ERROR("Failed to initialize command generation compute shader");
        return false;
    }
    LOG("GPUDrivenRenderer: Command generation compute shader initialized successfully");
    
    LOG("GPUDrivenRenderer: Initializing world matrix generation compute shader");
    result = m_worldMatrixGenerationCS->Initialize(device, hwnd, L"../Engine/assets/shaders/WorldMatrixGenerationComputeShader.hlsl", "main");
    if (!result)
    {
        LOG_ERROR("Failed to initialize world matrix generation compute shader");
        return false;
    }
    LOG("GPUDrivenRenderer: World matrix generation compute shader initialized successfully");
    
    // Verify all compute shaders are valid
    LOG("GPUDrivenRenderer: Verifying compute shaders are valid");
    if (!m_frustumCullingCS->GetComputeShader())
    {
        LOG_ERROR("Frustum culling compute shader is null after initialization");
        return false;
    }
    if (!m_lodSelectionCS->GetComputeShader())
    {
        LOG_ERROR("LOD selection compute shader is null after initialization");
        return false;
    }
    if (!m_commandGenerationCS->GetComputeShader())
    {
        LOG_ERROR("Command generation compute shader is null after initialization");
        return false;
    }
    
    if (!m_worldMatrixGenerationCS->GetComputeShader())
    {
        LOG_ERROR("World matrix generation compute shader is null after initialization");
        return false;
    }
    
    LOG("GPUDrivenRenderer: All compute shaders initialized and verified successfully");
    return true;
}

void GPUDrivenRenderer::ReleaseComputeShaders()
{
    if (m_frustumCullingCS)
    {
        m_frustumCullingCS->Shutdown();
        delete m_frustumCullingCS;
        m_frustumCullingCS = nullptr;
    }
    
    if (m_lodSelectionCS)
    {
        m_lodSelectionCS->Shutdown();
        delete m_lodSelectionCS;
        m_lodSelectionCS = nullptr;
    }
    
    if (m_commandGenerationCS)
    {
        m_commandGenerationCS->Shutdown();
        delete m_commandGenerationCS;
        m_commandGenerationCS = nullptr;
    }
    
    if (m_worldMatrixGenerationCS)
    {
        m_worldMatrixGenerationCS->Shutdown();
        delete m_worldMatrixGenerationCS;
        m_worldMatrixGenerationCS = nullptr;
    }
}

void GPUDrivenRenderer::UpdateObjects(ID3D11DeviceContext* context, const std::vector<ObjectData>& objects)
{
    if (objects.empty())
    {
        LOG_WARNING("GPUDrivenRenderer::UpdateObjects - No objects provided");
        return;
    }
    
    m_indirectBuffer.UpdateObjectData(context, objects);
}

void GPUDrivenRenderer::UpdateCamera(ID3D11DeviceContext* context, const XMFLOAT3& cameraPos, const XMFLOAT3& cameraForward,
                                   const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix, bool debugLogging)
{
    m_cameraPosition = cameraPos;
    m_cameraForward = cameraForward;
    m_viewMatrix = viewMatrix;
    m_projectionMatrix = projectionMatrix;
    
    // Generate frustum data and update buffer
    FrustumData frustumData = GenerateFrustumData(viewMatrix, projectionMatrix);
    
    // Debug: Log frustum planes for comparison with CPU mode
    if (debugLogging)
    {
        LOG("GPU-Driven Frustum Planes:");
        for (int i = 0; i < 6; i++)
        {
            LOG("  Plane " + std::to_string(i) + ": (" + 
                std::to_string(frustumData.frustumPlanes[i].x) + ", " + 
                std::to_string(frustumData.frustumPlanes[i].y) + ", " + 
                std::to_string(frustumData.frustumPlanes[i].z) + ", " + 
                std::to_string(frustumData.frustumPlanes[i].w) + ")");
        }
    }
    
    m_indirectBuffer.UpdateFrustumData(context, frustumData);
}

void GPUDrivenRenderer::Render(ID3D11DeviceContext* context, ID3D11Buffer* vertexBuffer, ID3D11Buffer* indexBuffer,
                              ID3D11VertexShader* vertexShader, ID3D11PixelShader* pixelShader, ID3D11InputLayout* inputLayout,
                              Model* model, PBRShader* pbrShader, Light* light, Camera* camera, D3D11Device* direct3D, bool debugLogging)
{
    if (!m_enableGPUDriven)
    {
        LOG_WARNING("GPU-driven rendering is disabled");
        return;
    }
    
    // Comprehensive validation of all input parameters
    if (debugLogging)
    {
        LOG("GPUDrivenRenderer::Render - Validating input parameters:");
        LOG("  Context: " + std::string(context ? "valid" : "null"));
        LOG("  VertexBuffer: " + std::string(vertexBuffer ? "valid" : "null"));
        LOG("  IndexBuffer: " + std::string(indexBuffer ? "valid" : "null"));
        LOG("  VertexShader: " + std::string(vertexShader ? "valid" : "null"));
        LOG("  PixelShader: " + std::string(pixelShader ? "valid" : "null"));
        LOG("  InputLayout: " + std::string(inputLayout ? "valid" : "null"));
    }
    
    // Validate required resources
    if (!context)
    {
        LOG_ERROR("GPUDrivenRenderer::Render - context is null");
        return;
    }
    
    if (!vertexBuffer)
    {
        LOG_ERROR("GPUDrivenRenderer::Render - vertexBuffer is null");
        return;
    }
    
    if (!indexBuffer)
    {
        LOG_ERROR("GPUDrivenRenderer::Render - indexBuffer is null");
        return;
    }
    
    if (!vertexShader)
    {
        LOG_ERROR("GPUDrivenRenderer::Render - vertexShader is null");
        return;
    }
    
    if (!pixelShader)
    {
        LOG_ERROR("GPUDrivenRenderer::Render - pixelShader is null");
        return;
    }
    
    if (!inputLayout)
    {
        LOG_ERROR("GPUDrivenRenderer::Render - inputLayout is null");
        return;
    }
    
    // Start profiling GPU-driven rendering
    PerformanceProfiler::GetInstance().BeginSection("GPU-Driven Rendering");
    
    // Validate compute shader resources before setting them
    ID3D11ShaderResourceView* objectDataSRV = m_indirectBuffer.GetObjectDataSRV();
    ID3D11ShaderResourceView* lodLevelsSRV = m_indirectBuffer.GetLODLevelsSRV();
    ID3D11ShaderResourceView* frustumSRV = m_indirectBuffer.GetFrustumSRV();
    ID3D11UnorderedAccessView* drawCommandUAV = m_indirectBuffer.GetDrawCommandUAV();
    ID3D11UnorderedAccessView* visibleObjectCountUAV = m_indirectBuffer.GetVisibleObjectCountUAV();
    ID3D11Buffer* frustumBuffer = m_indirectBuffer.GetFrustumBuffer();
    
    if (debugLogging)
    {
        LOG("GPUDrivenRenderer::Render - Compute shader resource validation:");
        LOG("  ObjectDataSRV: " + std::string(objectDataSRV ? "valid" : "null"));
        LOG("  LODLevelsSRV: " + std::string(lodLevelsSRV ? "valid" : "null"));
        LOG("  FrustumSRV: " + std::string(frustumSRV ? "valid" : "null"));
        LOG("  DrawCommandUAV: " + std::string(drawCommandUAV ? "valid" : "null"));
        LOG("  VisibleObjectCountUAV: " + std::string(visibleObjectCountUAV ? "valid" : "null"));
        LOG("  FrustumBuffer: " + std::string(frustumBuffer ? "valid" : "null"));
    }
    
    if (!objectDataSRV || !lodLevelsSRV || !frustumSRV || !drawCommandUAV || !visibleObjectCountUAV || !frustumBuffer)
    {
        LOG_ERROR("GPUDrivenRenderer::Render - One or more compute shader resources are null");
        LOG_ERROR("ObjectDataSRV: " + std::string(objectDataSRV ? "valid" : "null"));
        LOG_ERROR("LODLevelsSRV: " + std::string(lodLevelsSRV ? "valid" : "null"));
        LOG_ERROR("FrustumSRV: " + std::string(frustumSRV ? "valid" : "null"));
        LOG_ERROR("DrawCommandUAV: " + std::string(drawCommandUAV ? "valid" : "null"));
        LOG_ERROR("VisibleObjectCountUAV: " + std::string(visibleObjectCountUAV ? "valid" : "null"));
        LOG_ERROR("FrustumBuffer: " + std::string(frustumBuffer ? "valid" : "null"));
        return;
    }
    

    
    // Generate world matrices for all objects
    if (debugLogging)
    {
        LOG("GPUDrivenRenderer::Render - Generating world matrices");
    }
    ID3D11UnorderedAccessView* worldMatrixUAV = m_indirectBuffer.GetWorldMatrixUAV();
    if (!worldMatrixUAV)
    {
        LOG_ERROR("GPUDrivenRenderer::Render - World matrix UAV is null");
        return;
    }
    
    // Set up world matrix generation compute shader resources
    if (debugLogging)
    {
        LOG("GPUDrivenRenderer::Render - Setting up world matrix generation compute shader resources");
        LOG("GPUDrivenRenderer::Render - ObjectDataSRV pointer: " + std::to_string(reinterpret_cast<uintptr_t>(objectDataSRV)));
        LOG("GPUDrivenRenderer::Render - WorldMatrixUAV pointer: " + std::to_string(reinterpret_cast<uintptr_t>(worldMatrixUAV)));
    }
    
    // DEBUG: Skip clearing debug UAV buffer to prevent interference
    if (debugLogging)
    {
        LOG("GPUDrivenRenderer::Render - Skipping debug UAV buffer clearing to prevent interference");
    }
    
    // FIXED: Use correct UAV bindings - world matrix at slot 0, debug at slot 1
    m_worldMatrixGenerationCS->SetShaderResourceView(context, 0, objectDataSRV);
    m_worldMatrixGenerationCS->SetUnorderedAccessView(context, 0, worldMatrixUAV); // World matrix at slot 0 (correct slot)
    m_worldMatrixGenerationCS->SetUnorderedAccessView(context, 1, visibleObjectCountUAV); // Debug buffer at slot 1
    
    // Create and set object count constant buffer for world matrix generation
    UINT actualObjectCount = m_indirectBuffer.GetObjectCount();
    
    // Create a simple constant buffer for object count
    D3D11_BUFFER_DESC objectCountBufferDesc;
    objectCountBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    objectCountBufferDesc.ByteWidth = sizeof(UINT) * 4; // 16-byte aligned
    objectCountBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    objectCountBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    objectCountBufferDesc.MiscFlags = 0;
    objectCountBufferDesc.StructureByteStride = 0;
    
    ID3D11Buffer* objectCountBuffer = nullptr;
    HRESULT result = direct3D->GetDevice()->CreateBuffer(&objectCountBufferDesc, nullptr, &objectCountBuffer);
    if (FAILED(result))
    {
        LOG_ERROR("GPUDrivenRenderer::Render - Failed to create object count constant buffer");
        return;
    }
    
    // Map and update the constant buffer
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    result = context->Map(objectCountBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (FAILED(result))
    {
        LOG_ERROR("GPUDrivenRenderer::Render - Failed to map object count constant buffer");
        objectCountBuffer->Release();
        return;
    }
    
    UINT* objectCountData = static_cast<UINT*>(mappedResource.pData);
    objectCountData[0] = actualObjectCount;
    objectCountData[1] = 0; // padding
    objectCountData[2] = 0; // padding
    objectCountData[3] = 0; // padding
    
    context->Unmap(objectCountBuffer, 0);
    
    // Set the constant buffer for the world matrix generation compute shader
    m_worldMatrixGenerationCS->SetConstantBuffer(context, 0, objectCountBuffer);
    
    // Validate compute shader before dispatching
    if (!m_worldMatrixGenerationCS || !m_worldMatrixGenerationCS->GetComputeShader())
    {
        LOG_ERROR("GPUDrivenRenderer::Render - World matrix generation compute shader is null or invalid");
        objectCountBuffer->Release();
        return;
    }
    
    // Calculate thread group count for world matrix generation
    UINT worldMatrixThreadGroupCount = (actualObjectCount + 63) / 64;
    
    if (debugLogging)
    {
        LOG("GPUDrivenRenderer::Render - World matrix generation compute shader is valid");
        LOG("GPUDrivenRenderer::Render - Compute shader pointer: " + std::to_string(reinterpret_cast<uintptr_t>(m_worldMatrixGenerationCS->GetComputeShader())));
        LOG("GPUDrivenRenderer::Render - Dispatching world matrix generation with " + std::to_string(worldMatrixThreadGroupCount) + " thread groups for " + std::to_string(actualObjectCount) + " objects");
        LOG("GPUDrivenRenderer::Render - Object count being passed to compute shader: " + std::to_string(actualObjectCount));
        LOG("GPUDrivenRenderer::Render - Thread group calculation: (" + std::to_string(actualObjectCount) + " + 63) / 64 = " + std::to_string(worldMatrixThreadGroupCount));
        LOG("GPUDrivenRenderer::Render - Total threads that will execute: " + std::to_string(worldMatrixThreadGroupCount * 64));
        
        // Debug: Check if the compute shader is actually being set
        LOG("GPUDrivenRenderer::Render - About to set compute shader and dispatch");
        
        // Validate buffer properties for common pitfalls
        LOG("GPUDrivenRenderer::Render - Validating buffer properties:");
        
        // Check object data buffer
        ID3D11Buffer* objectDataBuffer = m_indirectBuffer.GetObjectDataBuffer();
        if (objectDataBuffer)
        {
            D3D11_BUFFER_DESC objDesc = {};
            objectDataBuffer->GetDesc(&objDesc);
            LOG("  ObjectData buffer size: " + std::to_string(objDesc.ByteWidth) + " bytes");
            LOG("  ObjectData element stride: " + std::to_string(objDesc.StructureByteStride) + " bytes");
            LOG("  ObjectData element count: " + std::to_string(objDesc.ByteWidth / objDesc.StructureByteStride));
            LOG("  Expected ObjectData size: " + std::to_string(sizeof(ObjectData)) + " bytes");
        }
        
        // Check world matrix buffer
        ID3D11Buffer* worldMatrixBuffer = m_indirectBuffer.GetWorldMatrixBuffer();
        if (worldMatrixBuffer)
        {
            D3D11_BUFFER_DESC matDesc = {};
            worldMatrixBuffer->GetDesc(&matDesc);
            LOG("  WorldMatrix buffer size: " + std::to_string(matDesc.ByteWidth) + " bytes");
            LOG("  WorldMatrix element stride: " + std::to_string(matDesc.StructureByteStride) + " bytes");
            LOG("  WorldMatrix element count: " + std::to_string(matDesc.ByteWidth / matDesc.StructureByteStride));
            LOG("  Expected WorldMatrix size: " + std::to_string(sizeof(XMFLOAT4X4)) + " bytes");
        }
        
        // Check debug buffer
        ID3D11Buffer* debugBuffer = m_indirectBuffer.GetVisibleObjectCountBuffer();
        if (debugBuffer)
        {
            D3D11_BUFFER_DESC debugDesc = {};
            debugBuffer->GetDesc(&debugDesc);
            LOG("  Debug buffer size: " + std::to_string(debugDesc.ByteWidth) + " bytes");
            LOG("  Debug element stride: " + std::to_string(debugDesc.StructureByteStride) + " bytes");
            LOG("  Debug element count: " + std::to_string(debugDesc.ByteWidth / debugDesc.StructureByteStride));
        }
    }
    
    // DISABLED: Skip clearing world matrix buffer to prevent corruption
    if (debugLogging)
    {
        LOG("GPUDrivenRenderer::Render - Skipping world matrix buffer clearing to prevent test matrix corruption");
    }
    
    // STEP 0: CPU Direct Write Test - Verify buffer integrity
    LOG("GPUDrivenRenderer::Render - STEP 0: Testing CPU direct write to world matrix buffer");
    
    // Create a staging buffer to write identity matrices from CPU
    D3D11_BUFFER_DESC stagingWriteDesc = {};
    stagingWriteDesc.Usage = D3D11_USAGE_STAGING;
    stagingWriteDesc.ByteWidth = sizeof(XMFLOAT4X4) * 3; // Test first 3 matrices
    stagingWriteDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    stagingWriteDesc.BindFlags = 0;
    stagingWriteDesc.MiscFlags = 0;
    
    ID3D11Buffer* cpuWriteBuffer = nullptr;
    ID3D11Device* device = nullptr;
    ID3D11Buffer* worldMatrixBuffer = m_indirectBuffer.GetWorldMatrixBuffer();
    
    if (!worldMatrixBuffer)
    {
        LOG_ERROR("GPUDrivenRenderer::Render - Failed to get world matrix buffer for CPU test");
        return;
    }
    
    worldMatrixBuffer->GetDevice(&device);
    
    if (device && SUCCEEDED(device->CreateBuffer(&stagingWriteDesc, nullptr, &cpuWriteBuffer)))
    {
        // Map the staging buffer and write identity matrices
        D3D11_MAPPED_SUBRESOURCE mappedWrite;
        if (SUCCEEDED(context->Map(cpuWriteBuffer, 0, D3D11_MAP_WRITE, 0, &mappedWrite)))
        {
            XMFLOAT4X4* matrices = static_cast<XMFLOAT4X4*>(mappedWrite.pData);
            
            // Write identity matrices
            for (int i = 0; i < 3; i++)
            {
                matrices[i] = XMFLOAT4X4(
                    1.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, 1.0f, 0.0f, 0.0f,
                    0.0f, 0.0f, 1.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f
                );
            }
            
            context->Unmap(cpuWriteBuffer, 0);
            
            // Copy from staging buffer to world matrix buffer
            D3D11_BOX srcBox = {};
            srcBox.left = 0;
            srcBox.top = 0;
            srcBox.front = 0;
            srcBox.right = sizeof(XMFLOAT4X4) * 3;
            srcBox.bottom = 1;
            srcBox.back = 1;
            
            context->CopySubresourceRegion(worldMatrixBuffer, 0, 0, 0, 0, cpuWriteBuffer, 0, &srcBox);
            
            LOG("GPUDrivenRenderer::Render - CPU identity matrices written to world matrix buffer");
            
            // Test reading back the CPU-written matrices immediately
            D3D11_BUFFER_DESC readBackDesc = {};
            readBackDesc.Usage = D3D11_USAGE_STAGING;
            readBackDesc.ByteWidth = sizeof(XMFLOAT4X4) * 3;
            readBackDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            readBackDesc.BindFlags = 0;
            readBackDesc.MiscFlags = 0;
            
            ID3D11Buffer* readBackBuffer = nullptr;
            if (SUCCEEDED(device->CreateBuffer(&readBackDesc, nullptr, &readBackBuffer)))
            {
                context->CopySubresourceRegion(readBackBuffer, 0, 0, 0, 0, worldMatrixBuffer, 0, &srcBox);
                
                D3D11_MAPPED_SUBRESOURCE mappedRead;
                if (SUCCEEDED(context->Map(readBackBuffer, 0, D3D11_MAP_READ, 0, &mappedRead)))
                {
                    XMFLOAT4X4* readMatrices = static_cast<XMFLOAT4X4*>(mappedRead.pData);
                    
                    LOG("GPUDrivenRenderer::Render - CPU Write Test Results:");
                    for (int i = 0; i < 3; i++)
                    {
                        LOG("  Matrix " + std::to_string(i) + " [0]: " + 
                            std::to_string(readMatrices[i]._11) + ", " + 
                            std::to_string(readMatrices[i]._12) + ", " + 
                            std::to_string(readMatrices[i]._13) + ", " + 
                            std::to_string(readMatrices[i]._14));
                        LOG("  Matrix " + std::to_string(i) + " [3]: " + 
                            std::to_string(readMatrices[i]._41) + ", " + 
                            std::to_string(readMatrices[i]._42) + ", " + 
                            std::to_string(readMatrices[i]._43) + ", " + 
                            std::to_string(readMatrices[i]._44));
                    }
                    
                    context->Unmap(readBackBuffer, 0);
                }
                else
                {
                    LOG_ERROR("GPUDrivenRenderer::Render - Failed to map read-back buffer for CPU test");
                }
                readBackBuffer->Release();
            }
            else
            {
                LOG_ERROR("GPUDrivenRenderer::Render - Failed to create read-back buffer for CPU test");
            }
        }
        else
        {
            LOG_ERROR("GPUDrivenRenderer::Render - Failed to map CPU write buffer");
        }
        cpuWriteBuffer->Release();
        device->Release();
    }
    else
    {
        LOG_ERROR("GPUDrivenRenderer::Render - Failed to create CPU write buffer or get device");
        if (device) device->Release();
    }

    // STEP 1: Test GPU compute shader execution with enhanced diagnostics
    LOG("GPUDrivenRenderer::Render - STEP 1: Testing GPU compute shader execution");
    
    // Validate compute shader is loaded and compiled
    if (!m_worldMatrixGenerationCS->GetComputeShader())
    {
        LOG_ERROR("GPUDrivenRenderer::Render - World matrix compute shader is null!");
        return;
    }
    LOG("GPUDrivenRenderer::Render - Compute shader validation: PASSED");
    
    // Use existing variables to avoid conflicts
    LOG("GPUDrivenRenderer::Render - World matrix UAV validation: PASSED");
    LOG("GPUDrivenRenderer::Render - Object data SRV validation: PASSED");
    
    // Set compute shader and validate
    context->CSSetShader(m_worldMatrixGenerationCS->GetComputeShader(), nullptr, 0);
    LOG("GPUDrivenRenderer::Render - Compute shader set on device context");
    
    // Clear UAV with debug pattern before compute shader
    UINT clearValues[4] = { 0xDEADBEEF, 0xDEADBEEF, 0xDEADBEEF, 0xDEADBEEF };
    context->ClearUnorderedAccessViewUint(worldMatrixUAV, clearValues);
    LOG("GPUDrivenRenderer::Render - Cleared world matrix UAV with debug pattern");
    
    // Use existing binding logic - just add validation logs
    m_worldMatrixGenerationCS->SetUnorderedAccessView(context, 0, worldMatrixUAV);
    LOG("GPUDrivenRenderer::Render - Bound world matrix UAV to slot 0");
    
    m_worldMatrixGenerationCS->SetShaderResourceView(context, 0, objectDataSRV);
    LOG("GPUDrivenRenderer::Render - Bound object data SRV to slot 0");
    
    // Use existing constant buffer setup but add validation
    LOG("GPUDrivenRenderer::Render - Using existing object count constant buffer");
    m_worldMatrixGenerationCS->SetConstantBuffer(context, 0, objectCountBuffer);
    LOG("GPUDrivenRenderer::Render - Bound object count constant buffer");
    
    // Calculate dispatch parameters with validation
    UINT objectCount = 5000;
    UINT threadGroupSize = 64;
    UINT numThreadGroups = (objectCount + threadGroupSize - 1) / threadGroupSize;
    
    LOG("GPUDrivenRenderer::Render - Dispatch parameters:");
    LOG("  Object count: " + std::to_string(objectCount));
    LOG("  Thread group size: " + std::to_string(threadGroupSize));
    LOG("  Number of thread groups: " + std::to_string(numThreadGroups));
    
    if (numThreadGroups == 0)
    {
        LOG_ERROR("GPUDrivenRenderer::Render - Invalid thread group count!");
        return;
    }
    
    // Force device context flush before dispatch
    context->Flush();
    LOG("GPUDrivenRenderer::Render - Device context flushed before dispatch");
    
    // Dispatch compute shader with validation
    LOG("GPUDrivenRenderer::Render - DISPATCHING COMPUTE SHADER NOW...");
    m_worldMatrixGenerationCS->Dispatch(context, numThreadGroups, 1, 1);
    LOG("GPUDrivenRenderer::Render - Compute shader dispatched successfully");
    
    // Force synchronization and flush
    context->Flush();
    LOG("GPUDrivenRenderer::Render - Device context flushed after dispatch");
    
    // Unbind UAV to ensure writes are committed
    m_worldMatrixGenerationCS->SetUnorderedAccessView(context, 0, nullptr);
    LOG("GPUDrivenRenderer::Render - Unbound UAVs to commit writes");
    
    // Additional flush and finish
    context->Flush();
    LOG("GPUDrivenRenderer::Render - Final device context flush completed");
    
    // Skip all command generation and debug validation - test world matrices directly
    if (debugLogging)
    {
        LOG("GPUDrivenRenderer::Render - Testing world matrices from GPU generation");
        
        // Read world matrices immediately after compute shader dispatch
        ID3D11Buffer* worldMatrixBuffer = m_indirectBuffer.GetWorldMatrixBuffer();
        if (worldMatrixBuffer)
        {
            // Create a staging buffer to read back the world matrices
            D3D11_BUFFER_DESC stagingDesc = {};
            stagingDesc.Usage = D3D11_USAGE_STAGING;
            stagingDesc.ByteWidth = sizeof(XMFLOAT4X4) * 2; // Read first 2 matrices
            stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            stagingDesc.BindFlags = 0;
            stagingDesc.MiscFlags = 0;
            
            ID3D11Buffer* stagingBuffer = nullptr;
            ID3D11Device* device = nullptr;
            worldMatrixBuffer->GetDevice(&device);
            if (device)
            {
                HRESULT result = device->CreateBuffer(&stagingDesc, nullptr, &stagingBuffer);
                if (SUCCEEDED(result) && stagingBuffer)
                {
                    // Copy from GPU buffer to staging buffer
                    context->CopyResource(stagingBuffer, worldMatrixBuffer);
                    
                    // Map the staging buffer to read the world matrices
                    D3D11_MAPPED_SUBRESOURCE mappedResource;
                    HRESULT mapResult = context->Map(stagingBuffer, 0, D3D11_MAP_READ, 0, &mappedResource);
                    if (SUCCEEDED(mapResult))
                    {
                        XMFLOAT4X4* matrices = static_cast<XMFLOAT4X4*>(mappedResource.pData);
                        LOG("GPUDrivenRenderer::Render - MINIMAL TEST - World matrix 0:");
                        LOG("  [0]: " + std::to_string(matrices[0]._11) + ", " + std::to_string(matrices[0]._12) + ", " + std::to_string(matrices[0]._13) + ", " + std::to_string(matrices[0]._14));
                        LOG("  [1]: " + std::to_string(matrices[0]._21) + ", " + std::to_string(matrices[0]._22) + ", " + std::to_string(matrices[0]._23) + ", " + std::to_string(matrices[0]._24));
                        LOG("  [2]: " + std::to_string(matrices[0]._31) + ", " + std::to_string(matrices[0]._32) + ", " + std::to_string(matrices[0]._33) + ", " + std::to_string(matrices[0]._34));
                        LOG("  [3]: " + std::to_string(matrices[0]._41) + ", " + std::to_string(matrices[0]._42) + ", " + std::to_string(matrices[0]._43) + ", " + std::to_string(matrices[0]._44));
                        context->Unmap(stagingBuffer, 0);
                    }
                    stagingBuffer->Release();
                }
                device->Release();
            }
        }
    }
    
    // EARLY RETURN: For now, just test world matrix generation - skip all rendering
    LOG("GPUDrivenRenderer::Render - MINIMAL TEST: Skipping rendering - only testing world matrix generation");
    
    // End profiling
    PerformanceProfiler::GetInstance().EndSection();
}

FrustumData GPUDrivenRenderer::GenerateFrustumData(const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix)
{
    FrustumData frustumData;
    
    // Copy camera data
    frustumData.cameraPosition = XMFLOAT4(m_cameraPosition.x, m_cameraPosition.y, m_cameraPosition.z, 1.0f);
    frustumData.cameraForward = XMFLOAT4(m_cameraForward.x, m_cameraForward.y, m_cameraForward.z, 0.0f);
    
    // Set LOD distances
    frustumData.lodDistances[0] = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
    frustumData.lodDistances[1] = XMFLOAT4(50.0f, 0.0f, 0.0f, 0.0f);
    frustumData.lodDistances[2] = XMFLOAT4(100.0f, 0.0f, 0.0f, 0.0f);
    frustumData.lodDistances[3] = XMFLOAT4(200.0f, 0.0f, 0.0f, 0.0f);
    frustumData.maxLODLevels = 4;
    frustumData.objectCount = m_indirectBuffer.GetObjectCount();
    
    // Generate frustum planes using the same method as CPU-driven rendering
    XMMATRIX finalMatrix;
    XMFLOAT4X4 projMatrix, matrix;
    float zMinimum, r, t;
    
    // Load the projection matrix into a XMFLOAT4X4 structure
    XMStoreFloat4x4(&projMatrix, projectionMatrix);
    
    // Calculate the minimum Z distance in the frustum
    zMinimum = -projMatrix._43 / projMatrix._33;
    r = AppConfig::SCREEN_DEPTH / (AppConfig::SCREEN_DEPTH - zMinimum);
    projMatrix._33 = r;
    projMatrix._43 = -r * zMinimum;
    
    // Load the updated XMFLOAT4X4 back into the original projection matrix
    XMMATRIX updatedProjectionMatrix = XMLoadFloat4x4(&projMatrix);
    
    // Create the frustum matrix from the view matrix and updated projection matrix
    finalMatrix = XMMatrixMultiply(viewMatrix, updatedProjectionMatrix);
    
    // Load the final matrix into a XMFLOAT4X4 structure
    XMStoreFloat4x4(&matrix, finalMatrix);
    
    // Extract frustum planes from the matrix (same as CPU frustum construction)
    
    // Near plane
    frustumData.frustumPlanes[0].x = matrix._13;
    frustumData.frustumPlanes[0].y = matrix._23;
    frustumData.frustumPlanes[0].z = matrix._33;
    frustumData.frustumPlanes[0].w = matrix._43;
    
    // Normalize near plane
    t = sqrt((frustumData.frustumPlanes[0].x * frustumData.frustumPlanes[0].x) + 
             (frustumData.frustumPlanes[0].y * frustumData.frustumPlanes[0].y) + 
             (frustumData.frustumPlanes[0].z * frustumData.frustumPlanes[0].z));
    frustumData.frustumPlanes[0].x /= t;
    frustumData.frustumPlanes[0].y /= t;
    frustumData.frustumPlanes[0].z /= t;
    frustumData.frustumPlanes[0].w /= t;
    
    // Far plane
    frustumData.frustumPlanes[1].x = matrix._14 - matrix._13;
    frustumData.frustumPlanes[1].y = matrix._24 - matrix._23;
    frustumData.frustumPlanes[1].z = matrix._34 - matrix._33;
    frustumData.frustumPlanes[1].w = matrix._44 - matrix._43;
    
    // Normalize far plane
    t = sqrt((frustumData.frustumPlanes[1].x * frustumData.frustumPlanes[1].x) + 
             (frustumData.frustumPlanes[1].y * frustumData.frustumPlanes[1].y) + 
             (frustumData.frustumPlanes[1].z * frustumData.frustumPlanes[1].z));
    frustumData.frustumPlanes[1].x /= t;
    frustumData.frustumPlanes[1].y /= t;
    frustumData.frustumPlanes[1].z /= t;
    frustumData.frustumPlanes[1].w /= t;
    
    // Left plane
    frustumData.frustumPlanes[2].x = matrix._14 + matrix._11;
    frustumData.frustumPlanes[2].y = matrix._24 + matrix._21;
    frustumData.frustumPlanes[2].z = matrix._34 + matrix._31;
    frustumData.frustumPlanes[2].w = matrix._44 + matrix._41;
    
    // Normalize left plane
    t = sqrt((frustumData.frustumPlanes[2].x * frustumData.frustumPlanes[2].x) + 
             (frustumData.frustumPlanes[2].y * frustumData.frustumPlanes[2].y) + 
             (frustumData.frustumPlanes[2].z * frustumData.frustumPlanes[2].z));
    frustumData.frustumPlanes[2].x /= t;
    frustumData.frustumPlanes[2].y /= t;
    frustumData.frustumPlanes[2].z /= t;
    frustumData.frustumPlanes[2].w /= t;
    
    // Right plane
    frustumData.frustumPlanes[3].x = matrix._14 - matrix._11;
    frustumData.frustumPlanes[3].y = matrix._24 - matrix._21;
    frustumData.frustumPlanes[3].z = matrix._34 - matrix._31;
    frustumData.frustumPlanes[3].w = matrix._44 - matrix._41;
    
    // Normalize right plane
    t = sqrt((frustumData.frustumPlanes[3].x * frustumData.frustumPlanes[3].x) + 
             (frustumData.frustumPlanes[3].y * frustumData.frustumPlanes[3].y) + 
             (frustumData.frustumPlanes[3].z * frustumData.frustumPlanes[3].z));
    frustumData.frustumPlanes[3].x /= t;
    frustumData.frustumPlanes[3].y /= t;
    frustumData.frustumPlanes[3].z /= t;
    frustumData.frustumPlanes[3].w /= t;
    
    // Top plane
    frustumData.frustumPlanes[4].x = matrix._14 - matrix._12;
    frustumData.frustumPlanes[4].y = matrix._24 - matrix._22;
    frustumData.frustumPlanes[4].z = matrix._34 - matrix._32;
    frustumData.frustumPlanes[4].w = matrix._44 - matrix._42;
    
    // Normalize top plane
    t = sqrt((frustumData.frustumPlanes[4].x * frustumData.frustumPlanes[4].x) + 
             (frustumData.frustumPlanes[4].y * frustumData.frustumPlanes[4].y) + 
             (frustumData.frustumPlanes[4].z * frustumData.frustumPlanes[4].z));
    frustumData.frustumPlanes[4].x /= t;
    frustumData.frustumPlanes[4].y /= t;
    frustumData.frustumPlanes[4].z /= t;
    frustumData.frustumPlanes[4].w /= t;
    
    // Bottom plane
    frustumData.frustumPlanes[5].x = matrix._14 + matrix._12;
    frustumData.frustumPlanes[5].y = matrix._24 + matrix._22;
    frustumData.frustumPlanes[5].z = matrix._34 + matrix._32;
    frustumData.frustumPlanes[5].w = matrix._44 + matrix._42;
    
    // Normalize bottom plane
    t = sqrt((frustumData.frustumPlanes[5].x * frustumData.frustumPlanes[5].x) + 
             (frustumData.frustumPlanes[5].y * frustumData.frustumPlanes[5].y) + 
             (frustumData.frustumPlanes[5].z * frustumData.frustumPlanes[5].z));
    frustumData.frustumPlanes[5].x /= t;
    frustumData.frustumPlanes[5].y /= t;
    frustumData.frustumPlanes[5].z /= t;
    frustumData.frustumPlanes[5].w /= t;
    

    
    return frustumData;
} 