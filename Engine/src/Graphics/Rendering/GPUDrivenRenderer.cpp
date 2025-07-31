#include "GPUDrivenRenderer.h"
#include "../../Core/System/Logger.h"
#include "../../Core/System/PerformanceProfiler.h"
#include "../../Core/Application/Application.h"
#include "../../Core/Common/EngineTypes.h"
#include <chrono>
#include "../Resource/Model.h"
#include "Light.h"
#include "../Rendering/Camera.h"
#include "../Shaders/PBRShader.h"
#include "../D3D11/D3D11Device.h"
#include <d3dcompiler.h>

GPUDrivenRenderer::GPUDrivenRenderer()
    : m_worldMatrixGenerationCS(nullptr)
    , m_frustumCullingCS(nullptr)
    , m_streamCompactionCS(nullptr)
    , m_updateDrawArgsCS(nullptr)
    , m_gpuDrivenVertexShader(nullptr)
    , m_gpuDrivenPixelShader(nullptr)
    , m_gpuDrivenInputLayout(nullptr)
    , m_visibilityBuffer(nullptr)
    , m_visibilitySRV(nullptr)
    , m_visibilityUAV(nullptr)
    , m_visibilityReadbackBuffer(nullptr)
    , m_drawArgumentsBuffer(nullptr)
    , m_drawArgumentsUAV(nullptr)
    , m_visibleObjectsBuffer(nullptr)
    , m_visibleObjectsSRV(nullptr)
    , m_visibleObjectsUAV(nullptr)
    , m_visibleCountBuffer(nullptr)
    , m_visibleCountUAV(nullptr)
    , m_visibleCountStagingBuffer(nullptr)
    , m_frustumConstantBuffer(nullptr)
    , m_objectCountBuffer(nullptr)
    , m_viewProjectionBuffer(nullptr)
    , m_lightBuffer(nullptr)
    , m_materialBuffer(nullptr)
    , m_enableGPUDriven(true)
    , m_maxObjects(0)
    , m_renderCount(0)
    , m_lastFrustumCullingTime(0)
    , m_cameraPosition(0.0f, 0.0f, 0.0f)
    , m_viewMatrix(XMMatrixIdentity())
    , m_projectionMatrix(XMMatrixIdentity())
    , m_prevViewMatrix(XMMatrixIdentity())
    , m_prevProjectionMatrix(XMMatrixIdentity())
    , m_prevCameraPosition(0.0f, 0.0f, 0.0f)
    , m_prevAmbientColor(0.0f, 0.0f, 0.0f, 0.0f)
    , m_prevDiffuseColor(0.0f, 0.0f, 0.0f, 0.0f)
    , m_prevLightDirection(0.0f, 0.0f, 0.0f)
    , m_prevBaseColor(0.0f, 0.0f, 0.0f, 0.0f)
    , m_prevMaterialProperties(0.0f, 0.0f, 0.0f, 0.0f)
    , m_constantBuffersInitialized(false)
{
    LOG("GPUDrivenRenderer: Constructor - GPU-driven renderer with frustum culling created");
    
    // Initialize frustum planes
    for (int i = 0; i < 6; ++i)
    {
        m_frustumPlanes[i] = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
    }
}

GPUDrivenRenderer::~GPUDrivenRenderer()
{
    Shutdown();
}

bool GPUDrivenRenderer::Initialize(ID3D11Device* device, HWND hwnd, UINT maxObjects)
{
    LOG("GPUDrivenRenderer: Initialize - Starting minimal GPU-driven renderer initialization");
    LOG("GPUDrivenRenderer: Initialize - Max objects: " + std::to_string(maxObjects));
    
    m_maxObjects = maxObjects;
    
    // Initialize indirect draw buffer (simplified)
    bool result = m_indirectBuffer.Initialize(device, maxObjects);
    if (!result)
    {
        LOG_ERROR("GPUDrivenRenderer: Failed to initialize indirect draw buffer");
        return false;
    }
    LOG("GPUDrivenRenderer: Indirect draw buffer initialized successfully");
    
    // Initialize compute shaders (only world matrix generation)
    result = InitializeComputeShaders(device, hwnd);
    if (!result)
    {
        LOG_ERROR("GPUDrivenRenderer: Failed to initialize compute shaders");
        return false;
    }
    LOG("GPUDrivenRenderer: Compute shaders initialized successfully");
    
    // Initialize GPU-driven rendering shaders
    result = InitializeGPUDrivenShaders(device, hwnd);
    if (!result)
    {
        LOG_ERROR("GPUDrivenRenderer: Failed to initialize GPU-driven rendering shaders");
        return false;
    }
    LOG("GPUDrivenRenderer: GPU-driven rendering shaders initialized successfully");
    
    // Initialize visibility buffer for frustum culling
    result = InitializeVisibilityBuffer(device, maxObjects);
    if (!result)
    {
        LOG_ERROR("GPUDrivenRenderer: Failed to initialize visibility buffer");
        return false;
    }
    LOG("GPUDrivenRenderer: Visibility buffer initialized successfully");
    
    // Initialize reusable constant buffers for performance
    result = InitializeConstantBuffers(device);
    if (!result)
    {
        LOG_ERROR("GPUDrivenRenderer: Failed to initialize constant buffers");
        return false;
    }
    LOG("GPUDrivenRenderer: Constant buffers initialized successfully");
    
    // TRUE GPU-DRIVEN: Initialize indirect draw and stream compaction buffers
    result = InitializeIndirectDrawBuffers(device, maxObjects);
    if (!result)
    {
        LOG_ERROR("GPUDrivenRenderer: Failed to initialize indirect draw buffers");
        return false;
    }
    LOG("GPUDrivenRenderer: Indirect draw buffers initialized successfully");
    
    LOG("GPUDrivenRenderer: TRUE GPU-driven renderer with indirect drawing initialized with " + std::to_string(maxObjects) + " max objects");
    return true;
}

void GPUDrivenRenderer::Shutdown()
{
    LOG("GPUDrivenRenderer: Shutdown - Releasing resources");
    ReleaseComputeShaders();
    ReleaseGPUDrivenShaders();
    ReleaseVisibilityBuffer();
    ReleaseIndirectDrawBuffers();
    ReleaseConstantBuffers();
    m_indirectBuffer.Shutdown();
    LOG("GPUDrivenRenderer: Shutdown completed");
}

