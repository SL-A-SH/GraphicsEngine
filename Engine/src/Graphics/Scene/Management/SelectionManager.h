#ifndef _SELECTIONMANAGER_H_
#define _SELECTIONMANAGER_H_

#include <d3d11.h>
#include <directxmath.h>
#include <vector>
#include <memory>
#include "../../Math/Frustum.h"

using namespace DirectX;

class Model;
class Camera;
class D3D11Device;

enum class TransformMode
{
    None,
    Position,
    Rotation,
    Scale
};

enum class GizmoAxis
{
    None,
    X,
    Y,
    Z
};

struct TransformData
{
    XMFLOAT3 position;
    XMFLOAT3 rotation;
    XMFLOAT3 scale;
    
    TransformData() : position(0, 0, 0), rotation(0, 0, 0), scale(1, 1, 1) {}
};

struct ModelInstance
{
    int modelIndex;
    TransformData transform;
    bool isSelected;
    
    ModelInstance() : modelIndex(-1), isSelected(false) {}
};

class SelectionManager
{
public:
    SelectionManager();
    ~SelectionManager();

    bool Initialize(D3D11Device* device);
    void Shutdown();

    // Selection management
    void SelectModel(int modelIndex);
    void DeselectAll();
    bool IsModelSelected(int modelIndex) const;
    int GetSelectedModelIndex() const { return m_selectedModelIndex; }
    
    // Transform mode
    void SetTransformMode(TransformMode mode) { m_transformMode = mode; }
    TransformMode GetTransformMode() const { return m_transformMode; }
    
    // Raycasting
    int PickModel(const XMFLOAT2& screenPos, const XMMATRIX& viewMatrix, 
                  const XMMATRIX& projectionMatrix, const std::vector<ModelInstance>& models,
                  const Model* modelTemplate, const Frustum* frustum, const Camera* camera);
    
    // Transform operations
    void StartTransform(const XMFLOAT2& screenPos, const XMMATRIX& viewMatrix, 
                       const XMMATRIX& projectionMatrix, const Camera* camera);
    void UpdateTransform(const XMFLOAT2& screenPos, const XMMATRIX& viewMatrix, 
                        const XMMATRIX& projectionMatrix, const Camera* camera,
                        std::vector<ModelInstance>& models);
    void EndTransform();
    
    // Gizmo interaction
    GizmoAxis GetGizmoAxis(const XMFLOAT2& screenPos, const XMMATRIX& viewMatrix,
                           const XMMATRIX& projectionMatrix, const XMMATRIX& worldMatrix) const;
    
    // Transform data access
    TransformData* GetSelectedTransform(std::vector<ModelInstance>& models);
    const TransformData* GetSelectedTransform(const std::vector<ModelInstance>& models) const;
    
    // Gizmo rendering
    void RenderGizmos(D3D11Device* device, const XMMATRIX& viewMatrix, 
                     const XMMATRIX& projectionMatrix, const XMMATRIX& worldMatrix);

private:
    // Raycasting helpers
    XMFLOAT3 ScreenToWorldRay(const XMFLOAT2& screenPos, const XMMATRIX& viewMatrix, 
                              const XMMATRIX& projectionMatrix) const;
    bool RayAABBIntersection(const XMFLOAT3& rayOrigin, const XMFLOAT3& rayDirection,
                             const XMFLOAT3& min, const XMFLOAT3& max, float& t) const;
    XMFLOAT3 TransformPoint(const XMFLOAT3& point, const XMMATRIX& matrix) const;
    
    // Transform helpers
    XMFLOAT3 GetTransformAxis(GizmoAxis axis) const;
    float CalculateTransformDelta(const XMFLOAT2& currentPos, const XMFLOAT2& startPos,
                                 const XMFLOAT3& axis, const XMMATRIX& viewMatrix,
                                 const XMMATRIX& projectionMatrix, const Camera* camera) const;
    
    // Gizmo geometry creation
    void CreateGizmoGeometry(D3D11Device* device);
    void RenderPositionGizmo(D3D11Device* device, const XMMATRIX& viewMatrix, 
                            const XMMATRIX& projectionMatrix, const XMMATRIX& worldMatrix);
    void RenderRotationGizmo(D3D11Device* device, const XMMATRIX& viewMatrix, 
                            const XMMATRIX& projectionMatrix, const XMMATRIX& worldMatrix);
    void RenderScaleGizmo(D3D11Device* device, const XMMATRIX& viewMatrix, 
                         const XMMATRIX& projectionMatrix, const XMMATRIX& worldMatrix);

private:
    int m_selectedModelIndex;
    TransformMode m_transformMode;
    GizmoAxis m_activeAxis;
    bool m_isTransforming;
    XMFLOAT2 m_transformStartPos;
    XMFLOAT3 m_transformStartValue;
    
    // Gizmo geometry (simple lines and shapes)
    ID3D11Buffer* m_gizmoVertexBuffer;
    ID3D11Buffer* m_gizmoIndexBuffer;
    int m_gizmoVertexCount;
    int m_gizmoIndexCount;
    
    // Gizmo colors
    XMFLOAT4 m_xAxisColor;
    XMFLOAT4 m_yAxisColor;
    XMFLOAT4 m_zAxisColor;
    XMFLOAT4 m_selectedAxisColor;
};

#endif 