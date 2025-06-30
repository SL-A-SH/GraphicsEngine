#include "SelectionManager.h"
#include "../Resource/Model.h"
#include "../Rendering/Camera.h"
#include "../D3D11/D3D11Device.h"
#include "../../Core/System/Logger.h"
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

int SelectionManager::PickModel(const XMFLOAT2& screenPos, const XMMATRIX& viewMatrix, 
                               const XMMATRIX& projectionMatrix, const std::vector<ModelInstance>& models,
                               const Model* modelTemplate, const Frustum* frustum, const Camera* camera)
{
    LOG("PickModel called with screenPos: (" + std::to_string(screenPos.x) + ", " + std::to_string(screenPos.y) + ")");
    LOG("Number of models to test: " + std::to_string(models.size()));
    
    if (models.empty())
    {
        LOG("No models to pick from");
        return -1;
    }
    
    if (!modelTemplate)
    {
        LOG_ERROR("Model template is null");
        return -1;
    }
    
    if (!camera)
    {
        LOG_ERROR("Camera is null");
        return -1;
    }
    
    // Get camera position directly from camera object
    XMFLOAT3 rayOrigin = camera->GetPosition();
    
    LOG("Camera position calculation:");
    LOG("  Camera position: (" + std::to_string(rayOrigin.x) + ", " + std::to_string(rayOrigin.y) + ", " + std::to_string(rayOrigin.z) + ")");
    
    // Debug: Get camera's forward direction from view matrix
    XMFLOAT3 cameraForward;
    XMStoreFloat3(&cameraForward, XMVector3Transform(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), XMMatrixInverse(nullptr, viewMatrix)));
    float forwardLength = sqrt(cameraForward.x * cameraForward.x + cameraForward.y * cameraForward.y + cameraForward.z * cameraForward.z);
    if (forwardLength > 0.0f)
    {
        cameraForward.x /= forwardLength;
        cameraForward.y /= forwardLength;
        cameraForward.z /= forwardLength;
    }
    LOG("  Camera forward direction: (" + std::to_string(cameraForward.x) + ", " + std::to_string(cameraForward.y) + ", " + std::to_string(cameraForward.z) + ")");
    
    // Get ray direction from screen position
    XMFLOAT3 rayDirection = ScreenToWorldRay(screenPos, viewMatrix, projectionMatrix);
    
    LOG("Ray direction: (" + std::to_string(rayDirection.x) + ", " + std::to_string(rayDirection.y) + ", " + std::to_string(rayDirection.z) + ")");
    
    // Test: Check if ray direction makes sense
    // The camera should be looking in the positive Z direction by default
    // If the ray is pointing in negative Z, we need to flip it
    if (rayDirection.z < 0.0f)
    {
        LOG("WARNING: Ray direction is pointing backward, flipping Z component");
        rayDirection.z = -rayDirection.z;
        LOG("Corrected ray direction: (" + std::to_string(rayDirection.x) + ", " + std::to_string(rayDirection.y) + ", " + std::to_string(rayDirection.z) + ")");
    }
    
    // Test ray-AABB intersection with a simple case
    XMFLOAT3 testMin = XMFLOAT3(-1.0f, -1.0f, -1.0f);
    XMFLOAT3 testMax = XMFLOAT3(1.0f, 1.0f, 1.0f);
    XMFLOAT3 testOrigin = XMFLOAT3(0.0f, 0.0f, -5.0f);
    XMFLOAT3 testDirection = XMFLOAT3(0.0f, 0.0f, 1.0f);
    float testT;
    bool testIntersects = RayAABBIntersection(testOrigin, testDirection, testMin, testMax, testT);
    LOG("Test ray-AABB intersection: " + std::string(testIntersects ? "true" : "false") + ", t = " + std::to_string(testT));
    
    // Find closest intersection
    int closestModel = -1;
    float closestDistance = FLT_MAX;
    
    for (size_t i = 0; i < models.size(); ++i)
    {
        const ModelInstance& instance = models[i];
        
        // Get model's world position
        XMFLOAT3 worldPos = instance.transform.position;
        
        LOG("Testing model " + std::to_string(i) + " at position: (" + std::to_string(worldPos.x) + ", " + std::to_string(worldPos.y) + ", " + std::to_string(worldPos.z) + ")");
        
        // Get model's bounding box
        const Model::AABB& bbox = modelTemplate->GetBoundingBox();
        
        LOG("Model bounding box - min: (" + std::to_string(bbox.min.x) + ", " + std::to_string(bbox.min.y) + ", " + std::to_string(bbox.min.z) + ")");
        LOG("Model bounding box - max: (" + std::to_string(bbox.max.x) + ", " + std::to_string(bbox.max.y) + ", " + std::to_string(bbox.max.z) + ")");
        
        // Transform bounding box to world space
        XMFLOAT3 worldMin, worldMax;
        worldMin.x = bbox.min.x * instance.transform.scale.x + worldPos.x;
        worldMin.y = bbox.min.y * instance.transform.scale.y + worldPos.y;
        worldMin.z = bbox.min.z * instance.transform.scale.z + worldPos.z;
        worldMax.x = bbox.max.x * instance.transform.scale.x + worldPos.x;
        worldMax.y = bbox.max.y * instance.transform.scale.y + worldPos.y;
        worldMax.z = bbox.max.z * instance.transform.scale.z + worldPos.z;
        
        LOG("World bounding box - min: (" + std::to_string(worldMin.x) + ", " + std::to_string(worldMin.y) + ", " + std::to_string(worldMin.z) + ")");
        LOG("World bounding box - max: (" + std::to_string(worldMax.x) + ", " + std::to_string(worldMax.y) + ", " + std::to_string(worldMax.z) + ")");
        
        // Check if model is in frustum
        if (frustum)
        {
            bool inFrustum = frustum->CheckAABB(worldMin, worldMax);
            LOG("Model " + std::to_string(i) + " in frustum: " + std::string(inFrustum ? "true" : "false"));
            // Temporarily disable frustum culling for debugging
            // if (!inFrustum)
            //     continue;
        }
        
        // Check ray-AABB intersection
        float t;
        bool intersects = RayAABBIntersection(rayOrigin, rayDirection, worldMin, worldMax, t);
        LOG("Model " + std::to_string(i) + " ray intersection: " + std::string(intersects ? "true" : "false") + ", distance: " + std::to_string(t));
        
        // Only consider intersections in front of the camera (positive t values)
        if (intersects && t > 0.0f)
        {
            if (t < closestDistance)
            {
                closestDistance = t;
                closestModel = static_cast<int>(i);
                LOG("New closest model: " + std::to_string(i) + " at distance: " + std::to_string(t));
            }
        }
    }
    
    LOG("PickModel result: " + std::to_string(closestModel));
    return closestModel;
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
    
    // For now, return the axis based on which quadrant of the screen the mouse is in
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

