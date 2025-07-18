#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

// Forward declarations
class DirectXViewport;
// class PerformanceWidget; // Now handled directly by Application
class TransformUI;
class ModelListUI;
class QVBoxLayout;
class QTabWidget;
class QDockWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    // UI switching methods
    void SwitchToModelList();
    void SwitchToTransformUI();

    // Getters
    TransformUI* GetTransformUI() const { return m_TransformUI; }
    ModelListUI* GetModelListUI() const { return m_ModelListUI; }

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
    // UI Components
    DirectXViewport* m_ViewportWidget;
    // PerformanceWidget* m_PerformanceWidget; // Now handled by Application
    TransformUI* m_TransformUI;
    ModelListUI* m_ModelListUI;
    
    // Layout components
    QVBoxLayout* m_MainLayout;
    QTabWidget* m_TabWidget;
    QDockWidget* m_PropertiesDock;
};

#endif // MAINWINDOW_H