bool GPUDrivenRenderer::InitializeComputeShaders(ID3D11Device* device, HWND hwnd)
{
    LOG("GPUDrivenRenderer: InitializeComputeShaders - Starting compute shader initialization");
    
    // Create world matrix generation compute shader
    m_worldMatrixGenerationCS = new ComputeShader();
    
    LOG("GPUDrivenRenderer: Initializing world matrix generation compute shader");
    bool result = m_worldMatrixGenerationCS->Initialize(device, hwnd, L"../Engine/assets/shaders/WorldMatrixGenerationComputeShader.hlsl", "main");
    if (!result)
    {
        LOG_ERROR("GPUDrivenRenderer: Failed to initialize world matrix generation compute shader");
        return false;
    }
    LOG("GPUDrivenRenderer: World matrix generation compute shader initialized successfully");
    
    // Verify world matrix compute shader is valid
    if (!m_worldMatrixGenerationCS->GetComputeShader())
    {
        LOG_ERROR("GPUDrivenRenderer: World matrix generation compute shader is null after initialization");
        return false;
    }
    
    // Create frustum culling compute shader
    m_frustumCullingCS = new ComputeShader();
    
    LOG("GPUDrivenRenderer: Initializing frustum culling compute shader");
    result = m_frustumCullingCS->Initialize(device, hwnd, L"../Engine/assets/shaders/FrustumCullingComputeShader.hlsl", "main");
    if (!result)
    {
        LOG_ERROR("GPUDrivenRenderer: Failed to initialize frustum culling compute shader");
        return false;
    }
    LOG("GPUDrivenRenderer: Frustum culling compute shader initialized successfully");
    
    // Verify frustum culling compute shader is valid
    if (!m_frustumCullingCS->GetComputeShader())
    {
        LOG_ERROR("GPUDrivenRenderer: Frustum culling compute shader is null after initialization");
        return false;
    }
    
    // Try to create stream compaction compute shader (optional for now)
    m_streamCompactionCS = new ComputeShader();
    
    LOG("GPUDrivenRenderer: Initializing stream compaction compute shader");
    result = m_streamCompactionCS->Initialize(device, hwnd, L"../Engine/assets/shaders/StreamCompactionComputeShader.hlsl", "main");
    if (!result)
    {
        LOG_ERROR("GPUDrivenRenderer: Failed to initialize stream compaction compute shader - WILL USE FALLBACK");
        delete m_streamCompactionCS;
        m_streamCompactionCS = nullptr;
    }
    else
    {
        LOG("GPUDrivenRenderer: Stream compaction compute shader initialized successfully");
        
        // Verify stream compaction compute shader is valid
        if (!m_streamCompactionCS->GetComputeShader())
        {
            LOG_ERROR("GPUDrivenRenderer: Stream compaction compute shader is null after initialization - WILL USE FALLBACK");
            delete m_streamCompactionCS;
            m_streamCompactionCS = nullptr;
        }
    }
    
    // Try to create update draw arguments compute shader (optional for now)
    m_updateDrawArgsCS = new ComputeShader();
    
    LOG("GPUDrivenRenderer: Initializing update draw arguments compute shader");
    result = m_updateDrawArgsCS->Initialize(device, hwnd, L"../Engine/assets/shaders/UpdateDrawArgsComputeShader.hlsl", "main");
    if (!result)
    {
        LOG_ERROR("GPUDrivenRenderer: Failed to initialize update draw arguments compute shader - WILL USE FALLBACK");
        delete m_updateDrawArgsCS;
        m_updateDrawArgsCS = nullptr;
    }
    else
    {
        LOG("GPUDrivenRenderer: Update draw arguments compute shader initialized successfully");
        
        // Verify update draw arguments compute shader is valid
        if (!m_updateDrawArgsCS->GetComputeShader())
        {
            LOG_ERROR("GPUDrivenRenderer: Update draw arguments compute shader is null after initialization - WILL USE FALLBACK");
            delete m_updateDrawArgsCS;
            m_updateDrawArgsCS = nullptr;
        }
    }
    
    LOG("GPUDrivenRenderer: All compute shaders initialized and verified successfully");
    return true;
}

void GPUDrivenRenderer::ReleaseComputeShaders()
{
    if (m_worldMatrixGenerationCS)
    {
        m_worldMatrixGenerationCS->Shutdown();
        delete m_worldMatrixGenerationCS;
        m_worldMatrixGenerationCS = nullptr;
    }
    
    if (m_frustumCullingCS)
    {
        m_frustumCullingCS->Shutdown();
        delete m_frustumCullingCS;
        m_frustumCullingCS = nullptr;
    }
    
    if (m_streamCompactionCS)
    {
        m_streamCompactionCS->Shutdown();
        delete m_streamCompactionCS;
        m_streamCompactionCS = nullptr;
    }
    
    if (m_updateDrawArgsCS)
    {
        m_updateDrawArgsCS->Shutdown();
        delete m_updateDrawArgsCS;
        m_updateDrawArgsCS = nullptr;
    }
}

void GPUDrivenRenderer::UpdateObjects(ID3D11DeviceContext* context, const std::vector<ObjectData>& objects)
{
    if (objects.empty())
    {
        return;
    }
    
    // Remove frequent logging - was causing FPS drops
    m_indirectBuffer.UpdateObjectData(context, objects);
}

void GPUDrivenRenderer::UpdateCamera(ID3D11DeviceContext* context, const XMFLOAT3& cameraPos, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix)
{
    // Remove frequent logging - was causing FPS drops
    m_cameraPosition = cameraPos;
    m_viewMatrix = viewMatrix;
    m_projectionMatrix = projectionMatrix;
    
    // Extract frustum planes for GPU frustum culling
    XMMATRIX viewProjectionMatrix = XMMatrixMultiply(viewMatrix, projectionMatrix);
    ExtractFrustumPlanes(viewProjectionMatrix);
}