XMFLOAT3 SelectionManager::ScreenToWorldRay(const XMFLOAT2& screenPos, const XMMATRIX& viewMatrix, 
                                           const XMMATRIX& projectionMatrix) const
{
    LOG("ScreenToWorldRay - input screenPos: (" + std::to_string(screenPos.x) + ", " + std::to_string(screenPos.y) + ")");
    
    // Convert screen coordinates to normalized device coordinates
    XMFLOAT2 ndc;
    ndc.x = (2.0f * screenPos.x) - 1.0f;
    ndc.y = 1.0f - (2.0f * screenPos.y);
    
    LOG("ScreenToWorldRay - NDC coordinates: (" + std::to_string(ndc.x) + ", " + std::to_string(ndc.y) + ")");
    
    // Create ray in clip space
    XMFLOAT4 rayClip = XMFLOAT4(ndc.x, ndc.y, -1.0f, 1.0f);
    
    // Transform to eye space
    XMMATRIX invProj = XMMatrixInverse(nullptr, projectionMatrix);
    XMFLOAT4 rayEye;
    XMStoreFloat4(&rayEye, XMVector4Transform(XMLoadFloat4(&rayClip), invProj));
    rayEye.z = -1.0f;
    rayEye.w = 0.0f;
    
    LOG("ScreenToWorldRay - eye space ray: (" + std::to_string(rayEye.x) + ", " + std::to_string(rayEye.y) + ", " + std::to_string(rayEye.z) + ", " + std::to_string(rayEye.w) + ")");
    
    // Transform to world space
    XMMATRIX invView = XMMatrixInverse(nullptr, viewMatrix);
    XMFLOAT4 rayWorld;
    XMStoreFloat4(&rayWorld, XMVector4Transform(XMLoadFloat4(&rayEye), invView));
    
    LOG("ScreenToWorldRay - world space ray: (" + std::to_string(rayWorld.x) + ", " + std::to_string(rayWorld.y) + ", " + std::to_string(rayWorld.z) + ", " + std::to_string(rayWorld.w) + ")");
    
    // Normalize the ray direction
    XMFLOAT3 rayDir = XMFLOAT3(rayWorld.x, rayWorld.y, rayWorld.z);
    float length = sqrt(rayDir.x * rayDir.x + rayDir.y * rayDir.y + rayDir.z * rayDir.z);
    
    if (length > 0.0f)
    {
        rayDir.x /= length;
        rayDir.y /= length;
        rayDir.z /= length;
    }
    else
    {
        LOG_ERROR("ScreenToWorldRay - ray direction length is zero");
        rayDir = XMFLOAT3(0.0f, 0.0f, 1.0f); // Default direction (forward)
    }
    
    LOG("ScreenToWorldRay - normalized ray direction: (" + std::to_string(rayDir.x) + ", " + std::to_string(rayDir.y) + ", " + std::to_string(rayDir.z) + ")");
    
    return rayDir;
}

