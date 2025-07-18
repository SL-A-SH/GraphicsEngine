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
    , m_gpuDrivenVertexShader(nullptr)
    , m_gpuDrivenPixelShader(nullptr)
    , m_gpuDrivenInputLayout(nullptr)
    , m_enableGPUDriven(true)
    , m_maxObjects(0)
    , m_renderCount(0)
    , m_cameraPosition(0.0f, 0.0f, 0.0f)
    , m_viewMatrix(XMMatrixIdentity())
    , m_projectionMatrix(XMMatrixIdentity())
{
    LOG("GPUDrivenRenderer: Constructor - Minimal GPU-driven renderer created");
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
    
    LOG("GPUDrivenRenderer: Minimal GPU-driven renderer initialized with " + std::to_string(maxObjects) + " max objects");
    return true;
}

void GPUDrivenRenderer::Shutdown()
{
    LOG("GPUDrivenRenderer: Shutdown - Releasing resources");
    ReleaseComputeShaders();
    ReleaseGPUDrivenShaders();
    m_indirectBuffer.Shutdown();
    LOG("GPUDrivenRenderer: Shutdown completed");
}

bool GPUDrivenRenderer::InitializeComputeShaders(ID3D11Device* device, HWND hwnd)
{
    LOG("GPUDrivenRenderer: InitializeComputeShaders - Starting world matrix generation compute shader initialization");
    
    // Create only world matrix generation compute shader
    m_worldMatrixGenerationCS = new ComputeShader();
    
    LOG("GPUDrivenRenderer: Initializing world matrix generation compute shader");
    bool result = m_worldMatrixGenerationCS->Initialize(device, hwnd, L"../Engine/assets/shaders/WorldMatrixGenerationComputeShader.hlsl", "main");
    if (!result)
    {
        LOG_ERROR("GPUDrivenRenderer: Failed to initialize world matrix generation compute shader");
        return false;
    }
    LOG("GPUDrivenRenderer: World matrix generation compute shader initialized successfully");
    
    // Verify compute shader is valid
    if (!m_worldMatrixGenerationCS->GetComputeShader())
    {
        LOG_ERROR("GPUDrivenRenderer: World matrix generation compute shader is null after initialization");
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
    
    // STEP 1: Generate world matrices using compute shader
    LOG("GPUDrivenRenderer::Render - STEP 1: Generating world matrices");
    
    // Get required buffers
    ID3D11ShaderResourceView* objectDataSRV = m_indirectBuffer.GetObjectDataSRV();
    ID3D11UnorderedAccessView* worldMatrixUAV = m_indirectBuffer.GetWorldMatrixUAV();
    
    if (!objectDataSRV || !worldMatrixUAV)
    {
        LOG_ERROR("GPUDrivenRenderer::Render - Missing compute shader resources");
        //PerformanceProfiler::GetInstance().EndSection();
        return;
    }
    
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
    HRESULT result = direct3D->GetDevice()->CreateBuffer(&objectCountBufferDesc, nullptr, &objectCountBuffer);
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
        
        // STEP 2: Render all objects using instanced rendering
        LOG("GPUDrivenRenderer::Render - STEP 2: Performing instanced rendering");
        
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
        
        D3D11_BUFFER_DESC vpBufferDesc = {};
        vpBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        vpBufferDesc.ByteWidth = sizeof(ViewProjectionBuffer);
        vpBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        vpBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        
        ID3D11Buffer* vpConstantBuffer = nullptr;
        result = direct3D->GetDevice()->CreateBuffer(&vpBufferDesc, nullptr, &vpConstantBuffer);
        if (SUCCEEDED(result))
        {
            D3D11_MAPPED_SUBRESOURCE vpMappedResource;
            result = context->Map(vpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &vpMappedResource);
            if (SUCCEEDED(result))
            {
                memcpy(vpMappedResource.pData, &vpBuffer, sizeof(ViewProjectionBuffer));
                context->Unmap(vpConstantBuffer, 0);
                
                // Set constant buffer to vertex shader
                context->VSSetConstantBuffers(0, 1, &vpConstantBuffer);
                
                // Bind model textures if available
                if (model)
                {
                    ID3D11ShaderResourceView* diffuseTexture = model->GetDiffuseTexture();
                    if (diffuseTexture)
                    {
                        context->PSSetShaderResources(0, 1, &diffuseTexture);
                    }
                }
                
                // Get index count from model
                int indexCount = model->GetIndexCount();
                LOG("GPUDrivenRenderer::Render - Model index count: " + std::to_string(indexCount));
                
                // Perform instanced rendering - render all objects in one call
                LOG("GPUDrivenRenderer::Render - Executing DrawIndexedInstanced: " + std::to_string(indexCount) + " indices, " + std::to_string(objectCount) + " instances");
                context->DrawIndexedInstanced(indexCount, objectCount, 0, 0, 0);
                
                m_renderCount = static_cast<int>(objectCount);
                LOG("GPUDrivenRenderer::Render - GPU-driven rendering completed successfully - rendered " + std::to_string(m_renderCount) + " objects");
                
                // Cleanup
                vpConstantBuffer->Release();
            }
            else
            {
                LOG_ERROR("GPUDrivenRenderer::Render - Failed to map view/projection constant buffer");
                vpConstantBuffer->Release();
            }
        }
        else
        {
            LOG_ERROR("GPUDrivenRenderer::Render - Failed to create view/projection constant buffer");
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