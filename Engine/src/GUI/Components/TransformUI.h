#ifndef _TRANSFORMUI_H_
#define _TRANSFORMUI_H_

#include <d3d11.h>
#include <directxmath.h>
#include <QWidget>
#include <functional>
#include "../../Graphics/D3D11/D3D11Device.h"
#include "../../Graphics/Shaders/Management/ShaderManager.h"
#include "../../Graphics/Rendering/Font.h"
#include "../../Graphics/Resource/Text.h"
#include "../../Graphics/Rendering/Sprite.h"
#include "../../Graphics/Scene/Management/SelectionManager.h"

using namespace DirectX;

// Forward declarations for Qt MOC
class SelectionManager;
class QVBoxLayout;
class QHBoxLayout;
class QLabel;
class QLineEdit;
class QPushButton;
class QGroupBox;
class QGridLayout;
class QSignalMapper;
class QPropertyAnimation;
class QGraphicsOpacityEffect;

class TransformUI : public QWidget
{
    Q_OBJECT

public:
    TransformUI(QWidget* parent = nullptr);
    ~TransformUI();

    bool Initialize(D3D11Device* Direct3D, int screenHeight, int screenWidth);
    void Shutdown();
    bool Frame(ID3D11DeviceContext* deviceContext);
    bool Render(D3D11Device* Direct3D, ShaderManager* ShaderManager, XMMATRIX worldMatrix, XMMATRIX viewMatrix, XMMATRIX orthoMatrix);
    
    // Transform mode management
    void SetTransformMode(TransformMode mode);
    TransformMode GetTransformMode() const { return m_currentTransformMode; }
    
    // Update transform values from external source
    void UpdateTransformValues(const TransformData* transform);
    void SetTransformData(const TransformData& transform);
    void ClearTransformData();
    
    // Show/Hide UI
    void ShowUI();
    void HideUI();
    bool IsVisible() const { return isVisible(); }
    
    // Set selection manager reference
    void SetSelectionManager(SelectionManager* selectionManager) { m_selectionManager = selectionManager; }
    
    // Set callback functions
    void SetTransformModeChangedCallback(std::function<void(TransformMode)> callback);
    void SetTransformValuesChangedCallback(std::function<void(const TransformData&)> callback);

private slots:
    void OnTransformModeButtonClicked(int mode);
    void OnPositionValueChanged();
    void OnRotationValueChanged();
    void OnScaleValueChanged();

private:
    void InitializeUI();
    void CreateTransformModeButtons();
    void CreateTransformValueEditors();
    void CreateGizmoIcons(D3D11Device* Direct3D);
    void UpdateButtonStates();
    void UpdateValueEditors();
    
    // Gizmo icon rendering
    void RenderGizmoIcons(D3D11Device* Direct3D, ShaderManager* ShaderManager, 
                         XMMATRIX worldMatrix, XMMATRIX viewMatrix, XMMATRIX orthoMatrix);

private:
    // UI Components
    QVBoxLayout* m_mainLayout;
    QGroupBox* m_transformModeGroup;
    QGroupBox* m_transformValuesGroup;
    
    // Transform mode buttons
    QPushButton* m_positionButton;
    QPushButton* m_rotationButton;
    QPushButton* m_scaleButton;
    QSignalMapper* m_buttonMapper;
    
    // Transform value editors
    QLineEdit* m_positionXEdit;
    QLineEdit* m_positionYEdit;
    QLineEdit* m_positionZEdit;
    QLineEdit* m_rotationXEdit;
    QLineEdit* m_rotationYEdit;
    QLineEdit* m_rotationZEdit;
    QLineEdit* m_scaleXEdit;
    QLineEdit* m_scaleYEdit;
    QLineEdit* m_scaleZEdit;
    
    // Gizmo icons (rendered as sprites)
    Sprite* m_positionIcon;
    Sprite* m_rotationIcon;
    Sprite* m_scaleIcon;
    Text* m_positionIconText;
    Text* m_rotationIconText;
    Text* m_scaleIconText;
    
    // Current state
    TransformMode m_currentTransformMode;
    TransformData m_currentTransform;
    bool m_updatingFromExternal;
    
    // References
    SelectionManager* m_selectionManager;
    Font* m_Font;
    
    // Screen dimensions
    int m_screenWidth;
    int m_screenHeight;
    
    // Icon positions (bottom right corner)
    int m_iconSize;
    int m_iconSpacing;
    int m_iconMargin;
    
    // Callback functions
    std::function<void(TransformMode)> m_transformModeChangedCallback;
    std::function<void(const TransformData&)> m_transformValuesChangedCallback;
};

#endif 