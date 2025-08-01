#include "TransformUI.h"
#include "../../Core/System/Logger.h"
#include "../../Core/Common/EngineTypes.h"
#include <QGroupBox>
#include <QPushButton>
#include <QLineEdit>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QButtonGroup>
#include <QSignalMapper>

TransformUI::TransformUI(QWidget* parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_transformModeGroup(nullptr)
    , m_transformValuesGroup(nullptr)
    , m_positionButton(nullptr)
    , m_rotationButton(nullptr)
    , m_scaleButton(nullptr)
    , m_buttonMapper(nullptr)
    , m_positionXEdit(nullptr)
    , m_positionYEdit(nullptr)
    , m_positionZEdit(nullptr)
    , m_rotationXEdit(nullptr)
    , m_rotationYEdit(nullptr)
    , m_rotationZEdit(nullptr)
    , m_scaleXEdit(nullptr)
    , m_scaleYEdit(nullptr)
    , m_scaleZEdit(nullptr)
    , m_positionIcon(nullptr)
    , m_rotationIcon(nullptr)
    , m_scaleIcon(nullptr)
    , m_positionIconText(nullptr)
    , m_rotationIconText(nullptr)
    , m_scaleIconText(nullptr)
    , m_currentTransformMode(TransformMode::Position)
    , m_updatingFromExternal(false)
    , m_selectionManager(nullptr)
    , m_Font(nullptr)
    , m_screenWidth(0)
    , m_screenHeight(0)
    , m_iconSize(32)
    , m_iconSpacing(10)
    , m_iconMargin(20)
{
    InitializeUI();
    HideUI(); // Hide by default
    LOG("TransformUI created (Direct3D components will be initialized later)");
}

TransformUI::~TransformUI()
{
    Shutdown();
}

bool TransformUI::Initialize(D3D11Device* Direct3D, int screenHeight, int screenWidth)
{
    LOG("TransformUI: Initializing Direct3D components");
    
    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;
    
    // Create font for icon text
    if (!m_Font)
    {
        m_Font = new Font();
        if (!m_Font->Initialize(Direct3D->GetDevice(), Direct3D->GetDeviceContext(), 32))
        {
            LOG("TransformUI: Failed to initialize font");
            return false;
        }
    }
    
    // Create gizmo icons (only if they don't exist)
    if (!m_positionIcon && !m_rotationIcon && !m_scaleIcon)
    {
        CreateGizmoIcons(Direct3D);
    }
    
    LOG("TransformUI: Direct3D components initialized successfully");
    return true;
}

void TransformUI::Shutdown()
{
    LOG("TransformUI: Shutting down");
    
    if (m_Font)
    {
        m_Font->Shutdown();
        delete m_Font;
        m_Font = nullptr;
    }
    
    if (m_positionIcon)
    {
        m_positionIcon->Shutdown();
        delete m_positionIcon;
        m_positionIcon = nullptr;
    }
    
    if (m_rotationIcon)
    {
        m_rotationIcon->Shutdown();
        delete m_rotationIcon;
        m_rotationIcon = nullptr;
    }
    
    if (m_scaleIcon)
    {
        m_scaleIcon->Shutdown();
        delete m_scaleIcon;
        m_scaleIcon = nullptr;
    }
    
    if (m_positionIconText)
    {
        m_positionIconText->Shutdown();
        delete m_positionIconText;
        m_positionIconText = nullptr;
    }
    
    if (m_rotationIconText)
    {
        m_rotationIconText->Shutdown();
        delete m_rotationIconText;
        m_rotationIconText = nullptr;
    }
    
    if (m_scaleIconText)
    {
        m_scaleIconText->Shutdown();
        delete m_scaleIconText;
        m_scaleIconText = nullptr;
    }
}

bool TransformUI::Frame(ID3D11DeviceContext* deviceContext)
{
    return true;
}

bool TransformUI::Render(D3D11Device* Direct3D, ShaderManager* ShaderManager, XMMATRIX worldMatrix, XMMATRIX viewMatrix, XMMATRIX orthoMatrix)
{
    // Render gizmo icons
    RenderGizmoIcons(Direct3D, ShaderManager, worldMatrix, viewMatrix, orthoMatrix);
    
    return true;
}

void TransformUI::SetTransformMode(TransformMode mode)
{
    LOG("TransformUI: Setting transform mode to " + std::to_string(static_cast<int>(mode)));
    
    m_currentTransformMode = mode;
    UpdateButtonStates();
    
    // Call callback if set
    if (m_transformModeChangedCallback)
    {
        m_transformModeChangedCallback(mode);
    }
}

void TransformUI::UpdateTransformValues(const TransformData* transform)
{
    if (!transform)
        return;
    
    LOG("TransformUI: Updating transform values from external source");
    
    m_updatingFromExternal = true;
    m_currentTransform = *transform;
    UpdateValueEditors();
    m_updatingFromExternal = false;
}

void TransformUI::SetTransformData(const TransformData& transform)
{
    LOG("TransformUI: Setting transform data for selected model");
    
    m_updatingFromExternal = true;
    m_currentTransform = transform;
    UpdateValueEditors();
    m_updatingFromExternal = false;
}

void TransformUI::ClearTransformData()
{
    LOG("TransformUI: Clearing transform data");
    
    m_updatingFromExternal = true;
    m_currentTransform.position = XMFLOAT3(0.0f, 0.0f, 0.0f);
    m_currentTransform.rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
    m_currentTransform.scale = XMFLOAT3(1.0f, 1.0f, 1.0f);
    UpdateValueEditors();
    m_updatingFromExternal = false;
}

void TransformUI::InitializeUI()
{
    LOG("TransformUI: Initializing UI");
    
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(10);
    m_mainLayout->setContentsMargins(5, 5, 5, 5);
    
    // Create transform mode buttons
    CreateTransformModeButtons();
    
    // Create transform value editors
    CreateTransformValueEditors();
    
    // Set initial state
    m_positionButton->setChecked(true);
    UpdateValueEditors();
}

void TransformUI::CreateTransformModeButtons()
{
    m_transformModeGroup = new QGroupBox("Transform Mode", this);
    QHBoxLayout* buttonLayout = new QHBoxLayout(m_transformModeGroup);
    
    // Create buttons
    m_positionButton = new QPushButton("Position", m_transformModeGroup);
    m_rotationButton = new QPushButton("Rotation", m_transformModeGroup);
    m_scaleButton = new QPushButton("Scale", m_transformModeGroup);
    
    // Set button properties
    m_positionButton->setCheckable(true);
    m_rotationButton->setCheckable(true);
    m_scaleButton->setCheckable(true);
    
    // Create button group for exclusive selection
    QButtonGroup* buttonGroup = new QButtonGroup(this);
    buttonGroup->addButton(m_positionButton);
    buttonGroup->addButton(m_rotationButton);
    buttonGroup->addButton(m_scaleButton);
    buttonGroup->setExclusive(true);
    
    // Add buttons to layout
    buttonLayout->addWidget(m_positionButton);
    buttonLayout->addWidget(m_rotationButton);
    buttonLayout->addWidget(m_scaleButton);
    
    // Connect signals
    connect(m_positionButton, &QPushButton::clicked, [this]() { OnTransformModeButtonClicked(static_cast<int>(TransformMode::Position)); });
    connect(m_rotationButton, &QPushButton::clicked, [this]() { OnTransformModeButtonClicked(static_cast<int>(TransformMode::Rotation)); });
    connect(m_scaleButton, &QPushButton::clicked, [this]() { OnTransformModeButtonClicked(static_cast<int>(TransformMode::Scale)); });
    
    m_mainLayout->addWidget(m_transformModeGroup);
}

void TransformUI::CreateTransformValueEditors()
{
    m_transformValuesGroup = new QGroupBox("Transform Values", this);
    QGridLayout* gridLayout = new QGridLayout(m_transformValuesGroup);
    
    // Position editors
    gridLayout->addWidget(new QLabel("Position X:"), 0, 0);
    m_positionXEdit = new QLineEdit("0.0", m_transformValuesGroup);
    gridLayout->addWidget(m_positionXEdit, 0, 1);
    
    gridLayout->addWidget(new QLabel("Position Y:"), 1, 0);
    m_positionYEdit = new QLineEdit("0.0", m_transformValuesGroup);
    gridLayout->addWidget(m_positionYEdit, 1, 1);
    
    gridLayout->addWidget(new QLabel("Position Z:"), 2, 0);
    m_positionZEdit = new QLineEdit("0.0", m_transformValuesGroup);
    gridLayout->addWidget(m_positionZEdit, 2, 1);
    
    // Rotation editors
    gridLayout->addWidget(new QLabel("Rotation X:"), 3, 0);
    m_rotationXEdit = new QLineEdit("0.0", m_transformValuesGroup);
    gridLayout->addWidget(m_rotationXEdit, 3, 1);
    
    gridLayout->addWidget(new QLabel("Rotation Y:"), 4, 0);
    m_rotationYEdit = new QLineEdit("0.0", m_transformValuesGroup);
    gridLayout->addWidget(m_rotationYEdit, 4, 1);
    
    gridLayout->addWidget(new QLabel("Rotation Z:"), 5, 0);
    m_rotationZEdit = new QLineEdit("0.0", m_transformValuesGroup);
    gridLayout->addWidget(m_rotationZEdit, 5, 1);
    
    // Scale editors
    gridLayout->addWidget(new QLabel("Scale X:"), 6, 0);
    m_scaleXEdit = new QLineEdit("1.0", m_transformValuesGroup);
    gridLayout->addWidget(m_scaleXEdit, 6, 1);
    
    gridLayout->addWidget(new QLabel("Scale Y:"), 7, 0);
    m_scaleYEdit = new QLineEdit("1.0", m_transformValuesGroup);
    gridLayout->addWidget(m_scaleYEdit, 7, 1);
    
    gridLayout->addWidget(new QLabel("Scale Z:"), 8, 0);
    m_scaleZEdit = new QLineEdit("1.0", m_transformValuesGroup);
    gridLayout->addWidget(m_scaleZEdit, 8, 1);
    
    // Connect value change signals
    connect(m_positionXEdit, &QLineEdit::editingFinished, [this]() { OnPositionValueChanged(); });
    connect(m_positionYEdit, &QLineEdit::editingFinished, [this]() { OnPositionValueChanged(); });
    connect(m_positionZEdit, &QLineEdit::editingFinished, [this]() { OnPositionValueChanged(); });
    connect(m_rotationXEdit, &QLineEdit::editingFinished, [this]() { OnRotationValueChanged(); });
    connect(m_rotationYEdit, &QLineEdit::editingFinished, [this]() { OnRotationValueChanged(); });
    connect(m_rotationZEdit, &QLineEdit::editingFinished, [this]() { OnRotationValueChanged(); });
    connect(m_scaleXEdit, &QLineEdit::editingFinished, [this]() { OnScaleValueChanged(); });
    connect(m_scaleYEdit, &QLineEdit::editingFinished, [this]() { OnScaleValueChanged(); });
    connect(m_scaleZEdit, &QLineEdit::editingFinished, [this]() { OnScaleValueChanged(); });
    
    m_mainLayout->addWidget(m_transformValuesGroup);
}

void TransformUI::CreateGizmoIcons(D3D11Device* Direct3D)
{
    LOG("TransformUI: Creating gizmo icons");
    
    
    // In a real implementation, you would create proper icon textures
    LOG("TransformUI: Skipping sprite creation (texture files not available)");
    
    // Create text for icon labels
    if (!m_positionIconText)
    {
        m_positionIconText = new Text();
        m_positionIconText->Initialize(Direct3D->GetDevice(), Direct3D->GetDeviceContext(), m_screenWidth, m_screenHeight, 32, m_Font, (char*)"Position", 10, 10, 1.0f, 1.0f, 1.0f);
    }
    
    if (!m_rotationIconText)
    {
        m_rotationIconText = new Text();
        m_rotationIconText->Initialize(Direct3D->GetDevice(), Direct3D->GetDeviceContext(), m_screenWidth, m_screenHeight, 32, m_Font, (char*)"Rotation", 10, 10, 1.0f, 1.0f, 1.0f);
    }
    
    if (!m_scaleIconText)
    {
        m_scaleIconText = new Text();
        m_scaleIconText->Initialize(Direct3D->GetDevice(), Direct3D->GetDeviceContext(), m_screenWidth, m_screenHeight, 32, m_Font, (char*)"Scale", 10, 10, 1.0f, 1.0f, 1.0f);
    }
    
    LOG("TransformUI: Gizmo icons created successfully");
}

void TransformUI::UpdateButtonStates()
{
    // Update button checked states based on current mode
    m_positionButton->setChecked(m_currentTransformMode == TransformMode::Position);
    m_rotationButton->setChecked(m_currentTransformMode == TransformMode::Rotation);
    m_scaleButton->setChecked(m_currentTransformMode == TransformMode::Scale);
}

void TransformUI::UpdateValueEditors()
{
    // Update position editors
    m_positionXEdit->setText(QString::number(m_currentTransform.position.x, 'f', 3));
    m_positionYEdit->setText(QString::number(m_currentTransform.position.y, 'f', 3));
    m_positionZEdit->setText(QString::number(m_currentTransform.position.z, 'f', 3));
    
    // Update rotation editors
    m_rotationXEdit->setText(QString::number(m_currentTransform.rotation.x, 'f', 3));
    m_rotationYEdit->setText(QString::number(m_currentTransform.rotation.y, 'f', 3));
    m_rotationZEdit->setText(QString::number(m_currentTransform.rotation.z, 'f', 3));
    
    // Update scale editors
    m_scaleXEdit->setText(QString::number(m_currentTransform.scale.x, 'f', 3));
    m_scaleYEdit->setText(QString::number(m_currentTransform.scale.y, 'f', 3));
    m_scaleZEdit->setText(QString::number(m_currentTransform.scale.z, 'f', 3));
}

void TransformUI::RenderGizmoIcons(D3D11Device* Direct3D, ShaderManager* ShaderManager, 
                                  XMMATRIX worldMatrix, XMMATRIX viewMatrix, XMMATRIX orthoMatrix)
{
    // Calculate icon positions (bottom right corner)
    int startX = m_screenWidth - m_iconMargin - m_iconSize;
    int startY = m_screenHeight - m_iconMargin - m_iconSize * 3 - m_iconSpacing * 2;
    
    
    if (m_positionIcon)
    {
        m_positionIcon->Render(Direct3D->GetDeviceContext());
    }
    
    
    if (m_rotationIcon)
    {
        m_rotationIcon->Render(Direct3D->GetDeviceContext());
    }
    
    
    if (m_scaleIcon)
    {
        m_scaleIcon->Render(Direct3D->GetDeviceContext());
    }
    
    
    if (m_positionIconText)
    {
        m_positionIconText->Render(Direct3D->GetDeviceContext());
    }
    
    if (m_rotationIconText)
    {
        m_rotationIconText->Render(Direct3D->GetDeviceContext());
    }
    
    if (m_scaleIconText)
    {
        m_scaleIconText->Render(Direct3D->GetDeviceContext());
    }
}

void TransformUI::OnTransformModeButtonClicked(int mode)
{
    LOG("TransformUI: Transform mode button clicked: " + std::to_string(mode));
    
    TransformMode newMode = static_cast<TransformMode>(mode);
    SetTransformMode(newMode);
}

void TransformUI::OnPositionValueChanged()
{
    if (m_updatingFromExternal)
        return;
    
    LOG("TransformUI: Position values changed");
    
    // Update current transform from UI values
    m_currentTransform.position.x = m_positionXEdit->text().toFloat();
    m_currentTransform.position.y = m_positionYEdit->text().toFloat();
    m_currentTransform.position.z = m_positionZEdit->text().toFloat();
    
    // Call callback if set
    if (m_transformValuesChangedCallback)
    {
        m_transformValuesChangedCallback(m_currentTransform);
    }
}

void TransformUI::OnRotationValueChanged()
{
    if (m_updatingFromExternal)
        return;
    
    LOG("TransformUI: Rotation values changed");
    
    // Update current transform from UI values
    m_currentTransform.rotation.x = m_rotationXEdit->text().toFloat();
    m_currentTransform.rotation.y = m_rotationYEdit->text().toFloat();
    m_currentTransform.rotation.z = m_rotationZEdit->text().toFloat();
    
    // Call callback if set
    if (m_transformValuesChangedCallback)
    {
        m_transformValuesChangedCallback(m_currentTransform);
    }
}

void TransformUI::OnScaleValueChanged()
{
    if (m_updatingFromExternal)
        return;
    
    LOG("TransformUI: Scale values changed");
    
    // Update current transform from UI values
    m_currentTransform.scale.x = m_scaleXEdit->text().toFloat();
    m_currentTransform.scale.y = m_scaleYEdit->text().toFloat();
    m_currentTransform.scale.z = m_scaleZEdit->text().toFloat();
    
    // Call callback if set
    if (m_transformValuesChangedCallback)
    {
        m_transformValuesChangedCallback(m_currentTransform);
    }
}

void TransformUI::SetTransformModeChangedCallback(std::function<void(TransformMode)> callback)
{
    m_transformModeChangedCallback = callback;
}

void TransformUI::SetTransformValuesChangedCallback(std::function<void(const TransformData&)> callback)
{
    m_transformValuesChangedCallback = callback;
}

void TransformUI::ShowUI()
{
    LOG("TransformUI: Showing UI");
    show();
}

void TransformUI::HideUI()
{
    LOG("TransformUI: Hiding UI");
    hide();
} 