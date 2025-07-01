#ifndef _MODELLISTUI_H_
#define _MODELLISTUI_H_

#include <d3d11.h>
#include <directxmath.h>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QGroupBox>
#include <functional>
#include "../D3D11/D3D11Device.h"
#include "../Shaders/ShaderManager.h"
#include "../Rendering/Font.h"
#include "../Resource/Text.h"
#include "../Scene/SelectionManager.h"
#include "../Scene/ModelList.h"

using namespace DirectX;

// Forward declarations
class SelectionManager;
class ModelList;

class ModelListUI : public QWidget
{
    Q_OBJECT

public:
    ModelListUI(QWidget* parent = nullptr);
    ~ModelListUI();

    bool Initialize(D3D11Device* Direct3D, int screenHeight, int screenWidth);
    void Shutdown();
    bool Frame(ID3D11DeviceContext* deviceContext);
    bool Render(D3D11Device* Direct3D, ShaderManager* ShaderManager, XMMATRIX worldMatrix, XMMATRIX viewMatrix, XMMATRIX orthoMatrix);
    
    // Model list management
    void UpdateModelList(ModelList* modelList);
    void SetSelectionManager(SelectionManager* selectionManager) { m_selectionManager = selectionManager; }
    
    // Show/Hide UI
    void ShowUI();
    void HideUI();
    bool IsVisible() const { return isVisible(); }
    
    // Set callback functions
    void SetModelSelectedCallback(std::function<void(int)> callback);
    void SetModelDeselectedCallback(std::function<void()> callback);

private slots:
    void OnModelItemClicked(QListWidgetItem* item);
    void OnDeselectButtonClicked();

private:
    void InitializeUI();
    void CreateModelListWidget();
    void UpdateModelListItems();
    void GenerateModelNames();

private:
    // UI Components
    QVBoxLayout* m_mainLayout;
    QGroupBox* m_modelListGroup;
    QListWidget* m_modelListWidget;
    QPushButton* m_deselectButton;
    
    // Model data
    ModelList* m_modelList;
    std::vector<std::string> m_modelNames;
    
    // References
    SelectionManager* m_selectionManager;
    Font* m_Font;
    
    // Screen dimensions
    int m_screenWidth;
    int m_screenHeight;
    
    // Callback functions
    std::function<void(int)> m_modelSelectedCallback;
    std::function<void()> m_modelDeselectedCallback;
};

#endif 