bool SelectionManager::RayAABBIntersection(const XMFLOAT3& rayOrigin, const XMFLOAT3& rayDirection,
                                          const XMFLOAT3& min, const XMFLOAT3& max, float& t) const
{
    LOG("RayAABBIntersection - rayOrigin: (" + std::to_string(rayOrigin.x) + ", " + std::to_string(rayOrigin.y) + ", " + std::to_string(rayOrigin.z) + ")");
    LOG("RayAABBIntersection - rayDirection: (" + std::to_string(rayDirection.x) + ", " + std::to_string(rayDirection.y) + ", " + std::to_string(rayDirection.z) + ")");
    LOG("RayAABBIntersection - AABB min: (" + std::to_string(min.x) + ", " + std::to_string(min.y) + ", " + std::to_string(min.z) + ")");
    LOG("RayAABBIntersection - AABB max: (" + std::to_string(max.x) + ", " + std::to_string(max.y) + ", " + std::to_string(max.z) + ")");
    
    // Check for division by zero
    if (rayDirection.x == 0.0f && rayDirection.y == 0.0f && rayDirection.z == 0.0f)
    {
        LOG_ERROR("Ray direction is zero vector");
        return false;
    }
    
    float tMin, tMax;
    
    // X-axis
    if (rayDirection.x != 0.0f)
    {
        tMin = (min.x - rayOrigin.x) / rayDirection.x;
        tMax = (max.x - rayOrigin.x) / rayDirection.x;
        if (tMin > tMax) std::swap(tMin, tMax);
    }
    else
    {
        // Ray is parallel to X-axis
        if (rayOrigin.x < min.x || rayOrigin.x > max.x)
        {
            LOG("RayAABBIntersection - no intersection (ray parallel to X-axis, outside bounds)");
            return false;
        }
        tMin = -FLT_MAX;
        tMax = FLT_MAX;
    }
    
    // Y-axis
    float tyMin, tyMax;
    if (rayDirection.y != 0.0f)
    {
        tyMin = (min.y - rayOrigin.y) / rayDirection.y;
        tyMax = (max.y - rayOrigin.y) / rayDirection.y;
        if (tyMin > tyMax) std::swap(tyMin, tyMax);
    }
    else
    {
        // Ray is parallel to Y-axis
        if (rayOrigin.y < min.y || rayOrigin.y > max.y)
        {
            LOG("RayAABBIntersection - no intersection (ray parallel to Y-axis, outside bounds)");
            return false;
        }
        tyMin = -FLT_MAX;
        tyMax = FLT_MAX;
    }
    
    if (tMin > tyMax || tyMin > tMax)
    {
        LOG("RayAABBIntersection - no intersection (X/Y planes)");
        return false;
    }
    
    if (tyMin > tMin) tMin = tyMin;
    if (tyMax < tMax) tMax = tyMax;
    
    // Z-axis
    float tzMin, tzMax;
    if (rayDirection.z != 0.0f)
    {
        tzMin = (min.z - rayOrigin.z) / rayDirection.z;
        tzMax = (max.z - rayOrigin.z) / rayDirection.z;
        if (tzMin > tzMax) std::swap(tzMin, tzMax);
    }
    else
    {
        // Ray is parallel to Z-axis
        if (rayOrigin.z < min.z || rayOrigin.z > max.z)
        {
            LOG("RayAABBIntersection - no intersection (ray parallel to Z-axis, outside bounds)");
            return false;
        }
        tzMin = -FLT_MAX;
        tzMax = FLT_MAX;
    }
    
    if (tMin > tzMax || tzMin > tMax)
    {
        LOG("RayAABBIntersection - no intersection (Z plane)");
        return false;
    }
    
    if (tzMin > tMin) tMin = tzMin;
    if (tzMax < tMax) tMax = tzMax;
    
    t = tMin;
    LOG("RayAABBIntersection - intersection found at t = " + std::to_string(t));
    return true;
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
    
    // For now, we'll create simple line geometry
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
    // For now, we'll just set up the buffers
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