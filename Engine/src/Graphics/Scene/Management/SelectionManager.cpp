#include "SelectionManager.h"
#include "../../Resource/Model.h"
#include "../../Rendering/Camera.h"
#include "../../D3D11/D3D11Device.h"
#include "../../../Core/System/Logger.h"
#include "../../../Core/Common/EngineTypes.h"
#include <algorithm>

SelectionManager::SelectionManager()
    : m_selectedModelIndex(-1)
    , m_transformMode(TransformMode::None)
    , m_activeAxis(GizmoAxis::None)
    , m_isTransforming(false)
    , m_transformStartPos(0, 0)
    , m_transformStartValue(0, 0, 0)
    , m_gizmoVertexBuffer(nullptr)
    , m_gizmoIndexBuffer(nullptr)
    , m_gizmoVertexCount(0)
    , m_gizmoIndexCount(0)
    , m_xAxisColor(1.0f, 0.0f, 0.0f, 1.0f)
    , m_yAxisColor(0.0f, 1.0f, 0.0f, 1.0f)
    , m_zAxisColor(0.0f, 0.0f, 1.0f, 1.0f)
    , m_selectedAxisColor(1.0f, 1.0f, 0.0f, 1.0f)
{
}

SelectionManager::~SelectionManager()
{
    Shutdown();
}

bool SelectionManager::Initialize(D3D11Device* device)
{
    LOG("Initializing SelectionManager");
    
    // Create gizmo geometry (simple arrows for position, arcs for rotation, lines with cubes for scale)
    CreateGizmoGeometry(device);
    
    LOG("SelectionManager initialized successfully");
    return true;
}

void SelectionManager::Shutdown()
{
    if (m_gizmoVertexBuffer)
    {
        m_gizmoVertexBuffer->Release();
        m_gizmoVertexBuffer = nullptr;
    }
    
    if (m_gizmoIndexBuffer)
    {
        m_gizmoIndexBuffer->Release();
        m_gizmoIndexBuffer = nullptr;
    }
    
    m_gizmoVertexCount = 0;
    m_gizmoIndexCount = 0;
}

void SelectionManager::SelectModel(int modelIndex)
{
    m_selectedModelIndex = modelIndex;
    LOG("Model " + std::to_string(modelIndex) + " selected");
}

void SelectionManager::DeselectAll()
{
    m_selectedModelIndex = -1;
    LOG("All models deselected");
}

bool SelectionManager::IsModelSelected(int modelIndex) const
{
    return m_selectedModelIndex == modelIndex;
}



void SelectionManager::StartTransform(const XMFLOAT2& screenPos, const XMMATRIX& viewMatrix, 
                                     const XMMATRIX& projectionMatrix, const Camera* camera)
{
    if (m_selectedModelIndex < 0 || m_transformMode == TransformMode::None)
        return;
    
    m_isTransforming = true;
    m_transformStartPos = screenPos;
    
    // Store the current transform value based on mode
    // This will be used to calculate the delta
    m_transformStartValue = XMFLOAT3(0, 0, 0);
}

void SelectionManager::UpdateTransform(const XMFLOAT2& screenPos, const XMMATRIX& viewMatrix, 
                                      const XMMATRIX& projectionMatrix, const Camera* camera,
                                      std::vector<ModelInstance>& models)
{
    if (!m_isTransforming || m_selectedModelIndex < 0 || m_selectedModelIndex >= static_cast<int>(models.size()))
        return;
    
    ModelInstance& selectedModel = models[m_selectedModelIndex];
    
    // Calculate transform delta based on mouse movement
    XMFLOAT3 axis = GetTransformAxis(m_activeAxis);
    float delta = CalculateTransformDelta(screenPos, m_transformStartPos, axis, viewMatrix, projectionMatrix, camera);
    
    // Apply transform based on mode
    switch (m_transformMode)
    {
    case TransformMode::Position:
        selectedModel.transform.position.x += axis.x * delta;
        selectedModel.transform.position.y += axis.y * delta;
        selectedModel.transform.position.z += axis.z * delta;
        break;
        
    case TransformMode::Rotation:
        selectedModel.transform.rotation.x += axis.x * delta * 0.1f; // Scale rotation
        selectedModel.transform.rotation.y += axis.y * delta * 0.1f;
        selectedModel.transform.rotation.z += axis.z * delta * 0.1f;
        break;
        
    case TransformMode::Scale:
        selectedModel.transform.scale.x += axis.x * delta * 0.01f; // Scale factor
        selectedModel.transform.scale.y += axis.y * delta * 0.01f;
        selectedModel.transform.scale.z += axis.z * delta * 0.01f;
        
        // Clamp scale to prevent negative values
        selectedModel.transform.scale.x = std::max(0.1f, selectedModel.transform.scale.x);
        selectedModel.transform.scale.y = std::max(0.1f, selectedModel.transform.scale.y);
        selectedModel.transform.scale.z = std::max(0.1f, selectedModel.transform.scale.z);
        break;
    }
}

