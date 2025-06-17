#ifndef _MAIN_WINDOW_H_
#define _MAIN_WINDOW_H_

#include <QMainWindow>
#include <QVBoxLayout>
#include <QMenuBar>
#include <QToolBar>
#include <QDockWidget>
#include <QTabWidget>

class DirectXViewport;
class PerformanceWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void ToggleFullscreen();
    void ToggleFPS(bool show);
    void RunBenchmark();
    void ToggleProfiler(bool show);
    void OnTabCloseRequested(int index);
    void OnTabChanged(int index);

private:
    void CreateMenus();
    void CreateToolbars();
    void CreateDockWidgets();

    DirectXViewport* m_ViewportWidget;
    QVBoxLayout* m_MainLayout;
    PerformanceWidget* m_PerformanceWidget;
    QTabWidget* m_TabWidget;
    QDockWidget* m_PropertiesDock;
};

#endif