#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVBoxLayout>
#include <QTabWidget>
#include <QDockWidget>
#include <QMenu>
#include <QMenuBar>
#include <QAction>
#include <QToolBar>
#include <QEvent>
#include <QKeyEvent>
#include <QObject>

class DirectXViewport;
class PerformanceWidget;
class TransformUI;
class ModelListUI;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    // UI switching methods
    void SwitchToModelList();
    void SwitchToTransformUI();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void ToggleFullscreen();
    void ToggleFPS(bool show);
    void OpenBenchmarking();
    void ToggleProfiler(bool show);
    void OnTabChanged(int index);
    void OnTabCloseRequested(int index);

private:
    void CreateMenus();
    void CreateToolbars();
    void CreateDockWidgets();

private:
    DirectXViewport* m_ViewportWidget;
    QVBoxLayout* m_MainLayout;
    PerformanceWidget* m_PerformanceWidget;
    QTabWidget* m_TabWidget;
    QDockWidget* m_PropertiesDock;
    TransformUI* m_TransformUI;
    ModelListUI* m_ModelListUI;

public:
    TransformUI* GetTransformUI() const { return m_TransformUI; }
    ModelListUI* GetModelListUI() const { return m_ModelListUI; }
};

#endif // MAINWINDOW_H