void SelectionManager::EndTransform()
{
    m_isTransforming = false;
    m_activeAxis = GizmoAxis::None;
}

GizmoAxis SelectionManager::GetGizmoAxis(const XMFLOAT2& screenPos, const XMMATRIX& viewMatrix,
                                         const XMMATRIX& projectionMatrix, const XMMATRIX& worldMatrix) const
{
    if (m_selectedModelIndex < 0)
        return GizmoAxis::None;
    
    // Simple axis detection based on screen position relative to gizmo center
    // This is a simplified version - in a real implementation you'd do proper ray-gizmo intersection
    
    
    if (screenPos.x > 0.5f && screenPos.y < 0.5f)
        return GizmoAxis::X;
    else if (screenPos.x < 0.5f && screenPos.y < 0.5f)
        return GizmoAxis::Y;
    else if (screenPos.x > 0.5f && screenPos.y > 0.5f)
        return GizmoAxis::Z;
    
    return GizmoAxis::None;
}

TransformData* SelectionManager::GetSelectedTransform(std::vector<ModelInstance>& models)
{
    if (m_selectedModelIndex >= 0 && m_selectedModelIndex < static_cast<int>(models.size()))
        return &models[m_selectedModelIndex].transform;
    return nullptr;
}

const TransformData* SelectionManager::GetSelectedTransform(const std::vector<ModelInstance>& models) const
{
    if (m_selectedModelIndex >= 0 && m_selectedModelIndex < static_cast<int>(models.size()))
        return &models[m_selectedModelIndex].transform;
    return nullptr;
}

void SelectionManager::RenderGizmos(D3D11Device* device, const XMMATRIX& viewMatrix, 
                                   const XMMATRIX& projectionMatrix, const XMMATRIX& worldMatrix)
{
    if (m_selectedModelIndex < 0 || m_transformMode == TransformMode::None)
        return;
    
    // Set up gizmo rendering states
    device->TurnOffCulling();
    device->TurnZBufferOff();
    
    // Render gizmo based on transform mode
    switch (m_transformMode)
    {
    case TransformMode::Position:
        RenderPositionGizmo(device, viewMatrix, projectionMatrix, worldMatrix);
        break;
    case TransformMode::Rotation:
        RenderRotationGizmo(device, viewMatrix, projectionMatrix, worldMatrix);
        break;
    case TransformMode::Scale:
        RenderScaleGizmo(device, viewMatrix, projectionMatrix, worldMatrix);
        break;
    }
    
    // Restore render states
    device->TurnOnCulling();
    device->TurnZBufferOn();
}





XMFLOAT3 SelectionManager::TransformPoint(const XMFLOAT3& point, const XMMATRIX& matrix) const
{
    XMFLOAT4 transformed;
    XMStoreFloat4(&transformed, XMVector4Transform(XMLoadFloat3(&point), matrix));
    return XMFLOAT3(transformed.x, transformed.y, transformed.z);
}

XMFLOAT3 SelectionManager::GetTransformAxis(GizmoAxis axis) const
{
    switch (axis)
    {
    case GizmoAxis::X: return XMFLOAT3(1.0f, 0.0f, 0.0f);
    case GizmoAxis::Y: return XMFLOAT3(0.0f, 1.0f, 0.0f);
    case GizmoAxis::Z: return XMFLOAT3(0.0f, 0.0f, 1.0f);
    default: return XMFLOAT3(0.0f, 0.0f, 0.0f);
    }
}

float SelectionManager::CalculateTransformDelta(const XMFLOAT2& currentPos, const XMFLOAT2& startPos,
                                               const XMFLOAT3& axis, const XMMATRIX& viewMatrix,
                                               const XMMATRIX& projectionMatrix, const Camera* camera) const
{
    // Calculate mouse movement delta
    float deltaX = currentPos.x - startPos.x;
    float deltaY = currentPos.y - startPos.y;
    
    // Project movement onto the transform axis
    float delta = deltaX * axis.x + deltaY * axis.y;
    
    // Scale based on camera distance and transform mode
    float scale = 1.0f;
    if (camera)
    {
        XMFLOAT3 cameraPos = camera->GetPosition();
        // Calculate distance-based scaling
        scale = 0.1f; // Base scale
    }
    
    return delta * scale;
}

