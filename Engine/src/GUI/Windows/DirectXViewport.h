#pragma once

#include <QWidget>
#include <QTimer>
#include "../../Core/System/SystemManager.h"

class DirectXViewport : public QWidget
{
    Q_OBJECT

public:
    DirectXViewport(QWidget* parent = nullptr);
    ~DirectXViewport();
    void ToggleFullscreen();

protected:
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;
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
    SystemManager* m_SystemManager;
    QTimer* m_UpdateTimer;
    bool m_Initialized;

public:
    SystemManager* GetSystemManager() const { return m_SystemManager; }
}; 