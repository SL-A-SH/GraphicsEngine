#ifndef _DIRECTX_VIEWPORT_H_
#define _DIRECTX_VIEWPORT_H_

#ifndef NOMINMAX
#define NOMINMAX 1
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif

#include <QWidget>
#include "../../Core/System/SystemManager.h"
#include "../Components/TransformUI.h"
#include "../Components/ModelListUI.h"

// Forward declarations
class SystemManager;
class TransformUI;
class ModelListUI;
class MainWindow;
class QTimer;
class QResizeEvent;
class QShowEvent;
class QHideEvent;
class QKeyEvent;
class QMouseEvent;
class QFocusEvent;
class QEvent;
class QByteArray;

class DirectXViewport : public QWidget
{
    Q_OBJECT

public:
    DirectXViewport(QWidget* parent = nullptr, MainWindow* mainWindow = nullptr);
    ~DirectXViewport();
    void ToggleFullscreen();
    void SetTransformUI(TransformUI* transformUI);
    void SetModelListUI(ModelListUI* modelListUI);
    void SetBackgroundRendering(bool enabled);
    void SetupUISwitchingCallbacks();
    void ForceFocus();
    void HandleViewportClick();

    // Set background rendering mode (continues rendering even when not visible)
    bool IsBackgroundRendering() const { return m_BackgroundRendering; }

protected:
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;

    // Input event handlers
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    bool event(QEvent* event) override;

private slots:
    void updateFrame();

private:
    MainWindow* m_mainWindow;
    SystemManager* m_SystemManager;
    QTimer* m_UpdateTimer;
    bool m_Initialized;
    TransformUI* m_TransformUI;
    ModelListUI* m_ModelListUI;
    bool m_BackgroundRendering;

public:
    SystemManager* GetSystemManager() const { return m_SystemManager; }
};

#endif