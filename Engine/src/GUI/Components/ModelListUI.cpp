#include "ModelListUI.h"
#include "../../Core/System/Logger.h"
#include <QGroupBox>
#include <QPushButton>
#include <QListWidget>
#include <QListWidgetItem>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

ModelListUI::ModelListUI(QWidget* parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_modelListGroup(nullptr)
    , m_modelListWidget(nullptr)
    , m_deselectButton(nullptr)
    , m_modelList(nullptr)
    , m_selectionManager(nullptr)
    , m_Font(nullptr)
    , m_screenWidth(0)
    , m_screenHeight(0)
{
    InitializeUI();
    LOG("ModelListUI created");
}

ModelListUI::~ModelListUI()
{
    Shutdown();
}

bool ModelListUI::Initialize(D3D11Device* Direct3D, int screenHeight, int screenWidth)
{
    LOG("ModelListUI: Initializing Direct3D components");
    
    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;
    
    // Create font for text rendering
    if (!m_Font)
    {
        m_Font = new Font();
        if (!m_Font->Initialize(Direct3D->GetDevice(), Direct3D->GetDeviceContext(), 32))
        {
            LOG("ModelListUI: Failed to initialize font");
            return false;
        }
    }
    
    LOG("ModelListUI: Direct3D components initialized successfully");
    return true;
}

void ModelListUI::Shutdown()
{
    LOG("ModelListUI: Shutting down");
    
    if (m_Font)
    {
        m_Font->Shutdown();
        delete m_Font;
        m_Font = nullptr;
    }
}

bool ModelListUI::Frame(ID3D11DeviceContext* deviceContext)
{
    return true;
}

bool ModelListUI::Render(D3D11Device* Direct3D, ShaderManager* ShaderManager, XMMATRIX worldMatrix, XMMATRIX viewMatrix, XMMATRIX orthoMatrix)
{
    // This UI is primarily Qt-based, so minimal Direct3D rendering needed
    return true;
}

void ModelListUI::UpdateModelList(ModelList* modelList)
{
    LOG("ModelListUI: UpdateModelList called");
    
    if (!modelList)
    {
        LOG_ERROR("ModelListUI: UpdateModelList called with null ModelList!");
        return;
    }
    
    m_modelList = modelList;
    

    int modelCount = m_modelList->GetModelCount();
    LOG("ModelListUI: ModelList passed has " + std::to_string(modelCount) + " models");
    
    GenerateModelNames();
    UpdateModelListItems();
}

void ModelListUI::ShowUI()
{
    LOG("ModelListUI: Showing UI");
    show();
}

void ModelListUI::HideUI()
{
    LOG("ModelListUI: Hiding UI");
    hide();
}

void ModelListUI::SetModelSelectedCallback(std::function<void(int)> callback)
{
    m_modelSelectedCallback = callback;
}

void ModelListUI::SetModelDeselectedCallback(std::function<void()> callback)
{
    m_modelDeselectedCallback = callback;
}

void ModelListUI::InitializeUI()
{
    LOG("ModelListUI: Initializing UI");
    
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(10);
    m_mainLayout->setContentsMargins(5, 5, 5, 5);
    
    // Create model list widget
    CreateModelListWidget();
}

void ModelListUI::CreateModelListWidget()
{
    m_modelListGroup = new QGroupBox("Models in Scene", this);
    QVBoxLayout* groupLayout = new QVBoxLayout(m_modelListGroup);
    
    // Create list widget
    m_modelListWidget = new QListWidget(m_modelListGroup);
    m_modelListWidget->setMinimumHeight(200);
    m_modelListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    
    // Disable keyboard navigation to prevent interference with DirectX viewport
    m_modelListWidget->setFocusPolicy(Qt::NoFocus);
    m_modelListWidget->setTabOrder(m_modelListWidget, nullptr);
    
    // Create deselect button
    m_deselectButton = new QPushButton("Deselect All", m_modelListGroup);
    m_deselectButton->setEnabled(false); // Disabled by default since nothing is selected
    
    // Add widgets to layout
    groupLayout->addWidget(m_modelListWidget);
    groupLayout->addWidget(m_deselectButton);
    
    // Connect signals
    connect(m_modelListWidget, &QListWidget::itemClicked, this, &ModelListUI::OnModelItemClicked);
    connect(m_deselectButton, &QPushButton::clicked, this, &ModelListUI::OnDeselectButtonClicked);
    
    m_mainLayout->addWidget(m_modelListGroup);
}

void ModelListUI::UpdateModelListItems()
{
    if (!m_modelList)
    {
        LOG_ERROR("ModelListUI: No model list available for update");
        return;
    }
    
    LOG("ModelListUI: Updating model list items");
    
    m_modelListWidget->clear();
    
    int modelCount = m_modelList->GetModelCount();
    LOG("ModelListUI: Model count from ModelList: " + std::to_string(modelCount));
    

    if (modelCount <= 0)
    {
        LOG_ERROR("ModelListUI: ModelList reports 0 models - this is the problem!");
        return;
    }
    
    // Ensure we have enough model names
    if (m_modelNames.size() < modelCount)
    {
        LOG("ModelListUI: Regenerating model names for " + std::to_string(modelCount) + " models");
        GenerateModelNames();
    }
    
    LOG("ModelListUI: Adding " + std::to_string(modelCount) + " models to list widget");
    
    for (int i = 0; i < modelCount; ++i)
    {
        QString itemText;
        if (i < static_cast<int>(m_modelNames.size()))
        {
            itemText = QString::fromStdString(m_modelNames[i]);
        }
        else
        {
            itemText = QString("Spaceship %1").arg(i + 1);
        }
        
        QListWidgetItem* item = new QListWidgetItem(itemText);
        item->setData(Qt::UserRole, i); // Store model index
        m_modelListWidget->addItem(item);
    }
    
    LOG("ModelListUI: Successfully added " + std::to_string(modelCount) + " models to list");
    

    int actualItemCount = m_modelListWidget->count();
    LOG("ModelListUI: List widget now contains " + std::to_string(actualItemCount) + " items");
    

    m_modelListWidget->update();
    m_modelListWidget->repaint();
}

void ModelListUI::GenerateModelNames()
{
    if (!m_modelList)
        return;
    
    m_modelNames.clear();
    int modelCount = m_modelList->GetModelCount();
    
    for (int i = 0; i < modelCount; ++i)
    {
        std::string modelName = "Spaceship " + std::to_string(i + 1);
        m_modelNames.push_back(modelName);
    }
    
    LOG("ModelListUI: Generated " + std::to_string(modelCount) + " model names");
}

void ModelListUI::OnModelItemClicked(QListWidgetItem* item)
{
    if (!item)
        return;
    
    int modelIndex = item->data(Qt::UserRole).toInt();
    LOG("ModelListUI: Model " + std::to_string(modelIndex) + " clicked");
    
    // Enable deselect button
    m_deselectButton->setEnabled(true);
    
    // Call callback if set
    if (m_modelSelectedCallback)
    {
        m_modelSelectedCallback(modelIndex);
    }
}

void ModelListUI::OnDeselectButtonClicked()
{
    LOG("ModelListUI: Deselect button clicked");
    
    // Clear selection in list widget
    m_modelListWidget->clearSelection();
    
    // Disable deselect button
    m_deselectButton->setEnabled(false);
    
    // Call callback if set
    if (m_modelDeselectedCallback)
    {
        m_modelDeselectedCallback();
    }
}

void ModelListUI::focusOutEvent(QFocusEvent* event)
{
    LOG("ModelListUI: Lost focus");
    
    // When ModelListUI loses focus, ensure the DirectX viewport gets focus
    if (parentWidget())
    {
        // Find the DirectX viewport in the parent hierarchy
        QWidget* viewport = parentWidget()->findChild<QWidget*>("DirectXViewport");
        if (viewport)
        {
            LOG("ModelListUI: Transferring focus to DirectX viewport");
            viewport->setFocus();
        }
    }
    
    QWidget::focusOutEvent(event);
} 