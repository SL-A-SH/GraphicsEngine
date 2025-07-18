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
#include <d3dcompiler.h>

GPUDrivenRenderer::GPUDrivenRenderer()
    : m_worldMatrixGenerationCS(nullptr)
    , m_frustumCullingCS(nullptr)
    , m_gpuDrivenVertexShader(nullptr)
    , m_gpuDrivenPixelShader(nullptr)
    , m_gpuDrivenInputLayout(nullptr)
    , m_visibilityBuffer(nullptr)
    , m_visibilitySRV(nullptr)
    , m_visibilityUAV(nullptr)
    , m_visibilityReadbackBuffer(nullptr)
    , m_enableGPUDriven(true)
    , m_maxObjects(0)
    , m_renderCount(0)
    , m_cameraPosition(0.0f, 0.0f, 0.0f)
    , m_viewMatrix(XMMatrixIdentity())
    , m_projectionMatrix(XMMatrixIdentity())
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
    
    LOG("GPUDrivenRenderer: GPU-driven renderer with frustum culling initialized with " + std::to_string(maxObjects) + " max objects");
    return true;
}

void GPUDrivenRenderer::Shutdown()
{
    LOG("GPUDrivenRenderer: Shutdown - Releasing resources");
    ReleaseComputeShaders();
    ReleaseGPUDrivenShaders();
    ReleaseVisibilityBuffer();
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
}

void GPUDrivenRenderer::UpdateObjects(ID3D11DeviceContext* context, const std::vector<ObjectData>& objects)
{
    if (objects.empty())
    {
        LOG_WARNING("GPUDrivenRenderer::UpdateObjects - No objects provided");
        return;
    }
    
    LOG("GPUDrivenRenderer::UpdateObjects - Updating " + std::to_string(objects.size()) + " objects");
    m_indirectBuffer.UpdateObjectData(context, objects);
    LOG("GPUDrivenRenderer::UpdateObjects - Object data updated successfully");
}

void GPUDrivenRenderer::UpdateCamera(ID3D11DeviceContext* context, const XMFLOAT3& cameraPos, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix)
{
    LOG("GPUDrivenRenderer::UpdateCamera - Updating camera data");
    m_cameraPosition = cameraPos;
    m_viewMatrix = viewMatrix;
    m_projectionMatrix = projectionMatrix;
    
    // Extract frustum planes for GPU frustum culling
    XMMATRIX viewProjectionMatrix = XMMatrixMultiply(viewMatrix, projectionMatrix);
    ExtractFrustumPlanes(viewProjectionMatrix);
    
    LOG("GPUDrivenRenderer::UpdateCamera - Camera position: (" + 
        std::to_string(cameraPos.x) + ", " + 
        std::to_string(cameraPos.y) + ", " + 
        std::to_string(cameraPos.z) + ")");
}

