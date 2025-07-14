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
    LOG("GPUDrivenRenderer::UpdateObjects called with " + std::to_string(objects.size()) + " objects");
    
    if (objects.empty())
    {
        LOG_WARNING("GPUDrivenRenderer::UpdateObjects - No objects provided");
        return;
    }
    

    
    m_indirectBuffer.UpdateObjectData(context, objects);
}

void GPUDrivenRenderer::UpdateCamera(ID3D11DeviceContext* context, const XMFLOAT3& cameraPos, const XMFLOAT3& cameraForward,
                                   const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix)
{
    m_cameraPosition = cameraPos;
    m_cameraForward = cameraForward;
    m_viewMatrix = viewMatrix;
    m_projectionMatrix = projectionMatrix;
    
    // Generate frustum data and update buffer
    FrustumData frustumData = GenerateFrustumData(viewMatrix, projectionMatrix);
    m_indirectBuffer.UpdateFrustumData(context, frustumData);
}

void GPUDrivenRenderer::Render(ID3D11DeviceContext* context, ID3D11Buffer* vertexBuffer, ID3D11Buffer* indexBuffer,
                              ID3D11VertexShader* vertexShader, ID3D11PixelShader* pixelShader, ID3D11InputLayout* inputLayout,
                              Model* model, PBRShader* pbrShader, Light* light, Camera* camera, D3D11Device* direct3D)
{
    if (!m_enableGPUDriven)
    {
        LOG_WARNING("GPU-driven rendering is disabled");
        return;
    }
    
    // Comprehensive validation of all input parameters
    LOG("GPUDrivenRenderer::Render - Validating input parameters:");
    LOG("  Context: " + std::string(context ? "valid" : "null"));
    LOG("  VertexBuffer: " + std::string(vertexBuffer ? "valid" : "null"));
    LOG("  IndexBuffer: " + std::string(indexBuffer ? "valid" : "null"));
    LOG("  VertexShader: " + std::string(vertexShader ? "valid" : "null"));
    LOG("  PixelShader: " + std::string(pixelShader ? "valid" : "null"));
    LOG("  InputLayout: " + std::string(inputLayout ? "valid" : "null"));
    
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
    LOG("GPUDrivenRenderer::Render - Getting compute shader resources");
    ID3D11ShaderResourceView* objectDataSRV = m_indirectBuffer.GetObjectDataSRV();
    ID3D11ShaderResourceView* lodLevelsSRV = m_indirectBuffer.GetLODLevelsSRV();
    ID3D11ShaderResourceView* frustumSRV = m_indirectBuffer.GetFrustumSRV();
    ID3D11UnorderedAccessView* drawCommandUAV = m_indirectBuffer.GetDrawCommandUAV();
    ID3D11UnorderedAccessView* visibleObjectCountUAV = m_indirectBuffer.GetVisibleObjectCountUAV();
    ID3D11Buffer* frustumBuffer = m_indirectBuffer.GetFrustumBuffer();
    
    LOG("GPUDrivenRenderer::Render - Compute shader resource validation:");
    LOG("  ObjectDataSRV: " + std::string(objectDataSRV ? "valid" : "null"));
    LOG("  LODLevelsSRV: " + std::string(lodLevelsSRV ? "valid" : "null"));
    LOG("  FrustumSRV: " + std::string(frustumSRV ? "valid" : "null"));
    LOG("  DrawCommandUAV: " + std::string(drawCommandUAV ? "valid" : "null"));
    LOG("  VisibleObjectCountUAV: " + std::string(visibleObjectCountUAV ? "valid" : "null"));
    LOG("  FrustumBuffer: " + std::string(frustumBuffer ? "valid" : "null"));
    
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
    LOG("GPUDrivenRenderer::Render - Generating world matrices");
    ID3D11UnorderedAccessView* worldMatrixUAV = m_indirectBuffer.GetWorldMatrixUAV();
    if (!worldMatrixUAV)
    {
        LOG_ERROR("GPUDrivenRenderer::Render - World matrix UAV is null");
        return;
    }
    
    // Set up world matrix generation compute shader resources
    m_worldMatrixGenerationCS->SetShaderResourceView(context, 0, objectDataSRV);
    m_worldMatrixGenerationCS->SetUnorderedAccessView(context, 0, worldMatrixUAV);
    
    // Dispatch world matrix generation compute shader
    UINT actualObjectCount = m_indirectBuffer.GetObjectCount();
    UINT worldMatrixThreadGroupCount = (actualObjectCount + 63) / 64;
    LOG("GPUDrivenRenderer::Render - Dispatching world matrix generation with " + std::to_string(worldMatrixThreadGroupCount) + " thread groups");
    m_worldMatrixGenerationCS->Dispatch(context, worldMatrixThreadGroupCount, 1, 1);
    
    // Set up compute shader resources for command generation
    LOG("GPUDrivenRenderer::Render - Setting up compute shader resources");
    m_commandGenerationCS->SetShaderResourceView(context, 0, objectDataSRV);
    m_commandGenerationCS->SetShaderResourceView(context, 1, lodLevelsSRV);
    m_commandGenerationCS->SetShaderResourceView(context, 2, frustumSRV);
    m_commandGenerationCS->SetUnorderedAccessView(context, 0, drawCommandUAV);
    m_commandGenerationCS->SetUnorderedAccessView(context, 1, visibleObjectCountUAV);
    m_commandGenerationCS->SetConstantBuffer(context, 0, frustumBuffer);
    
    // Validate compute shader before dispatching
    if (!m_commandGenerationCS || !m_commandGenerationCS->GetComputeShader())
    {
        LOG_ERROR("GPUDrivenRenderer::Render - Command generation compute shader is null or invalid");
        return;
    }
    
    // Dispatch compute shader to generate draw commands
    // We should only dispatch enough thread groups to process the actual number of objects
    // Get the actual number of objects from the indirect buffer
    UINT threadGroupCount = (actualObjectCount + 63) / 64; // 64 threads per group
    LOG("GPUDrivenRenderer::Render - Dispatching compute shader with " + std::to_string(threadGroupCount) + " thread groups for " + std::to_string(actualObjectCount) + " objects");
    m_commandGenerationCS->Dispatch(context, threadGroupCount, 1, 1);
    
    // Check if compute shader generated any draw commands
    LOG("GPUDrivenRenderer::Render - Getting visible object count");
    UINT finalVisibleCount = m_indirectBuffer.GetVisibleObjectCount();
    LOG("GPUDrivenRenderer::Render - Visible object count: " + std::to_string(finalVisibleCount));
    
    if (finalVisibleCount == 0)
    {
        LOG_WARNING("GPUDrivenRenderer::Render - Compute shader found no visible objects. This might indicate:");
        LOG_WARNING("1. All objects are outside the frustum");
        LOG_WARNING("2. Object data is not being passed correctly to the compute shader");
        LOG_WARNING("3. Frustum culling is too aggressive");
        LOG_WARNING("4. Compute shader is not executing properly");
    }
    
    // Track compute shader dispatch
    PerformanceProfiler::GetInstance().IncrementComputeDispatches();
    
    // Set up rendering pipeline
    LOG("GPUDrivenRenderer::Render - Setting input layout");
    context->IASetInputLayout(inputLayout);
    
    // Set vertex buffer with proper stride and offset
    UINT stride = sizeof(EngineTypes::VertexType);
    UINT offset = 0;
    LOG("GPUDrivenRenderer::Render - Setting vertex buffer with stride: " + std::to_string(stride));
    context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    
    // Additional safety check right before IASetIndexBuffer
    if (!indexBuffer)
    {
        LOG_ERROR("GPUDrivenRenderer::Render - indexBuffer became null before IASetIndexBuffer call");
        LOG_ERROR("GPUDrivenRenderer::Render - Falling back to CPU-driven rendering");
        return;
    }
    
    // Log buffer pointer value for debugging
    LOG("GPUDrivenRenderer::Render - indexBuffer pointer: " + std::to_string(reinterpret_cast<uintptr_t>(indexBuffer)));
    
    // Try to set the index buffer with error handling
    try
    {
        context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
    }
    catch (...)
    {
        LOG_ERROR("GPUDrivenRenderer::Render - Exception occurred while setting index buffer");
        LOG_ERROR("GPUDrivenRenderer::Render - Falling back to CPU-driven rendering");
        return;
    }
    
    // For GPU-driven rendering, we need to use a special vertex shader that can handle per-instance world matrices
    // For now, we'll use the regular vertex shader but set up the world matrix buffer
    context->VSSetShader(vertexShader, nullptr, 0);
    context->PSSetShader(pixelShader, nullptr, 0);
    
    // Validate draw command buffer before performing indirect draw
    ID3D11Buffer* drawCommandBuffer = m_indirectBuffer.GetDrawCommandBuffer();
    if (!drawCommandBuffer)
    {
        LOG_ERROR("GPUDrivenRenderer::Render - draw command buffer is null");
        return;
    }
    

    
    // Only perform indirect draw if we have visible objects
    if (finalVisibleCount > 0)
    {
        LOG("GPUDrivenRenderer::Render - Performing indirect draw call with " + std::to_string(finalVisibleCount) + " visible objects");
        
        // Log draw command details for debugging
        LOG("GPUDrivenRenderer::Render - Draw command details:");
        LOG("  Draw command buffer pointer: " + std::to_string(reinterpret_cast<uintptr_t>(drawCommandBuffer)));
        LOG("  Vertex buffer pointer: " + std::to_string(reinterpret_cast<uintptr_t>(vertexBuffer)));
        LOG("  Index buffer pointer: " + std::to_string(reinterpret_cast<uintptr_t>(indexBuffer)));
        LOG("  Vertex shader pointer: " + std::to_string(reinterpret_cast<uintptr_t>(vertexShader)));
        LOG("  Pixel shader pointer: " + std::to_string(reinterpret_cast<uintptr_t>(pixelShader)));
        LOG("  Input layout pointer: " + std::to_string(reinterpret_cast<uintptr_t>(inputLayout)));
        
        // Use instanced rendering with world matrices
        LOG("GPUDrivenRenderer::Render - Using instanced rendering with world matrices");
        
        // Set the world matrix buffer as a shader resource view
        ID3D11ShaderResourceView* worldMatrixSRV = m_indirectBuffer.GetWorldMatrixSRV();
        if (!worldMatrixSRV)
        {
            LOG_ERROR("GPUDrivenRenderer::Render - World matrix SRV is null");
            return;
        }
        
            // Set the world matrix buffer as a shader resource view for the vertex shader
    context->VSSetShaderResources(1, 1, &worldMatrixSRV);
    
    // Set up texture resources for the pixel shader
    // The PBR pixel shader expects textures at registers t0-t5
    // We need to get these from the Model object
    ID3D11ShaderResourceView* diffuseTexture = model->GetDiffuseTexture();
    ID3D11ShaderResourceView* normalTexture = model->GetNormalTexture();
    ID3D11ShaderResourceView* metallicTexture = model->GetMetallicTexture();
    ID3D11ShaderResourceView* roughnessTexture = model->GetRoughnessTexture();
    ID3D11ShaderResourceView* emissionTexture = model->GetEmissionTexture();
    ID3D11ShaderResourceView* aoTexture = model->GetAOTexture();
    
    // Set shader texture resources in the pixel shader
    context->PSSetShaderResources(0, 1, &diffuseTexture);
    context->PSSetShaderResources(1, 1, &normalTexture);
    context->PSSetShaderResources(2, 1, &metallicTexture);
    context->PSSetShaderResources(3, 1, &roughnessTexture);
    context->PSSetShaderResources(4, 1, &emissionTexture);
    context->PSSetShaderResources(5, 1, &aoTexture);
    
    // Set up constant buffers for the PBR shader
    // We need to set up the light buffer and material buffer
    // For now, let's use the PBR shader's setup method with a dummy world matrix
    // and then override the world matrix with our per-instance matrices
    
    // Get view and projection matrices
    XMMATRIX viewMatrix, projectionMatrix;
    camera->GetViewMatrix(viewMatrix);
    direct3D->GetProjectionMatrix(projectionMatrix);
    
    // Set up PBR shader parameters with dummy world matrix
    // The actual world matrices will be provided by the vertex shader via structured buffer
    XMMATRIX dummyWorldMatrix = XMMatrixIdentity();
    
    // Use the PBR shader's setup method to configure constant buffers and sampler state
    pbrShader->SetupShaderParameters(context, dummyWorldMatrix, viewMatrix, projectionMatrix,
                                   diffuseTexture, normalTexture, metallicTexture,
                                   roughnessTexture, emissionTexture, aoTexture,
                                   light->GetDirection(), light->GetAmbientColor(), light->GetDiffuseColor(),
                                   model->GetBaseColor(), model->GetMetallic(), model->GetRoughness(),
                                   model->GetAO(), model->GetEmissionStrength(), camera->GetPosition(), true);
    
    // Set GPU-driven rendering flag in the constant buffer
    // We need to update the constant buffer to include the GPU-driven flag
    // For now, let's modify the shader to always use GPU-driven mode when the buffer is bound
    
    // Perform instanced draw call
    context->DrawIndexedInstanced(61260, finalVisibleCount, 0, 0, 0);
        LOG("GPUDrivenRenderer::Render - Instanced draw call completed with " + std::to_string(finalVisibleCount) + " instances");
    }
    else
    {
        LOG_WARNING("GPUDrivenRenderer::Render - No visible objects, skipping indirect draw");
    }
    
    // Track draw call
    PerformanceProfiler::GetInstance().IncrementIndirectDrawCalls();
    m_drawCallCount = 1; // Single indirect draw call
    
    // Update render count for UI display
    // Note: This should be updated by the Application class, but for now we'll set it here
    // The Application class should call a method to get the render count from GPU-driven renderer
    
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