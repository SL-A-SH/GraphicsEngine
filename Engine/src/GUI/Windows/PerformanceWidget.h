#ifndef PERFORMANCEWIDGET_H
#define PERFORMANCEWIDGET_H

#include <QWidget>
#include <QString>
#include <vector>
#include <memory>
#include "../../Core/System/PerformanceProfiler.h"
#include "../../Core/System/RenderingBenchmark.h"

// Forward declarations for Qt widgets
class QVBoxLayout;
class QHBoxLayout;
class QTabWidget;
class QTableWidget;
class QGroupBox;
class QComboBox;
class QSpinBox;
class QCheckBox;
class QPushButton;
class QProgressBar;
class QLabel;
class QTextEdit;
class QTimer;
class QHeaderView;
class QListWidget;
class QGridLayout;
class QAbstractItemView;

// Update interval for performance monitoring (16ms = 60 FPS)
#define UPDATE_INTERVAL_MS 16

class PerformanceWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PerformanceWidget(QWidget* parent = nullptr);
    ~PerformanceWidget();

    // Public method to start benchmark from MainWindow
    void StartBenchmark();
    
    // Public method to switch to benchmarking tab
    void SwitchToBenchmarkTab();
    
    // Set the main window tab index for proper logging
    void SetMainWindowTabIndex(int index);
    
    // Run benchmark with configuration
    void RunBenchmark(const BenchmarkConfig& config);
    
    // Initialize with access to the main window (to get Application)
    void InitializeBenchmarkSystem(class MainWindow* mainWindow);

private slots:
    void OnUpdateTimer();
    void OnStartBenchmark();
    void OnStopBenchmark();
    void OnExportResults();
    void OnExportComparison();
    void OnInternalTabChanged(int index);
    
    // Configuration change handlers
    void OnRenderingModeChanged(int index);
    void OnObjectCountChanged(int value);
    void OnBenchmarkDurationChanged(int value);
    void OnFrustumCullingToggled(bool enabled);
    void OnLODToggled(bool enabled);
    void OnOcclusionCullingToggled(bool enabled);

private:
    void CreateUI();
    void CreateCharts();
    void CreateRealTimeTab();
    void CreateBenchmarkTab();
    void CreateComparisonTab();
    void UpdateRealTimeStats();
    void UpdateBenchmarkProgress();
    void UpdateCharts();
    void SetupBenchmarkConfig();
    void LoadBenchmarkResults();
    void DisplayComparisonResults();
    
    // Get access to the real benchmark system from Application
    class RenderingBenchmark* GetBenchmarkSystem();

    // UI Components
    QTabWidget* m_TabWidget;
    QTableWidget* m_StatsTable;
    
    // Chart components (replaced with simple widgets)
    QListWidget* m_RealTimeChartWidget;
    QListWidget* m_ComparisonChartWidget;

    // Benchmark configuration
    QGroupBox* m_BenchmarkConfigGroup;
    QComboBox* m_RenderingModeCombo;
    QSpinBox* m_ObjectCountSpinBox;
    QSpinBox* m_BenchmarkDurationSpinBox;
    QCheckBox* m_FrustumCullingCheckBox;
    QCheckBox* m_LODCheckBox;
    QCheckBox* m_OcclusionCullingCheckBox;
    
    // Benchmark controls
    QPushButton* m_StartBenchmarkButton;
    QPushButton* m_StopBenchmarkButton;
    QProgressBar* m_BenchmarkProgressBar;
    QLabel* m_BenchmarkStatusLabel;
    QTableWidget* m_BenchmarkResultsTable;
    
    // Export and comparison
    QPushButton* m_ExportResultsButton;
    QPushButton* m_ExportComparisonButton;
    QTextEdit* m_ComparisonTextEdit;
    
    // Update timer
    QTimer* m_UpdateTimer;
    
    // Chart data storage (for simple display)
    struct ChartPoint {
        double time;
        double fps;
        double cpuTime;
        double gpuTime;
    };
    std::vector<ChartPoint> m_ChartData;
    
    // Benchmark history
    std::vector<BenchmarkResult> m_BenchmarkHistory;
    BenchmarkResult m_LastBenchmarkResult;
    
    // Main window tab index for proper logging
    int m_MainWindowTabIndex;
    
    // Internal tab widget index
    int m_InternalTabIndex;
    
    // Reference to main window for accessing Application
    class MainWindow* m_MainWindow;
    
    // Benchmark system - use Application's system
    bool m_BenchmarkRunning;
    std::vector<BenchmarkResult> m_CurrentBenchmarkResults;
    QTimer* m_BenchmarkTimer;
    int m_BenchmarkCurrentFrame;
    BenchmarkConfig m_CurrentBenchmarkConfig;
    void OnBenchmarkFrame();
};

#endif // PERFORMANCEWIDGET_H