void SelectionManager::CreateGizmoGeometry(D3D11Device* device)
{
    // Create simple gizmo geometry (arrows for position, arcs for rotation, lines with cubes for scale)
    // This is a simplified version - in a real implementation you'd create proper 3D geometry
    
    struct GizmoVertex
    {
        XMFLOAT3 position;
        XMFLOAT4 color;
    };
    

    std::vector<GizmoVertex> vertices;
    std::vector<unsigned int> indices;
    
    // X-axis arrow (red)
    vertices.push_back({XMFLOAT3(0, 0, 0), m_xAxisColor});
    vertices.push_back({XMFLOAT3(1, 0, 0), m_xAxisColor});
    
    // Y-axis arrow (green)
    vertices.push_back({XMFLOAT3(0, 0, 0), m_yAxisColor});
    vertices.push_back({XMFLOAT3(0, 1, 0), m_yAxisColor});
    
    // Z-axis arrow (blue)
    vertices.push_back({XMFLOAT3(0, 0, 0), m_zAxisColor});
    vertices.push_back({XMFLOAT3(0, 0, 1), m_zAxisColor});
    
    // Create indices
    for (unsigned int i = 0; i < vertices.size(); ++i)
    {
        indices.push_back(i);
    }
    
    m_gizmoVertexCount = static_cast<int>(vertices.size());
    m_gizmoIndexCount = static_cast<int>(indices.size());
    
    // Create vertex buffer
    D3D11_BUFFER_DESC vertexBufferDesc;
    vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexBufferDesc.ByteWidth = sizeof(GizmoVertex) * m_gizmoVertexCount;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.CPUAccessFlags = 0;
    vertexBufferDesc.MiscFlags = 0;
    vertexBufferDesc.StructureByteStride = 0;
    
    D3D11_SUBRESOURCE_DATA vertexData;
    vertexData.pSysMem = vertices.data();
    vertexData.SysMemPitch = 0;
    vertexData.SysMemSlicePitch = 0;
    
    HRESULT result = device->GetDevice()->CreateBuffer(&vertexBufferDesc, &vertexData, &m_gizmoVertexBuffer);
    if (FAILED(result))
    {
        LOG_ERROR("Failed to create gizmo vertex buffer");
        return;
    }
    
    // Create index buffer
    D3D11_BUFFER_DESC indexBufferDesc;
    indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    indexBufferDesc.ByteWidth = sizeof(unsigned int) * m_gizmoIndexCount;
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexBufferDesc.CPUAccessFlags = 0;
    indexBufferDesc.MiscFlags = 0;
    indexBufferDesc.StructureByteStride = 0;
    
    D3D11_SUBRESOURCE_DATA indexData;
    indexData.pSysMem = indices.data();
    indexData.SysMemPitch = 0;
    indexData.SysMemSlicePitch = 0;
    
    result = device->GetDevice()->CreateBuffer(&indexBufferDesc, &indexData, &m_gizmoIndexBuffer);
    if (FAILED(result))
    {
        LOG_ERROR("Failed to create gizmo index buffer");
        return;
    }
}

void SelectionManager::RenderPositionGizmo(D3D11Device* device, const XMMATRIX& viewMatrix, 
                                          const XMMATRIX& projectionMatrix, const XMMATRIX& worldMatrix)
{
    // Render position gizmo (arrows)
    // This would use a line shader to render the arrows

    UINT stride = sizeof(XMFLOAT3) + sizeof(XMFLOAT4); // position + color
    UINT offset = 0;
    
    device->GetDeviceContext()->IASetVertexBuffers(0, 1, &m_gizmoVertexBuffer, &stride, &offset);
    device->GetDeviceContext()->IASetIndexBuffer(m_gizmoIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
    device->GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
}

void SelectionManager::RenderRotationGizmo(D3D11Device* device, const XMMATRIX& viewMatrix, 
                                          const XMMATRIX& projectionMatrix, const XMMATRIX& worldMatrix)
{
    // Render rotation gizmo (arcs)
    // Similar to position gizmo but with arc geometry
    RenderPositionGizmo(device, viewMatrix, projectionMatrix, worldMatrix);
}

void SelectionManager::RenderScaleGizmo(D3D11Device* device, const XMMATRIX& viewMatrix, 
                                       const XMMATRIX& projectionMatrix, const XMMATRIX& worldMatrix)
{
    // Render scale gizmo (lines with cubes at the end)
    // Similar to position gizmo but with cube geometry at the ends
    RenderPositionGizmo(device, viewMatrix, projectionMatrix, worldMatrix);
} 