void GPUDrivenRenderer::Render(ID3D11DeviceContext* context, ID3D11Buffer* vertexBuffer, ID3D11Buffer* indexBuffer,
                              Model* model, PBRShader* pbrShader, Light* light, Camera* camera, D3D11Device* direct3D)
{
    // TRUE GPU-DRIVEN RENDERING PIPELINE:
    // 1. GPU frustum culling → visibility buffer
    // 2. GPU stream compaction → visible objects array + count
    // 3. GPU updates draw arguments with visible count
    // 4. DrawIndexedInstancedIndirect renders ONLY visible objects
    // ZERO CPU-GPU sync, maximum performance scaling
    
    if (!m_enableGPUDriven)
    {
        return;
    }
    
    // Validate required resources
    if (!context || !vertexBuffer || !indexBuffer || !model)
    {
        return;
    }
    
    // Get object count
    UINT objectCount = m_indirectBuffer.GetObjectCount();
    if (objectCount == 0)
    {
        return;
    }
    
    // Get required buffers
    ID3D11ShaderResourceView* objectDataSRV = m_indirectBuffer.GetObjectDataSRV();
    ID3D11UnorderedAccessView* worldMatrixUAV = m_indirectBuffer.GetWorldMatrixUAV();
    
    if (!objectDataSRV || !worldMatrixUAV)
    {
        return;
    }
    
    // Start GPU culling timing
    auto gpuCullingStart = std::chrono::high_resolution_clock::now();
    
    // STEP 1: Generate world matrices using compute shader (run every frame)
    D3D11_MAPPED_SUBRESOURCE objectCountMappedResource;
    HRESULT result = context->Map(m_objectCountBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &objectCountMappedResource);
    if (SUCCEEDED(result))
    {
        UINT* objectCountData = static_cast<UINT*>(objectCountMappedResource.pData);
        objectCountData[0] = objectCount;
        objectCountData[1] = 0;
        objectCountData[2] = 0;
        objectCountData[3] = 0;
        context->Unmap(m_objectCountBuffer, 0);
        
        // Generate world matrices
        m_worldMatrixGenerationCS->SetShaderResourceView(context, 0, objectDataSRV);
        m_worldMatrixGenerationCS->SetUnorderedAccessView(context, 0, worldMatrixUAV);
        m_worldMatrixGenerationCS->SetConstantBuffer(context, 0, m_objectCountBuffer);
        
        UINT threadGroupCount = (objectCount + 63) / 64;
        m_worldMatrixGenerationCS->Dispatch(context, threadGroupCount, 1, 1);
        PerformanceProfiler::GetInstance().IncrementComputeDispatches();
        
        m_worldMatrixGenerationCS->SetUnorderedAccessView(context, 0, nullptr);
    }
    else
    {
        return;
    }
    
    // STEP 2: Perform GPU frustum culling (run every frame for accuracy)
    struct FrustumBuffer
    {
        XMFLOAT4 frustumPlanes[6];
        UINT objectCount;
        UINT padding[3];
    };
    
    D3D11_MAPPED_SUBRESOURCE frustumMappedResource;
    result = context->Map(m_frustumConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &frustumMappedResource);
    if (SUCCEEDED(result))
    {
        FrustumBuffer* frustumData = static_cast<FrustumBuffer*>(frustumMappedResource.pData);
        for (int i = 0; i < 6; ++i)
        {
            frustumData->frustumPlanes[i] = m_frustumPlanes[i];
        }
        frustumData->objectCount = objectCount;
        frustumData->padding[0] = 0;
        frustumData->padding[1] = 0;
        frustumData->padding[2] = 0;
        context->Unmap(m_frustumConstantBuffer, 0);
        
        // Run frustum culling
        m_frustumCullingCS->SetShaderResourceView(context, 0, objectDataSRV);
        m_frustumCullingCS->SetUnorderedAccessView(context, 0, m_visibilityUAV);
        m_frustumCullingCS->SetConstantBuffer(context, 0, m_frustumConstantBuffer);
        
        UINT frustumThreadGroupCount = (objectCount + 63) / 64;
        m_frustumCullingCS->Dispatch(context, frustumThreadGroupCount, 1, 1);
        PerformanceProfiler::GetInstance().IncrementComputeDispatches();
        
        m_frustumCullingCS->SetUnorderedAccessView(context, 0, nullptr);
    }
    else
    {
        return;
    }
    
    // Check if we have stream compaction and indirect draw support
    bool hasStreamCompaction = (m_streamCompactionCS && m_updateDrawArgsCS && m_visibleCountUAV && m_drawArgumentsUAV);
    
    if (hasStreamCompaction)
    {
        // STEP 3: Clear visible count buffer for stream compaction
        UINT clearValue[4] = { 0, 0, 0, 0 };
        context->ClearUnorderedAccessViewUint(m_visibleCountUAV, clearValue);
        
        // STEP 4: Stream compaction - create dense array of visible objects
        m_streamCompactionCS->SetShaderResourceView(context, 0, m_visibilitySRV);
        m_streamCompactionCS->SetUnorderedAccessView(context, 0, m_visibleObjectsUAV);
        m_streamCompactionCS->SetUnorderedAccessView(context, 1, m_visibleCountUAV);
        m_streamCompactionCS->SetConstantBuffer(context, 0, m_objectCountBuffer);
        
        UINT compactionThreadGroupCount = (objectCount + 63) / 64;
        m_streamCompactionCS->Dispatch(context, compactionThreadGroupCount, 1, 1);
        PerformanceProfiler::GetInstance().IncrementComputeDispatches();
        
        // Unbind UAVs
        m_streamCompactionCS->SetUnorderedAccessView(context, 0, nullptr);
        m_streamCompactionCS->SetUnorderedAccessView(context, 1, nullptr);
        
        // STEP 5: Update draw arguments based on visible count
        struct DrawInfoBuffer
        {
            UINT indexCountPerInstance;
            UINT startIndexLocation;
            INT baseVertexLocation;
            UINT startInstanceLocation;
        };
        
        // Create a temporary constant buffer for draw info
        static ID3D11Buffer* drawInfoBuffer = nullptr;
        if (!drawInfoBuffer)
        {
            D3D11_BUFFER_DESC bufferDesc = {};
            bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
            bufferDesc.ByteWidth = sizeof(DrawInfoBuffer);
            bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            bufferDesc.MiscFlags = 0;
            
            direct3D->GetDevice()->CreateBuffer(&bufferDesc, nullptr, &drawInfoBuffer);
        }
        
        if (drawInfoBuffer)
        {
            D3D11_MAPPED_SUBRESOURCE drawInfoMappedResource;
            result = context->Map(drawInfoBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &drawInfoMappedResource);
            if (SUCCEEDED(result))
            {
                DrawInfoBuffer* drawInfo = static_cast<DrawInfoBuffer*>(drawInfoMappedResource.pData);
                drawInfo->indexCountPerInstance = model->GetIndexCount();
                drawInfo->startIndexLocation = 0;
                drawInfo->baseVertexLocation = 0;
                drawInfo->startInstanceLocation = 0;
                context->Unmap(drawInfoBuffer, 0);
                
                // Update draw arguments
                ID3D11ShaderResourceView* visibleCountSRV = nullptr;
                D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                srvDesc.Format = DXGI_FORMAT_UNKNOWN;
                srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
                srvDesc.Buffer.FirstElement = 0;
                srvDesc.Buffer.NumElements = 4;
                
                HRESULT hr = direct3D->GetDevice()->CreateShaderResourceView(m_visibleCountBuffer, &srvDesc, &visibleCountSRV);
                
                if (SUCCEEDED(hr) && visibleCountSRV)
                {
                    m_updateDrawArgsCS->SetShaderResourceView(context, 0, visibleCountSRV);
                    m_updateDrawArgsCS->SetUnorderedAccessView(context, 0, m_drawArgumentsUAV);
                    m_updateDrawArgsCS->SetConstantBuffer(context, 0, drawInfoBuffer);
                    
                    m_updateDrawArgsCS->Dispatch(context, 1, 1, 1); // Single thread
                    PerformanceProfiler::GetInstance().IncrementComputeDispatches();
                    
                    m_updateDrawArgsCS->SetUnorderedAccessView(context, 0, nullptr);
                    visibleCountSRV->Release();
                }
            }
        }
    }
    
    // End GPU culling timing
    auto gpuCullingEnd = std::chrono::high_resolution_clock::now();
    auto gpuCullingDuration = std::chrono::duration_cast<std::chrono::microseconds>(gpuCullingEnd - gpuCullingStart);
    m_lastFrustumCullingTime = gpuCullingDuration.count();
    
    // STEP 6: Set up rendering pipeline
    context->IASetInputLayout(m_gpuDrivenInputLayout);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    UINT stride = sizeof(EngineTypes::VertexType);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
    
    context->VSSetShader(m_gpuDrivenVertexShader, nullptr, 0);
    context->PSSetShader(m_gpuDrivenPixelShader, nullptr, 0);
    
    // Bind buffers to vertex shader
    ID3D11ShaderResourceView* worldMatrixSRV = m_indirectBuffer.GetWorldMatrixSRV();
    if (worldMatrixSRV)
    {
        context->VSSetShaderResources(1, 1, &worldMatrixSRV);
    }
    
    // Decide which approach to use based on available features
    bool useIndirectDraw = (m_streamCompactionCS && m_updateDrawArgsCS && m_visibleObjectsSRV && m_drawArgumentsBuffer);
    
    if (useIndirectDraw)
    {
        // Bind compacted visible objects array to vertex shader (register t2)
        context->VSSetShaderResources(2, 1, &m_visibleObjectsSRV);
    }
    else
    {
        // FALLBACK: Bind visibility buffer directly to vertex shader (register t2)
        if (m_visibilitySRV)
        {
            context->VSSetShaderResources(2, 1, &m_visibilitySRV);
        }
    }
    
    // PERFORMANCE OPTIMIZATION: Only update view/projection buffer when data changes
    // Compare matrices using memcmp for performance
    if (!m_constantBuffersInitialized || 
        memcmp(&m_viewMatrix, &m_prevViewMatrix, sizeof(XMMATRIX)) != 0 || 
        memcmp(&m_projectionMatrix, &m_prevProjectionMatrix, sizeof(XMMATRIX)) != 0)
    {
        struct ViewProjectionBuffer
        {
            XMMATRIX viewMatrix;
            XMMATRIX projectionMatrix;
        };
        
        D3D11_MAPPED_SUBRESOURCE vpMappedResource;
        result = context->Map(m_viewProjectionBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &vpMappedResource);
        if (SUCCEEDED(result))
        {
            ViewProjectionBuffer* vpBuffer = static_cast<ViewProjectionBuffer*>(vpMappedResource.pData);
            vpBuffer->viewMatrix = XMMatrixTranspose(m_viewMatrix);
            vpBuffer->projectionMatrix = XMMatrixTranspose(m_projectionMatrix);
            context->Unmap(m_viewProjectionBuffer, 0);
            
            // Cache current values
            m_prevViewMatrix = m_viewMatrix;
            m_prevProjectionMatrix = m_projectionMatrix;
        }
    }
    context->VSSetConstantBuffers(0, 1, &m_viewProjectionBuffer);
    
    // PERFORMANCE OPTIMIZATION: Only update lighting buffer when data changes
    XMFLOAT4 currentAmbientColor = light ? light->GetAmbientColor() : XMFLOAT4(0.15f, 0.15f, 0.15f, 1.0f);
    XMFLOAT4 currentDiffuseColor = light ? light->GetDiffuseColor() : XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    XMFLOAT3 currentLightDirection = light ? light->GetDirection() : XMFLOAT3(0.0f, 0.0f, 1.0f);
    
    if (!m_constantBuffersInitialized ||
        memcmp(&currentAmbientColor, &m_prevAmbientColor, sizeof(XMFLOAT4)) != 0 ||
        memcmp(&currentDiffuseColor, &m_prevDiffuseColor, sizeof(XMFLOAT4)) != 0 ||
        memcmp(&currentLightDirection, &m_prevLightDirection, sizeof(XMFLOAT3)) != 0 ||
        memcmp(&m_cameraPosition, &m_prevCameraPosition, sizeof(XMFLOAT3)) != 0)
    {
        struct LightBuffer
        {
            XMFLOAT4 ambientColor;
            XMFLOAT4 diffuseColor;
            XMFLOAT3 lightDirection;
            float padding1;
            XMFLOAT3 cameraPosition;
            float padding2;
        };
        
        D3D11_MAPPED_SUBRESOURCE lightMappedResource;
        result = context->Map(m_lightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &lightMappedResource);
        if (SUCCEEDED(result))
        {
            LightBuffer* lightBuffer = static_cast<LightBuffer*>(lightMappedResource.pData);
            lightBuffer->ambientColor = currentAmbientColor;
            lightBuffer->diffuseColor = currentDiffuseColor;
            lightBuffer->lightDirection = currentLightDirection;
            lightBuffer->padding1 = 0.0f;
            lightBuffer->cameraPosition = m_cameraPosition;
            lightBuffer->padding2 = 0.0f;
            context->Unmap(m_lightBuffer, 0);
            
            // Cache current values
            m_prevAmbientColor = currentAmbientColor;
            m_prevDiffuseColor = currentDiffuseColor;
            m_prevLightDirection = currentLightDirection;
            m_prevCameraPosition = m_cameraPosition;
        }
    }
    context->PSSetConstantBuffers(0, 1, &m_lightBuffer);
    
    // PERFORMANCE OPTIMIZATION: Only update material buffer when data changes
    XMFLOAT4 currentBaseColor = model ? model->GetBaseColor() : XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
    XMFLOAT4 currentMaterialProperties = model ? XMFLOAT4(
        model->GetMetallic(),      // x = metallic
        model->GetRoughness(),     // y = roughness
        model->GetAO(),            // z = ao
        model->GetEmissionStrength() // w = emissionStrength
    ) : XMFLOAT4(0.0f, 0.5f, 1.0f, 0.0f);
    
    if (!m_constantBuffersInitialized ||
        memcmp(&currentBaseColor, &m_prevBaseColor, sizeof(XMFLOAT4)) != 0 ||
        memcmp(&currentMaterialProperties, &m_prevMaterialProperties, sizeof(XMFLOAT4)) != 0)
    {
        struct MaterialBuffer
        {
            XMFLOAT4 baseColor;
            XMFLOAT4 materialProperties; // x=metallic, y=roughness, z=ao, w=emissionStrength
            XMFLOAT4 materialPadding;
        };
        
        D3D11_MAPPED_SUBRESOURCE materialMappedResource;
        result = context->Map(m_materialBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &materialMappedResource);
        if (SUCCEEDED(result))
        {
            MaterialBuffer* materialBuffer = static_cast<MaterialBuffer*>(materialMappedResource.pData);
            materialBuffer->baseColor = currentBaseColor;
            materialBuffer->materialProperties = currentMaterialProperties;
            materialBuffer->materialPadding = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
            context->Unmap(m_materialBuffer, 0);
            
            // Cache current values
            m_prevBaseColor = currentBaseColor;
            m_prevMaterialProperties = currentMaterialProperties;
        }
    }
    context->PSSetConstantBuffers(1, 1, &m_materialBuffer);
    
    // Mark constant buffers as initialized after first update
    if (!m_constantBuffersInitialized)
    {
        m_constantBuffersInitialized = true;
    }
    
    // Bind model textures
    if (model)
    {
        ID3D11ShaderResourceView* textures[6] = { 
            model->GetDiffuseTexture(),   // Diffuse/Albedo
            model->GetNormalTexture(),    // Normal Map
            model->GetMetallicTexture(),  // Metallic
            model->GetRoughnessTexture(), // Roughness
            model->GetEmissionTexture(),  // Emission
            model->GetAOTexture()         // Ambient Occlusion
        };
        context->PSSetShaderResources(0, 6, textures);
    }
    
    // STEP 7: RENDERING - Choose approach based on available features
    int indexCount = model->GetIndexCount();
    static int lastKnownVisibleCount = objectCount;
    
    if (useIndirectDraw)
    {
        // TRUE GPU-DRIVEN RENDERING - DrawIndexedInstancedIndirect
        // GPU determines instance count, CPU never knows how many objects are visible
        // Maximum performance: only visible objects processed, zero CPU-GPU sync
        context->DrawIndexedInstancedIndirect(m_drawArgumentsBuffer, 0);
        
        // Track performance metrics - note: we don't know exact visible count on CPU
        PerformanceProfiler::GetInstance().IncrementDrawCalls();
        PerformanceProfiler::GetInstance().IncrementIndirectDrawCalls(); // Track indirect draw calls separately
        
        // For UI display, read back visible count occasionally (blocking for accuracy)
        static int framesSinceReadback = 0;
        
        if (++framesSinceReadback >= 30) // Read back every 30 frames for UI display
        {
            framesSinceReadback = 0;
            
            // Readback visible count for UI display only
            if (m_visibleCountStagingBuffer && m_visibleCountBuffer)
            {
                context->CopyResource(m_visibleCountStagingBuffer, m_visibleCountBuffer);
                
                D3D11_MAPPED_SUBRESOURCE mappedResource;
                HRESULT hr = context->Map(m_visibleCountStagingBuffer, 0, D3D11_MAP_READ, 0, &mappedResource); // Blocking read for UI
                if (SUCCEEDED(hr))
                {
                    UINT* visibleCountData = static_cast<UINT*>(mappedResource.pData);
                    int newVisibleCount = static_cast<int>(visibleCountData[0]);
                    context->Unmap(m_visibleCountStagingBuffer, 0);
                    
                    // Update the count
                    lastKnownVisibleCount = newVisibleCount;
                }
            }
        }
        
        // Store visible count for UI display
        m_renderCount = lastKnownVisibleCount;
        
        // Track performance metrics based on actual visible count
        int trianglesRendered = (indexCount / 3) * lastKnownVisibleCount;
        PerformanceProfiler::GetInstance().AddTriangles(trianglesRendered);
        PerformanceProfiler::GetInstance().AddInstances(lastKnownVisibleCount);
    }
    else
    {
        // FALLBACK APPROACH: Draw all instances, let vertex shader handle visibility with the existing approach
        context->DrawIndexedInstanced(indexCount, objectCount, 0, 0, 0);
        
        // Track performance metrics
        PerformanceProfiler::GetInstance().IncrementDrawCalls();
        PerformanceProfiler::GetInstance().AddTriangles((indexCount / 3) * objectCount); // Vertex shader will cull
        PerformanceProfiler::GetInstance().AddInstances(objectCount);
        
        // Store count for UI display
        m_renderCount = static_cast<int>(objectCount);
        lastKnownVisibleCount = objectCount; // Fallback doesn't track actual visible count
    }
    
    // Update PerformanceProfiler with GPU frustum culling data
    PerformanceProfiler::GetInstance().SetGPUFrustumCullingTime(static_cast<double>(gpuCullingDuration.count()));
    PerformanceProfiler::GetInstance().SetFrustumCullingObjects(objectCount, lastKnownVisibleCount);
    
    // Note: GPU utilization is now calculated automatically in PerformanceProfiler::CalculateEfficiencyMetrics()
}