void GPUDrivenRenderer::Render(ID3D11DeviceContext* context, ID3D11Buffer* vertexBuffer, ID3D11Buffer* indexBuffer,
                              Model* model, PBRShader* pbrShader, Light* light, Camera* camera, D3D11Device* direct3D)
{
    if (!m_enableGPUDriven)
    {
        LOG_WARNING("GPUDrivenRenderer::Render - GPU-driven rendering is disabled");
        return;
    }
    
    LOG("GPUDrivenRenderer::Render - Starting minimal GPU-driven rendering");
    
    // Validate required resources
    if (!context || !vertexBuffer || !indexBuffer || !model)
    {
        LOG_ERROR("GPUDrivenRenderer::Render - Missing required resources");
        return;
    }
    
    // Start profiling GPU-driven rendering
    //PerformanceProfiler::GetInstance().BeginSection("Minimal GPU-Driven Rendering");
    
    // Get object count
    UINT objectCount = m_indirectBuffer.GetObjectCount();
    LOG("GPUDrivenRenderer::Render - Object count: " + std::to_string(objectCount));
    
    if (objectCount == 0)
    {
        LOG_WARNING("GPUDrivenRenderer::Render - No objects to render");
        //PerformanceProfiler::GetInstance().EndSection();
        return;
    }
    
            // Get required buffers first
        ID3D11ShaderResourceView* objectDataSRV = m_indirectBuffer.GetObjectDataSRV();
        ID3D11UnorderedAccessView* worldMatrixUAV = m_indirectBuffer.GetWorldMatrixUAV();
        
        if (!objectDataSRV || !worldMatrixUAV)
        {
            LOG_ERROR("GPUDrivenRenderer::Render - Missing compute shader resources");
            return;
        }
        
        // STEP 1: Perform frustum culling
        LOG("GPUDrivenRenderer::Render - STEP 1: Performing GPU frustum culling");
        
        // Create frustum constant buffer
        struct FrustumBuffer
        {
            XMFLOAT4 frustumPlanes[6];
            UINT objectCount;
            UINT padding[3];
        };
        
        FrustumBuffer frustumData;
        for (int i = 0; i < 6; ++i)
        {
            frustumData.frustumPlanes[i] = m_frustumPlanes[i];
        }
        frustumData.objectCount = objectCount;
        frustumData.padding[0] = 0;
        frustumData.padding[1] = 0;
        frustumData.padding[2] = 0;
        
        D3D11_BUFFER_DESC frustumBufferDesc = {};
        frustumBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        frustumBufferDesc.ByteWidth = sizeof(FrustumBuffer);
        frustumBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        frustumBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        
        ID3D11Buffer* frustumConstantBuffer = nullptr;
        HRESULT result = direct3D->GetDevice()->CreateBuffer(&frustumBufferDesc, nullptr, &frustumConstantBuffer);
        if (SUCCEEDED(result))
        {
            D3D11_MAPPED_SUBRESOURCE frustumMappedResource;
            result = context->Map(frustumConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &frustumMappedResource);
            if (SUCCEEDED(result))
            {
                memcpy(frustumMappedResource.pData, &frustumData, sizeof(FrustumBuffer));
                context->Unmap(frustumConstantBuffer, 0);
                
                // Set up frustum culling compute shader
                m_frustumCullingCS->SetShaderResourceView(context, 0, objectDataSRV);
                m_frustumCullingCS->SetUnorderedAccessView(context, 0, m_visibilityUAV);
                m_frustumCullingCS->SetConstantBuffer(context, 0, frustumConstantBuffer);
                
                // Calculate thread groups for frustum culling
                UINT frustumThreadGroupCount = (objectCount + 63) / 64;
                LOG("GPUDrivenRenderer::Render - Dispatching frustum culling: " + std::to_string(frustumThreadGroupCount) + " thread groups for " + std::to_string(objectCount) + " objects");
                
                // Dispatch frustum culling compute shader
                m_frustumCullingCS->Dispatch(context, frustumThreadGroupCount, 1, 1);
                
                // Flush and unbind UAV
                context->Flush();
                m_frustumCullingCS->SetUnorderedAccessView(context, 0, nullptr);
                context->Flush();
                
                LOG("GPUDrivenRenderer::Render - GPU frustum culling completed");
                
                // Copy visibility buffer to readback buffer for CPU access
                context->CopyResource(m_visibilityReadbackBuffer, m_visibilityBuffer);
                
                // Read back visibility results to count visible objects
                D3D11_MAPPED_SUBRESOURCE mappedVisibility;
                HRESULT mapResult = context->Map(m_visibilityReadbackBuffer, 0, D3D11_MAP_READ, 0, &mappedVisibility);
                if (SUCCEEDED(mapResult))
                {
                    UINT* visibilityData = static_cast<UINT*>(mappedVisibility.pData);
                    UINT visibleCount = 0;
                    
                    for (UINT i = 0; i < objectCount; ++i)
                    {
                        if (visibilityData[i] == 1)
                        {
                            visibleCount++;
                        }
                    }
                    
                    context->Unmap(m_visibilityReadbackBuffer, 0);
                    
                    LOG("GPUDrivenRenderer::Render - GPU Frustum Culling Results: " + std::to_string(visibleCount) + "/" + std::to_string(objectCount) + " objects visible");
                    
                    // Update render count to reflect visible objects (for demonstration)
                    m_renderCount = static_cast<int>(visibleCount);
                }
                else
                {
                    LOG_ERROR("GPUDrivenRenderer::Render - Failed to map visibility readback buffer");
                    // Keep original render count as fallback
                    m_renderCount = static_cast<int>(objectCount);
                }
                
                frustumConstantBuffer->Release();
            }
            else
            {
                LOG_ERROR("GPUDrivenRenderer::Render - Failed to map frustum constant buffer");
                frustumConstantBuffer->Release();
                return;
            }
        }
        else
        {
            LOG_ERROR("GPUDrivenRenderer::Render - Failed to create frustum constant buffer");
            return;
        }
        
        // STEP 2: Generate world matrices using compute shader
        LOG("GPUDrivenRenderer::Render - STEP 2: Generating world matrices");
    
    // Set up world matrix generation compute shader
    m_worldMatrixGenerationCS->SetShaderResourceView(context, 0, objectDataSRV);
    m_worldMatrixGenerationCS->SetUnorderedAccessView(context, 0, worldMatrixUAV);
    
        // Create object count constant buffer
        D3D11_BUFFER_DESC objectCountBufferDesc = {};
        objectCountBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        objectCountBufferDesc.ByteWidth = sizeof(UINT) * 4; // 16-byte aligned
        objectCountBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        objectCountBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        
        ID3D11Buffer* objectCountBuffer = nullptr;
        result = direct3D->GetDevice()->CreateBuffer(&objectCountBufferDesc, nullptr, &objectCountBuffer);
    if (FAILED(result))
    {
        LOG_ERROR("GPUDrivenRenderer::Render - Failed to create object count constant buffer");
        //PerformanceProfiler::GetInstance().EndSection();
        return;
    }
    
    // Update object count constant buffer
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    result = context->Map(objectCountBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (SUCCEEDED(result))
    {
        UINT* objectCountData = static_cast<UINT*>(mappedResource.pData);
        objectCountData[0] = objectCount;
        objectCountData[1] = 0; // padding
        objectCountData[2] = 0; // padding
        objectCountData[3] = 0; // padding
        context->Unmap(objectCountBuffer, 0);
        
        m_worldMatrixGenerationCS->SetConstantBuffer(context, 0, objectCountBuffer);
        
        // Calculate thread groups
        UINT threadGroupCount = (objectCount + 63) / 64;
        LOG("GPUDrivenRenderer::Render - Dispatching world matrix generation: " + std::to_string(threadGroupCount) + " thread groups for " + std::to_string(objectCount) + " objects");
        
        // Dispatch compute shader
        m_worldMatrixGenerationCS->Dispatch(context, threadGroupCount, 1, 1);
        
        // Flush and unbind UAV
        context->Flush();
        m_worldMatrixGenerationCS->SetUnorderedAccessView(context, 0, nullptr);
        context->Flush();
        
        LOG("GPUDrivenRenderer::Render - World matrix generation completed");
        
        // STEP 3: Render all objects using instanced rendering (for now, all objects - visibility will be used later)
        LOG("GPUDrivenRenderer::Render - STEP 3: Performing instanced rendering");
        
        // Set up rendering pipeline
        context->IASetInputLayout(m_gpuDrivenInputLayout);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        
        // Set vertex and index buffers
        UINT stride = sizeof(EngineTypes::VertexType);
        UINT offset = 0;
        context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
        context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
        
        // Set GPU-driven shaders
        context->VSSetShader(m_gpuDrivenVertexShader, nullptr, 0);
        context->PSSetShader(m_gpuDrivenPixelShader, nullptr, 0);
        
        // Bind world matrix buffer to vertex shader
        ID3D11ShaderResourceView* worldMatrixSRV = m_indirectBuffer.GetWorldMatrixSRV();
        if (worldMatrixSRV)
        {
            context->VSSetShaderResources(1, 1, &worldMatrixSRV);
        }
        
        // Set up view and projection matrices
        struct ViewProjectionBuffer
        {
            XMMATRIX viewMatrix;
            XMMATRIX projectionMatrix;
        };
        
        ViewProjectionBuffer vpBuffer;
        vpBuffer.viewMatrix = XMMatrixTranspose(m_viewMatrix);
        vpBuffer.projectionMatrix = XMMatrixTranspose(m_projectionMatrix);
        
        // Set up lighting and material parameters to match CPU PBR shader
        struct LightBuffer
        {
            XMFLOAT4 ambientColor;
            XMFLOAT4 diffuseColor;
            XMFLOAT3 lightDirection;
            float padding1;
            XMFLOAT3 cameraPosition;
            float padding2;
        };
        
        struct MaterialBuffer
        {
            XMFLOAT4 baseColor;
            XMFLOAT4 materialProperties; // x=metallic, y=roughness, z=ao, w=emissionStrength
            XMFLOAT4 materialPadding;
        };
        
        LightBuffer lightBuffer;
        if (light)
        {
            lightBuffer.ambientColor = light->GetAmbientColor();
            lightBuffer.diffuseColor = light->GetDiffuseColor();
            lightBuffer.lightDirection = light->GetDirection();
            lightBuffer.padding1 = 0.0f;
        }
        else
        {
            lightBuffer.ambientColor = XMFLOAT4(0.15f, 0.15f, 0.15f, 1.0f);
            lightBuffer.diffuseColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
            lightBuffer.lightDirection = XMFLOAT3(0.0f, 0.0f, 1.0f);
            lightBuffer.padding1 = 0.0f;
        }
        lightBuffer.cameraPosition = m_cameraPosition;
        lightBuffer.padding2 = 0.0f;
        
        MaterialBuffer materialBuffer;
        if (model)
        {
            materialBuffer.baseColor = model->GetBaseColor();
            materialBuffer.materialProperties = XMFLOAT4(
                model->GetMetallic(),      // x = metallic
                model->GetRoughness(),     // y = roughness
                model->GetAO(),            // z = ao
                model->GetEmissionStrength() // w = emissionStrength
            );
        }
        else
        {
            materialBuffer.baseColor = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
            materialBuffer.materialProperties = XMFLOAT4(0.0f, 0.5f, 1.0f, 0.0f); // default PBR values
        }
        materialBuffer.materialPadding = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
        
        D3D11_BUFFER_DESC vpBufferDesc = {};
        vpBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        vpBufferDesc.ByteWidth = sizeof(ViewProjectionBuffer);
        vpBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        vpBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        
        // Create lighting constant buffer
        D3D11_BUFFER_DESC lightBufferDesc = {};
        lightBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        lightBufferDesc.ByteWidth = sizeof(LightBuffer);
        lightBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        lightBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        
        // Create material constant buffer
        D3D11_BUFFER_DESC materialBufferDesc = {};
        materialBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        materialBufferDesc.ByteWidth = sizeof(MaterialBuffer);
        materialBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        materialBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        
        ID3D11Buffer* vpConstantBuffer = nullptr;
        ID3D11Buffer* lightConstantBuffer = nullptr;
        ID3D11Buffer* materialConstantBuffer = nullptr;
        
        result = direct3D->GetDevice()->CreateBuffer(&vpBufferDesc, nullptr, &vpConstantBuffer);
        if (SUCCEEDED(result))
        {
            result = direct3D->GetDevice()->CreateBuffer(&lightBufferDesc, nullptr, &lightConstantBuffer);
        }
        if (SUCCEEDED(result))
        {
            result = direct3D->GetDevice()->CreateBuffer(&materialBufferDesc, nullptr, &materialConstantBuffer);
        }
        
        if (SUCCEEDED(result))
        {
            // Map and update view/projection buffer
            D3D11_MAPPED_SUBRESOURCE vpMappedResource;
            result = context->Map(vpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &vpMappedResource);
            if (SUCCEEDED(result))
            {
                memcpy(vpMappedResource.pData, &vpBuffer, sizeof(ViewProjectionBuffer));
                context->Unmap(vpConstantBuffer, 0);
                
                // Map and update lighting buffer
                D3D11_MAPPED_SUBRESOURCE lightMappedResource;
                result = context->Map(lightConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &lightMappedResource);
                if (SUCCEEDED(result))
                {
                    memcpy(lightMappedResource.pData, &lightBuffer, sizeof(LightBuffer));
                    context->Unmap(lightConstantBuffer, 0);
                    
                    // Map and update material buffer
                    D3D11_MAPPED_SUBRESOURCE materialMappedResource;
                    result = context->Map(materialConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &materialMappedResource);
                    if (SUCCEEDED(result))
                    {
                        memcpy(materialMappedResource.pData, &materialBuffer, sizeof(MaterialBuffer));
                        context->Unmap(materialConstantBuffer, 0);
                        
                        // Set constant buffers: VS gets view/projection, PS gets lighting and material
                        context->VSSetConstantBuffers(0, 1, &vpConstantBuffer);
                        context->PSSetConstantBuffers(0, 1, &lightConstantBuffer);
                        context->PSSetConstantBuffers(1, 1, &materialConstantBuffer);
                
                // Bind ALL PBR model textures if available (to match CPU mode)
                if (model)
                {
                    ID3D11ShaderResourceView* textures[6] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
                    
                    // Get all PBR textures
                    textures[0] = model->GetDiffuseTexture();   // Diffuse/Albedo
                    textures[1] = model->GetNormalTexture();    // Normal Map
                    textures[2] = model->GetMetallicTexture();  // Metallic
                    textures[3] = model->GetRoughnessTexture(); // Roughness
                    textures[4] = model->GetEmissionTexture();  // Emission
                    textures[5] = model->GetAOTexture();        // Ambient Occlusion
                    
                    // Bind all textures to match CPU PBR shader behavior
                    context->PSSetShaderResources(0, 6, textures);
                    
                    LOG("GPUDrivenRenderer::Render - Bound PBR textures: " +
                        std::string(textures[0] ? "Diffuse " : "") +
                        std::string(textures[1] ? "Normal " : "") +
                        std::string(textures[2] ? "Metallic " : "") +
                        std::string(textures[3] ? "Roughness " : "") +
                        std::string(textures[4] ? "Emission " : "") +
                        std::string(textures[5] ? "AO " : ""));
                }
                
                // Get index count from model
                int indexCount = model->GetIndexCount();
                LOG("GPUDrivenRenderer::Render - Model index count: " + std::to_string(indexCount));
                
                                        // Perform instanced rendering - render all objects in one call
                        LOG("GPUDrivenRenderer::Render - Executing DrawIndexedInstanced: " + std::to_string(indexCount) + " indices, " + std::to_string(objectCount) + " instances");
                        context->DrawIndexedInstanced(indexCount, objectCount, 0, 0, 0);
                        
                        LOG("GPUDrivenRenderer::Render - GPU-driven rendering completed successfully - " + std::to_string(objectCount) + " objects drawn, " + std::to_string(m_renderCount) + " were visible");
                    }
                    else
                    {
                        LOG_ERROR("GPUDrivenRenderer::Render - Failed to map material constant buffer");
                    }
                }
                else
                {
                    LOG_ERROR("GPUDrivenRenderer::Render - Failed to map lighting constant buffer");
                }
            }
            else
            {
                LOG_ERROR("GPUDrivenRenderer::Render - Failed to map view/projection constant buffer");
            }
            
            // Cleanup all constant buffers
            if (vpConstantBuffer) vpConstantBuffer->Release();
            if (lightConstantBuffer) lightConstantBuffer->Release();
            if (materialConstantBuffer) materialConstantBuffer->Release();
        }
        else
        {
            LOG_ERROR("GPUDrivenRenderer::Render - Failed to create constant buffers");
            if (vpConstantBuffer) vpConstantBuffer->Release();
            if (lightConstantBuffer) lightConstantBuffer->Release();
            if (materialConstantBuffer) materialConstantBuffer->Release();
        }
        
        objectCountBuffer->Release();
    }
    else
    {
        LOG_ERROR("GPUDrivenRenderer::Render - Failed to map object count constant buffer");
        objectCountBuffer->Release();
    }
    
    // End profiling
    //PerformanceProfiler::GetInstance().EndSection();
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
    
    LOG("GPUDrivenRenderer: Frustum planes extracted and normalized");
} 