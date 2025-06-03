#pragma once

#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QMenuBar>
#include <QToolBar>
#include <QDockWidget>
#include "DirectXViewport.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private:
    void CreateMenus();
    void CreateToolbars();
    void CreateDockWidgets();

    // DirectX viewport widget
    DirectXViewport* m_ViewportWidget;
    
    // Layout for the main window
    QVBoxLayout* m_MainLayout;
}; 