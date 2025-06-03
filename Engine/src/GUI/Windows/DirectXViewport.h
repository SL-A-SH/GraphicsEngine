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

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;
    bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;

private slots:
    void updateFrame();

private:
    SystemManager* m_SystemManager;
    QTimer* m_UpdateTimer;
    bool m_Initialized;
}; 