bool GPUDrivenRenderer::InitializeGPUDrivenShaders(ID3D11Device* device, HWND hwnd)
{
    HRESULT result;
    ID3D10Blob* errorMessage = nullptr;
    ID3D10Blob* vertexShaderBuffer = nullptr;
    ID3D10Blob* pixelShaderBuffer = nullptr;
    
    LOG("GPUDrivenRenderer: InitializeGPUDrivenShaders - Starting GPU-driven shader initialization");
    
    // Compile vertex shader
    result = D3DCompileFromFile(L"../Engine/assets/shaders/GPUDrivenPBRVertexShader.hlsl", NULL, NULL, 
                               "GPUDrivenPBRVertexShader", "vs_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0,
                               &vertexShaderBuffer, &errorMessage);
    if (FAILED(result))
    {
        if (errorMessage)
        {
            char* compileErrors = (char*)(errorMessage->GetBufferPointer());
            LOG_ERROR("GPU-driven vertex shader compilation failed: " + std::string(compileErrors));
            errorMessage->Release();
        }
        else
        {
            LOG_ERROR("GPU-driven vertex shader compilation failed - HRESULT: " + std::to_string(result));
        }
        return false;
    }
    LOG("GPU-driven vertex shader compiled successfully");
    
    // Compile pixel shader (use regular PBR pixel shader)
    result = D3DCompileFromFile(L"../Engine/assets/shaders/PBRPixelShader.hlsl", NULL, NULL, 
                               "PBRPixelShader", "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0,
                               &pixelShaderBuffer, &errorMessage);
    if (FAILED(result))
    {
        if (errorMessage)
        {
            char* compileErrors = (char*)(errorMessage->GetBufferPointer());
            LOG_ERROR("PBR pixel shader compilation failed: " + std::string(compileErrors));
            errorMessage->Release();
        }
        else
        {
            LOG_ERROR("PBR pixel shader compilation failed - HRESULT: " + std::to_string(result));
        }
        if (vertexShaderBuffer) vertexShaderBuffer->Release();
        return false;
    }
    LOG("PBR pixel shader compiled successfully");
    
    // Create vertex shader
    result = device->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), 
                                       vertexShaderBuffer->GetBufferSize(), NULL, 
                                       &m_gpuDrivenVertexShader);
    if (FAILED(result))
    {
        LOG_ERROR("Failed to create GPU-driven vertex shader");
        vertexShaderBuffer->Release();
        pixelShaderBuffer->Release();
        return false;
    }
    
    // Create pixel shader
    result = device->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), 
                                      pixelShaderBuffer->GetBufferSize(), NULL, 
                                      &m_gpuDrivenPixelShader);
    if (FAILED(result))
    {
        LOG_ERROR("Failed to create GPU-driven pixel shader");
        vertexShaderBuffer->Release();
        pixelShaderBuffer->Release();
        return false;
    }
    
    // Create input layout for PBR rendering
    D3D11_INPUT_ELEMENT_DESC polygonLayout[5];
    
    polygonLayout[0].SemanticName = "POSITION";
    polygonLayout[0].SemanticIndex = 0;
    polygonLayout[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    polygonLayout[0].InputSlot = 0;
    polygonLayout[0].AlignedByteOffset = 0;
    polygonLayout[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    polygonLayout[0].InstanceDataStepRate = 0;

    polygonLayout[1].SemanticName = "TEXCOORD";
    polygonLayout[1].SemanticIndex = 0;
    polygonLayout[1].Format = DXGI_FORMAT_R32G32_FLOAT;
    polygonLayout[1].InputSlot = 0;
    polygonLayout[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
    polygonLayout[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    polygonLayout[1].InstanceDataStepRate = 0;

    polygonLayout[2].SemanticName = "NORMAL";
    polygonLayout[2].SemanticIndex = 0;
    polygonLayout[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    polygonLayout[2].InputSlot = 0;
    polygonLayout[2].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
    polygonLayout[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    polygonLayout[2].InstanceDataStepRate = 0;
    
    polygonLayout[3].SemanticName = "TANGENT";
    polygonLayout[3].SemanticIndex = 0;
    polygonLayout[3].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    polygonLayout[3].InputSlot = 0;
    polygonLayout[3].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
    polygonLayout[3].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    polygonLayout[3].InstanceDataStepRate = 0;

    polygonLayout[4].SemanticName = "BINORMAL";
    polygonLayout[4].SemanticIndex = 0;
    polygonLayout[4].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    polygonLayout[4].InputSlot = 0;
    polygonLayout[4].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
    polygonLayout[4].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    polygonLayout[4].InstanceDataStepRate = 0;

    // Create the vertex input layout
    unsigned int numElements = sizeof(polygonLayout) / sizeof(polygonLayout[0]);
    result = device->CreateInputLayout(polygonLayout, numElements, vertexShaderBuffer->GetBufferPointer(), 
                                      vertexShaderBuffer->GetBufferSize(), &m_gpuDrivenInputLayout);
    if (FAILED(result))
    {
        LOG_ERROR("Failed to create GPU-driven input layout");
        vertexShaderBuffer->Release();
        pixelShaderBuffer->Release();
        return false;
    }

    // Release shader buffers
    vertexShaderBuffer->Release();
    pixelShaderBuffer->Release();
    
    LOG("GPU-driven shaders initialized successfully");
    return true;
}

void GPUDrivenRenderer::ReleaseGPUDrivenShaders()
{
    if (m_gpuDrivenInputLayout) { m_gpuDrivenInputLayout->Release(); m_gpuDrivenInputLayout = nullptr; }
    if (m_gpuDrivenPixelShader) { m_gpuDrivenPixelShader->Release(); m_gpuDrivenPixelShader = nullptr; }
    if (m_gpuDrivenVertexShader) { m_gpuDrivenVertexShader->Release(); m_gpuDrivenVertexShader = nullptr; }
}

bool GPUDrivenRenderer::InitializeVisibilityBuffer(ID3D11Device* device, UINT maxObjects)
{
    LOG("GPUDrivenRenderer: InitializeVisibilityBuffer - Creating visibility buffer for " + std::to_string(maxObjects) + " objects");
    
    // Create visibility buffer (one uint per object: 1 = visible, 0 = culled)
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth = sizeof(UINT) * maxObjects;
    bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
    bufferDesc.CPUAccessFlags = 0;
    bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    bufferDesc.StructureByteStride = sizeof(UINT);
    
    HRESULT result = device->CreateBuffer(&bufferDesc, nullptr, &m_visibilityBuffer);
    if (FAILED(result))
    {
        LOG_ERROR("GPUDrivenRenderer: Failed to create visibility buffer - HRESULT: " + std::to_string(result));
        return false;
    }
    LOG("GPUDrivenRenderer: Visibility buffer created successfully");
    
    // Create Shader Resource View
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = maxObjects;
    
    result = device->CreateShaderResourceView(m_visibilityBuffer, &srvDesc, &m_visibilitySRV);
    if (FAILED(result))
    {
        LOG_ERROR("GPUDrivenRenderer: Failed to create visibility SRV - HRESULT: " + std::to_string(result));
        return false;
    }
    LOG("GPUDrivenRenderer: Visibility SRV created successfully");
    
    // Create Unordered Access View
    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = DXGI_FORMAT_UNKNOWN;
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.NumElements = maxObjects;
    uavDesc.Buffer.Flags = 0;
    
    result = device->CreateUnorderedAccessView(m_visibilityBuffer, &uavDesc, &m_visibilityUAV);
    if (FAILED(result))
    {
        LOG_ERROR("GPUDrivenRenderer: Failed to create visibility UAV - HRESULT: " + std::to_string(result));
        return false;
    }
    LOG("GPUDrivenRenderer: Visibility UAV created successfully");
    
    // Create readback buffer for CPU access to visibility results
    D3D11_BUFFER_DESC readbackBufferDesc = {};
    readbackBufferDesc.Usage = D3D11_USAGE_STAGING;
    readbackBufferDesc.ByteWidth = sizeof(UINT) * maxObjects;
    readbackBufferDesc.BindFlags = 0;
    readbackBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    readbackBufferDesc.MiscFlags = 0;
    readbackBufferDesc.StructureByteStride = 0;
    
    result = device->CreateBuffer(&readbackBufferDesc, nullptr, &m_visibilityReadbackBuffer);
    if (FAILED(result))
    {
        LOG_ERROR("GPUDrivenRenderer: Failed to create visibility readback buffer - HRESULT: " + std::to_string(result));
        return false;
    }
    LOG("GPUDrivenRenderer: Visibility readback buffer created successfully");
    
    return true;
}

void GPUDrivenRenderer::ReleaseVisibilityBuffer()
{
    if (m_visibilityUAV) { m_visibilityUAV->Release(); m_visibilityUAV = nullptr; }
    if (m_visibilitySRV) { m_visibilitySRV->Release(); m_visibilitySRV = nullptr; }
    if (m_visibilityBuffer) { m_visibilityBuffer->Release(); m_visibilityBuffer = nullptr; }
    if (m_visibilityReadbackBuffer) { m_visibilityReadbackBuffer->Release(); m_visibilityReadbackBuffer = nullptr; }
}

bool GPUDrivenRenderer::InitializeConstantBuffers(ID3D11Device* device)
{
    HRESULT result;
    D3D11_BUFFER_DESC bufferDesc = {};
    
    LOG("GPUDrivenRenderer: InitializeConstantBuffers - Creating reusable constant buffers for performance");
    
    // Create frustum constant buffer
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.ByteWidth = sizeof(XMFLOAT4) * 6 + sizeof(UINT) * 4; // 6 planes + object count + padding
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bufferDesc.MiscFlags = 0;
    
    result = device->CreateBuffer(&bufferDesc, nullptr, &m_frustumConstantBuffer);
    if (FAILED(result))
    {
        LOG_ERROR("GPUDrivenRenderer: Failed to create frustum constant buffer");
        return false;
    }
    
    // Create object count constant buffer
    bufferDesc.ByteWidth = sizeof(UINT) * 4; // 16-byte aligned
    result = device->CreateBuffer(&bufferDesc, nullptr, &m_objectCountBuffer);
    if (FAILED(result))
    {
        LOG_ERROR("GPUDrivenRenderer: Failed to create object count constant buffer");
        return false;
    }
    
    // Create view/projection constant buffer
    bufferDesc.ByteWidth = sizeof(XMMATRIX) * 2; // view + projection matrices
    result = device->CreateBuffer(&bufferDesc, nullptr, &m_viewProjectionBuffer);
    if (FAILED(result))
    {
        LOG_ERROR("GPUDrivenRenderer: Failed to create view/projection constant buffer");
        return false;
    }
    
    // Create lighting constant buffer
    bufferDesc.ByteWidth = sizeof(XMFLOAT4) * 2 + sizeof(XMFLOAT3) * 2 + sizeof(float) * 2; // ambient + diffuse + light direction + camera position + padding
    result = device->CreateBuffer(&bufferDesc, nullptr, &m_lightBuffer);
    if (FAILED(result))
    {
        LOG_ERROR("GPUDrivenRenderer: Failed to create lighting constant buffer");
        return false;
    }
    
    // Create material constant buffer
    bufferDesc.ByteWidth = sizeof(XMFLOAT4) * 3; // base color + material properties + padding
    result = device->CreateBuffer(&bufferDesc, nullptr, &m_materialBuffer);
    if (FAILED(result))
    {
        LOG_ERROR("GPUDrivenRenderer: Failed to create material constant buffer");
        return false;
    }
    
    LOG("GPUDrivenRenderer: All constant buffers created successfully");
    return true;
}

void GPUDrivenRenderer::ReleaseConstantBuffers()
{
    if (m_frustumConstantBuffer) { m_frustumConstantBuffer->Release(); m_frustumConstantBuffer = nullptr; }
    if (m_objectCountBuffer) { m_objectCountBuffer->Release(); m_objectCountBuffer = nullptr; }
    if (m_viewProjectionBuffer) { m_viewProjectionBuffer->Release(); m_viewProjectionBuffer = nullptr; }
    if (m_lightBuffer) { m_lightBuffer->Release(); m_lightBuffer = nullptr; }
    if (m_materialBuffer) { m_materialBuffer->Release(); m_materialBuffer = nullptr; }
}

void GPUDrivenRenderer::ExtractFrustumPlanes(const XMMATRIX& viewProjectionMatrix)
{
    // Extract frustum planes from view-projection matrix
    // Plane equations: Ax + By + Cz + D = 0
    
    XMFLOAT4X4 vp;
    XMStoreFloat4x4(&vp, viewProjectionMatrix);
    
    // Left plane: col4 + col1
    m_frustumPlanes[0].x = vp._14 + vp._11;
    m_frustumPlanes[0].y = vp._24 + vp._21;
    m_frustumPlanes[0].z = vp._34 + vp._31;
    m_frustumPlanes[0].w = vp._44 + vp._41;
    
    // Right plane: col4 - col1
    m_frustumPlanes[1].x = vp._14 - vp._11;
    m_frustumPlanes[1].y = vp._24 - vp._21;
    m_frustumPlanes[1].z = vp._34 - vp._31;
    m_frustumPlanes[1].w = vp._44 - vp._41;
    
    // Top plane: col4 - col2
    m_frustumPlanes[2].x = vp._14 - vp._12;
    m_frustumPlanes[2].y = vp._24 - vp._22;
    m_frustumPlanes[2].z = vp._34 - vp._32;
    m_frustumPlanes[2].w = vp._44 - vp._42;
    
    // Bottom plane: col4 + col2
    m_frustumPlanes[3].x = vp._14 + vp._12;
    m_frustumPlanes[3].y = vp._24 + vp._22;
    m_frustumPlanes[3].z = vp._34 + vp._32;
    m_frustumPlanes[3].w = vp._44 + vp._42;
    
    // Near plane: col3
    m_frustumPlanes[4].x = vp._13;
    m_frustumPlanes[4].y = vp._23;
    m_frustumPlanes[4].z = vp._33;
    m_frustumPlanes[4].w = vp._43;
    
    // Far plane: col4 - col3
    m_frustumPlanes[5].x = vp._14 - vp._13;
    m_frustumPlanes[5].y = vp._24 - vp._23;
    m_frustumPlanes[5].z = vp._34 - vp._33;
    m_frustumPlanes[5].w = vp._44 - vp._43;
    
    // Normalize planes
    for (int i = 0; i < 6; ++i)
    {
        XMVECTOR plane = XMLoadFloat4(&m_frustumPlanes[i]);
        plane = XMPlaneNormalize(plane);
        XMStoreFloat4(&m_frustumPlanes[i], plane);
    }
}

bool GPUDrivenRenderer::InitializeIndirectDrawBuffers(ID3D11Device* device, UINT maxObjects)
{
    HRESULT result;
    D3D11_BUFFER_DESC bufferDesc = {};
    
    LOG("GPUDrivenRenderer: InitializeIndirectDrawBuffers - Creating TRUE GPU-driven rendering buffers");
    
    // Create draw arguments buffer for DrawIndexedInstancedIndirect
    // Structure: IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth = sizeof(UINT) * 5; // DrawIndexedInstancedIndirect arguments
    bufferDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
    bufferDesc.CPUAccessFlags = 0;
    bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
    bufferDesc.StructureByteStride = 0;
    
    result = device->CreateBuffer(&bufferDesc, nullptr, &m_drawArgumentsBuffer);
    if (FAILED(result))
    {
        LOG_ERROR("GPUDrivenRenderer: Failed to create draw arguments buffer - HRESULT: " + std::to_string(result));
        return false;
    }
    LOG("GPUDrivenRenderer: Draw arguments buffer created successfully");
    
    // Create UAV for draw arguments buffer
    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = DXGI_FORMAT_R32_UINT;
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.NumElements = 5; // 5 UINT values
    uavDesc.Buffer.Flags = 0;
    
    result = device->CreateUnorderedAccessView(m_drawArgumentsBuffer, &uavDesc, &m_drawArgumentsUAV);
    if (FAILED(result))
    {
        LOG_ERROR("GPUDrivenRenderer: Failed to create draw arguments UAV - HRESULT: " + std::to_string(result));
        return false;
    }
    LOG("GPUDrivenRenderer: Draw arguments UAV created successfully");
    
    // Create visible objects buffer for stream compaction
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth = sizeof(UINT) * maxObjects; // Array of visible object indices
    bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
    bufferDesc.CPUAccessFlags = 0;
    bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    bufferDesc.StructureByteStride = sizeof(UINT);
    
    result = device->CreateBuffer(&bufferDesc, nullptr, &m_visibleObjectsBuffer);
    if (FAILED(result))
    {
        LOG_ERROR("GPUDrivenRenderer: Failed to create visible objects buffer - HRESULT: " + std::to_string(result));
        return false;
    }
    LOG("GPUDrivenRenderer: Visible objects buffer created successfully");
    
    // Create SRV for visible objects buffer
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = maxObjects;
    
    result = device->CreateShaderResourceView(m_visibleObjectsBuffer, &srvDesc, &m_visibleObjectsSRV);
    if (FAILED(result))
    {
        LOG_ERROR("GPUDrivenRenderer: Failed to create visible objects SRV - HRESULT: " + std::to_string(result));
        return false;
    }
    LOG("GPUDrivenRenderer: Visible objects SRV created successfully");
    
    // Create UAV for visible objects buffer
    uavDesc.Format = DXGI_FORMAT_UNKNOWN;
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.NumElements = maxObjects;
    uavDesc.Buffer.Flags = 0;
    
    result = device->CreateUnorderedAccessView(m_visibleObjectsBuffer, &uavDesc, &m_visibleObjectsUAV);
    if (FAILED(result))
    {
        LOG_ERROR("GPUDrivenRenderer: Failed to create visible objects UAV - HRESULT: " + std::to_string(result));
        return false;
    }
    LOG("GPUDrivenRenderer: Visible objects UAV created successfully");
    
    // Create visible count buffer (single UINT counter)
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth = sizeof(UINT) * 4; // 16-byte aligned for single counter + padding
    bufferDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE; // Need SRV for UpdateDrawArgs CS
    bufferDesc.CPUAccessFlags = 0;
    bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    bufferDesc.StructureByteStride = sizeof(UINT);
    
    result = device->CreateBuffer(&bufferDesc, nullptr, &m_visibleCountBuffer);
    if (FAILED(result))
    {
        LOG_ERROR("GPUDrivenRenderer: Failed to create visible count buffer - HRESULT: " + std::to_string(result));
        return false;
    }
    LOG("GPUDrivenRenderer: Visible count buffer created successfully");
    
    // Create UAV for visible count buffer
    uavDesc.Format = DXGI_FORMAT_UNKNOWN;
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.NumElements = 4; // 4 UINT values (counter + padding)
    uavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_COUNTER; // Enable atomic counter
    
    result = device->CreateUnorderedAccessView(m_visibleCountBuffer, &uavDesc, &m_visibleCountUAV);
    if (FAILED(result))
    {
        LOG_ERROR("GPUDrivenRenderer: Failed to create visible count UAV - HRESULT: " + std::to_string(result));
        return false;
    }
    LOG("GPUDrivenRenderer: Visible count UAV created successfully");
    
    // Create staging buffer for reading visible count (optional - for debugging/UI)
    bufferDesc.Usage = D3D11_USAGE_STAGING;
    bufferDesc.ByteWidth = sizeof(UINT) * 4;
    bufferDesc.BindFlags = 0;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    bufferDesc.MiscFlags = 0;
    bufferDesc.StructureByteStride = 0;
    
    result = device->CreateBuffer(&bufferDesc, nullptr, &m_visibleCountStagingBuffer);
    if (FAILED(result))
    {
        LOG_ERROR("GPUDrivenRenderer: Failed to create visible count staging buffer - HRESULT: " + std::to_string(result));
        return false;
    }
    LOG("GPUDrivenRenderer: Visible count staging buffer created successfully");
    
    LOG("GPUDrivenRenderer: All indirect draw buffers initialized successfully");
    return true;
}

void GPUDrivenRenderer::ReleaseIndirectDrawBuffers()
{
    if (m_drawArgumentsUAV) { m_drawArgumentsUAV->Release(); m_drawArgumentsUAV = nullptr; }
    if (m_drawArgumentsBuffer) { m_drawArgumentsBuffer->Release(); m_drawArgumentsBuffer = nullptr; }
    
    if (m_visibleObjectsUAV) { m_visibleObjectsUAV->Release(); m_visibleObjectsUAV = nullptr; }
    if (m_visibleObjectsSRV) { m_visibleObjectsSRV->Release(); m_visibleObjectsSRV = nullptr; }
    if (m_visibleObjectsBuffer) { m_visibleObjectsBuffer->Release(); m_visibleObjectsBuffer = nullptr; }
    
    if (m_visibleCountUAV) { m_visibleCountUAV->Release(); m_visibleCountUAV = nullptr; }
    if (m_visibleCountBuffer) { m_visibleCountBuffer->Release(); m_visibleCountBuffer = nullptr; }
    if (m_visibleCountStagingBuffer) { m_visibleCountStagingBuffer->Release(); m_visibleCountStagingBuffer = nullptr; }
} 