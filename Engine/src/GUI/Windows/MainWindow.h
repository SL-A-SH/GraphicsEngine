#pragma once

#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QMenuBar>
#include <QToolBar>
#include <QDockWidget>

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

    // Main widget that will contain the DirectX11 viewport
    QWidget* m_ViewportWidget;
    
    // Layout for the main window
    QVBoxLayout* m_MainLayout;
}; 