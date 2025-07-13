#include "PerformanceWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QTableWidget>
#include <QGroupBox>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include <QTextEdit>
#include <QTimer>
#include <QHeaderView>
#include <QApplication>
#include <QScreen>
#include <QListWidget>
#include <QFrame>
#include <QGridLayout>
#include <QAbstractItemView>
#include <QDateTime>
#include <QMessageBox>
#include <QFileDialog>
#include <QStandardPaths>
#include <algorithm>
#include <memory>
#include "../../Core/System/PerformanceProfiler.h"
#include "../../Core/System/Logger.h"
#include "../../Core/System/PerformanceLogger.h"
#include "../../Core/System/RenderingBenchmark.h"

PerformanceWidget::PerformanceWidget(QWidget* parent)
    : QWidget(parent)
    , m_TabWidget(nullptr)
    , m_StatsTable(nullptr)
    , m_RealTimeChartWidget(nullptr)
    , m_BenchmarkConfigGroup(nullptr)
    , m_RenderingModeCombo(nullptr)
    , m_ObjectCountSpinBox(nullptr)
    , m_BenchmarkDurationSpinBox(nullptr)
    , m_FrustumCullingCheckBox(nullptr)
    , m_LODCheckBox(nullptr)
    , m_OcclusionCullingCheckBox(nullptr)
    , m_StartBenchmarkButton(nullptr)
    , m_StopBenchmarkButton(nullptr)
    , m_BenchmarkProgressBar(nullptr)
    , m_BenchmarkStatusLabel(nullptr)
    , m_BenchmarkResultsTable(nullptr)
    , m_ExportResultsButton(nullptr)
    , m_ExportComparisonButton(nullptr)
    , m_ComparisonTextEdit(nullptr)
    , m_ComparisonChartWidget(nullptr)
    , m_UpdateTimer(nullptr)
    , m_MainWindowTabIndex(0)
    , m_InternalTabIndex(0)
    , m_BenchmarkSystem(nullptr)
    , m_BenchmarkRunning(false)
    , m_BenchmarkTimer(nullptr)
    , m_BenchmarkCurrentFrame(0)
    , m_CurrentBenchmarkConfig()
{
    CreateUI();
    CreateCharts();
    
    // Create update timer with lower frequency to reduce CPU overhead
    m_UpdateTimer = new QTimer(this);
    connect(m_UpdateTimer, &QTimer::timeout, this, &PerformanceWidget::OnUpdateTimer);
    m_UpdateTimer->start(200); // Update every 200ms (5 FPS) to reduce overhead
    
    // Set up real-time callback
    PerformanceProfiler::GetInstance().SetRealTimeCallback([this](const PerformanceProfiler::TimingData& data) {
        // This will be called from the profiler thread, so we need to be careful
        // For now, we'll update in the timer callback
    });
    
    // Initialize benchmark timer
    m_BenchmarkTimer = new QTimer(this);
    connect(m_BenchmarkTimer, &QTimer::timeout, this, &PerformanceWidget::OnBenchmarkFrame);
    m_BenchmarkCurrentFrame = 0;
    
    // Connect tab widget signals to control update frequency
    if (m_TabWidget)
    {
        connect(m_TabWidget, &QTabWidget::currentChanged, this, &PerformanceWidget::OnInternalTabChanged);
    }
}

PerformanceWidget::~PerformanceWidget()
{
    if (m_UpdateTimer)
    {
        m_UpdateTimer->stop();
        delete m_UpdateTimer;
    }
    if (m_BenchmarkTimer)
    {
        m_BenchmarkTimer->stop();
        delete m_BenchmarkTimer;
    }
}

void PerformanceWidget::CreateUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    setLayout(mainLayout);
    
    // Create tab widget
    m_TabWidget = new QTabWidget(this);
    mainLayout->addWidget(m_TabWidget);
    
    // Create tabs
    CreateRealTimeTab();
    CreateBenchmarkTab();
    CreateComparisonTab();
}

void PerformanceWidget::CreateRealTimeTab()
{
    QWidget* realTimeTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(realTimeTab);
    
    // Create stats table
    m_StatsTable = new QTableWidget(realTimeTab);
    m_StatsTable->setColumnCount(2);
    m_StatsTable->setHorizontalHeaderLabels({"Metric", "Value"});
    m_StatsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_StatsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_StatsTable->verticalHeader()->setVisible(false);
    m_StatsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_StatsTable->setMinimumHeight(height() * 0.5);
    layout->addWidget(m_StatsTable);
    
    // Create real-time chart widget (simple list for now)
    m_RealTimeChartWidget = new QListWidget(realTimeTab);
    m_RealTimeChartWidget->setMinimumHeight(height() * 0.5);
    layout->addWidget(m_RealTimeChartWidget);
    
    m_TabWidget->addTab(realTimeTab, "Real-time Monitoring");
}

void PerformanceWidget::CreateBenchmarkTab()
{
    QWidget* benchmarkTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(benchmarkTab);
    
    // Benchmark configuration group
    m_BenchmarkConfigGroup = new QGroupBox("Benchmark Configuration", benchmarkTab);
    QGridLayout* configLayout = new QGridLayout(m_BenchmarkConfigGroup);
    
    // Rendering mode
    configLayout->addWidget(new QLabel("Rendering Mode:"), 0, 0);
    m_RenderingModeCombo = new QComboBox(m_BenchmarkConfigGroup);
    m_RenderingModeCombo->addItem("CPU-Driven", 0);
    m_RenderingModeCombo->addItem("GPU-Driven", 1);
    m_RenderingModeCombo->addItem("Hybrid", 2);
    connect(m_RenderingModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PerformanceWidget::OnRenderingModeChanged);
    configLayout->addWidget(m_RenderingModeCombo, 0, 1);
    
    // Object count
    configLayout->addWidget(new QLabel("Object Count:"), 1, 0);
    m_ObjectCountSpinBox = new QSpinBox(m_BenchmarkConfigGroup);
    m_ObjectCountSpinBox->setRange(100, 100000);
    m_ObjectCountSpinBox->setValue(1000);
    m_ObjectCountSpinBox->setSuffix(" objects");
    connect(m_ObjectCountSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &PerformanceWidget::OnObjectCountChanged);
    configLayout->addWidget(m_ObjectCountSpinBox, 1, 1);
    
    // Benchmark duration
    configLayout->addWidget(new QLabel("Duration:"), 2, 0);
    m_BenchmarkDurationSpinBox = new QSpinBox(m_BenchmarkConfigGroup);
    m_BenchmarkDurationSpinBox->setRange(60, 3000);
    m_BenchmarkDurationSpinBox->setValue(300);
    m_BenchmarkDurationSpinBox->setSuffix(" frames");
    connect(m_BenchmarkDurationSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &PerformanceWidget::OnBenchmarkDurationChanged);
    configLayout->addWidget(m_BenchmarkDurationSpinBox, 2, 1);
    
    // Optimization checkboxes
    m_FrustumCullingCheckBox = new QCheckBox("Enable Frustum Culling", m_BenchmarkConfigGroup);
    m_FrustumCullingCheckBox->setChecked(true);
    connect(m_FrustumCullingCheckBox, &QCheckBox::toggled, this, &PerformanceWidget::OnFrustumCullingToggled);
    configLayout->addWidget(m_FrustumCullingCheckBox, 3, 0);
    
    m_LODCheckBox = new QCheckBox("Enable LOD", m_BenchmarkConfigGroup);
    m_LODCheckBox->setChecked(false);
    connect(m_LODCheckBox, &QCheckBox::toggled, this, &PerformanceWidget::OnLODToggled);
    configLayout->addWidget(m_LODCheckBox, 3, 1);
    
    m_OcclusionCullingCheckBox = new QCheckBox("Enable Occlusion Culling", m_BenchmarkConfigGroup);
    m_OcclusionCullingCheckBox->setChecked(false);
    connect(m_OcclusionCullingCheckBox, &QCheckBox::toggled, this, &PerformanceWidget::OnOcclusionCullingToggled);
    configLayout->addWidget(m_OcclusionCullingCheckBox, 4, 0);
    
    layout->addWidget(m_BenchmarkConfigGroup);
    
    // Benchmark controls
    QHBoxLayout* controlsLayout = new QHBoxLayout();
    m_StartBenchmarkButton = new QPushButton("Start Benchmark", benchmarkTab);
    connect(m_StartBenchmarkButton, &QPushButton::clicked, this, &PerformanceWidget::OnStartBenchmark);
    controlsLayout->addWidget(m_StartBenchmarkButton);
    
    m_StopBenchmarkButton = new QPushButton("Stop Benchmark", benchmarkTab);
    m_StopBenchmarkButton->setEnabled(false);
    connect(m_StopBenchmarkButton, &QPushButton::clicked, this, &PerformanceWidget::OnStopBenchmark);
    controlsLayout->addWidget(m_StopBenchmarkButton);
    
    layout->addLayout(controlsLayout);
    
    // Progress bar
    m_BenchmarkProgressBar = new QProgressBar(benchmarkTab);
    m_BenchmarkProgressBar->setVisible(false);
    layout->addWidget(m_BenchmarkProgressBar);
    
    // Status label
    m_BenchmarkStatusLabel = new QLabel("Ready to start benchmark", benchmarkTab);
    layout->addWidget(m_BenchmarkStatusLabel);
    
    // Results table
    m_BenchmarkResultsTable = new QTableWidget(benchmarkTab);
    m_BenchmarkResultsTable->setColumnCount(2);
    m_BenchmarkResultsTable->setHorizontalHeaderLabels({"Metric", "Value"});
    m_BenchmarkResultsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_BenchmarkResultsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_BenchmarkResultsTable->verticalHeader()->setVisible(false);
    m_BenchmarkResultsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(m_BenchmarkResultsTable);
    
    m_TabWidget->addTab(benchmarkTab, "Benchmarking");
}

void PerformanceWidget::CreateComparisonTab()
{
    QWidget* comparisonTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(comparisonTab);
    
    // Export buttons
    QHBoxLayout* exportLayout = new QHBoxLayout();
    m_ExportResultsButton = new QPushButton("Export Results", comparisonTab);
    m_ExportResultsButton->setEnabled(false); // Disabled until benchmarks are run
    connect(m_ExportResultsButton, &QPushButton::clicked, this, &PerformanceWidget::OnExportResults);
    exportLayout->addWidget(m_ExportResultsButton);
    
    m_ExportComparisonButton = new QPushButton("Export Comparison", comparisonTab);
    m_ExportComparisonButton->setEnabled(false); // Disabled until benchmarks are run
    connect(m_ExportComparisonButton, &QPushButton::clicked, this, &PerformanceWidget::OnExportComparison);
    exportLayout->addWidget(m_ExportComparisonButton);
    
    layout->addLayout(exportLayout);
    
    // Comparison text
    m_ComparisonTextEdit = new QTextEdit(comparisonTab);
    m_ComparisonTextEdit->setReadOnly(true);
    m_ComparisonTextEdit->setMaximumHeight(200);
    layout->addWidget(m_ComparisonTextEdit);
    
    // Comparison chart widget (simple list for now)
    m_ComparisonChartWidget = new QListWidget(comparisonTab);
    m_ComparisonChartWidget->setMinimumHeight(300);
    layout->addWidget(m_ComparisonChartWidget);
    
    m_TabWidget->addTab(comparisonTab, "Comparison");
}

void PerformanceWidget::CreateCharts()
{
    // Initialize chart widgets (simple list widgets for now)
    if (m_RealTimeChartWidget)
    {
        m_RealTimeChartWidget->addItem("Real-time Performance Data");
        m_RealTimeChartWidget->addItem("FPS: --");
        m_RealTimeChartWidget->addItem("CPU Time: -- ms");
        m_RealTimeChartWidget->addItem("GPU Time: -- ms");
    }
    
    if (m_ComparisonChartWidget)
    {
        m_ComparisonChartWidget->addItem("Performance Comparison Data");
        m_ComparisonChartWidget->addItem("No comparison data available");
    }
}

void PerformanceWidget::OnUpdateTimer()
{
    // Only update if the main window tab is the performance tab (index 1)
    if (m_MainWindowTabIndex != 1)
    {
        return; // Skip updates when performance tab is not focused
    }
    
    // Update stats table only every few frames to reduce overhead
    static int updateCounter = 0;
    if (updateCounter++ % 5 == 0) // Update every 5th frame (about 2 FPS)
    {
        UpdateRealTimeStats();
        UpdateCharts();
    }
    
    // Always update benchmark progress if running
    UpdateBenchmarkProgress();
    
    // Log performance metrics to file less frequently
    static int logCounter = 0;
    if (logCounter++ % 50 == 0) // Log every 50th frame (about 0.5 FPS) to reduce spam
    {
        const auto& lastFrame = PerformanceProfiler::GetInstance().GetLastFrameTiming();
        double profilerFPS = PerformanceProfiler::GetInstance().GetCurrentFPS();
        
        // Get viewport FPS from the viewport widget (if available)
        double viewportFPS = profilerFPS; // Default to profiler FPS if viewport not available

        // Log metrics for both tabs when they're focused
        if (m_MainWindowTabIndex == 1) // Performance tab focused in MainWindow
        {
            PerformanceLogger::GetInstance().LogPerformanceMetrics(
                "Performance Tab Focused",
                profilerFPS,
                viewportFPS,
                lastFrame.cpuTime,
                lastFrame.gpuTime,
                lastFrame.drawCalls,
                lastFrame.triangles,
                lastFrame.instances
            );
        }
        else if (m_MainWindowTabIndex == 0) // Viewport tab focused in MainWindow
        {
            PerformanceLogger::GetInstance().LogPerformanceMetrics(
                "Viewport Tab Focused",
                profilerFPS,
                viewportFPS,
                lastFrame.cpuTime,
                lastFrame.gpuTime,
                lastFrame.drawCalls,
                lastFrame.triangles,
                lastFrame.instances
            );
        }
    }
}

void PerformanceWidget::UpdateRealTimeStats()
{
    const auto& lastFrame = PerformanceProfiler::GetInstance().GetLastFrameTiming();
    
    // Ensure table has enough rows
    if (m_StatsTable->rowCount() < 12)
    {
        m_StatsTable->setRowCount(12);
    }
    
    // Cache current values to avoid unnecessary updates
    static double lastFPS = -1.0;
    static double lastCPUTime = -1.0;
    static double lastGPUTime = -1.0;
    static int lastDrawCalls = -1;
    static int lastTriangles = -1;
    static int lastInstances = -1;
    static double lastGPUMemory = -1.0;
    static double lastCPUMemory = -1.0;
    static double lastBandwidth = -1.0;
    
    double currentFPS = PerformanceProfiler::GetInstance().GetCurrentFPS();
    
    // Only update FPS if changed significantly
    if (std::abs(currentFPS - lastFPS) > 0.5)
    {
        if (!m_StatsTable->item(0, 0)) m_StatsTable->setItem(0, 0, new QTableWidgetItem("FPS"));
        if (!m_StatsTable->item(0, 1)) m_StatsTable->setItem(0, 1, new QTableWidgetItem());
        m_StatsTable->item(0, 1)->setText(QString::number(currentFPS, 'f', 1));
        lastFPS = currentFPS;
    }

    // Only update CPU Time if changed significantly
    if (std::abs(lastFrame.cpuTime - lastCPUTime) > 0.5)
    {
        if (!m_StatsTable->item(1, 0)) m_StatsTable->setItem(1, 0, new QTableWidgetItem("CPU Time (ms)"));
        if (!m_StatsTable->item(1, 1)) m_StatsTable->setItem(1, 1, new QTableWidgetItem());
        m_StatsTable->item(1, 1)->setText(QString::number(lastFrame.cpuTime, 'f', 2));
        lastCPUTime = lastFrame.cpuTime;
    }

    // Only update GPU Time if changed significantly
    if (std::abs(lastFrame.gpuTime - lastGPUTime) > 0.1)
    {
        if (!m_StatsTable->item(2, 0)) m_StatsTable->setItem(2, 0, new QTableWidgetItem("GPU Time (ms)"));
        if (!m_StatsTable->item(2, 1)) m_StatsTable->setItem(2, 1, new QTableWidgetItem());
        m_StatsTable->item(2, 1)->setText(QString::number(lastFrame.gpuTime, 'f', 2));
        lastGPUTime = lastFrame.gpuTime;
    }

    // Only update Draw Calls if changed
    if (lastFrame.drawCalls != lastDrawCalls)
    {
        if (!m_StatsTable->item(3, 0)) m_StatsTable->setItem(3, 0, new QTableWidgetItem("Draw Calls"));
        if (!m_StatsTable->item(3, 1)) m_StatsTable->setItem(3, 1, new QTableWidgetItem());
        m_StatsTable->item(3, 1)->setText(QString::number(lastFrame.drawCalls));
        lastDrawCalls = lastFrame.drawCalls;
    }

    // Only update Triangles if changed
    if (lastFrame.triangles != lastTriangles)
    {
        if (!m_StatsTable->item(4, 0)) m_StatsTable->setItem(4, 0, new QTableWidgetItem("Triangles"));
        if (!m_StatsTable->item(4, 1)) m_StatsTable->setItem(4, 1, new QTableWidgetItem());
        m_StatsTable->item(4, 1)->setText(QString::number(lastFrame.triangles));
        lastTriangles = lastFrame.triangles;
    }

    // Only update Instances if changed
    if (lastFrame.instances != lastInstances)
    {
        if (!m_StatsTable->item(5, 0)) m_StatsTable->setItem(5, 0, new QTableWidgetItem("Instances"));
        if (!m_StatsTable->item(5, 1)) m_StatsTable->setItem(5, 1, new QTableWidgetItem());
        m_StatsTable->item(5, 1)->setText(QString::number(lastFrame.instances));
        lastInstances = lastFrame.instances;
    }

    // Indirect Draw Calls - GPU-driven rendering calls (not implemented yet)
    if (!m_StatsTable->item(6, 0)) m_StatsTable->setItem(6, 0, new QTableWidgetItem("Indirect Draw Calls"));
    if (!m_StatsTable->item(6, 1)) m_StatsTable->setItem(6, 1, new QTableWidgetItem());
    m_StatsTable->item(6, 1)->setText(QString::number(lastFrame.indirectDrawCalls));

    // Compute Dispatches - Compute shader dispatches (not implemented yet)
    if (!m_StatsTable->item(7, 0)) m_StatsTable->setItem(7, 0, new QTableWidgetItem("Compute Dispatches"));
    if (!m_StatsTable->item(7, 1)) m_StatsTable->setItem(7, 1, new QTableWidgetItem());
    m_StatsTable->item(7, 1)->setText(QString::number(lastFrame.computeDispatches));

    // Only update GPU Memory if changed significantly
    if (std::abs(lastFrame.gpuMemoryUsage - lastGPUMemory) > 0.5)
    {
        if (!m_StatsTable->item(8, 0)) m_StatsTable->setItem(8, 0, new QTableWidgetItem("GPU Memory (MB)"));
        if (!m_StatsTable->item(8, 1)) m_StatsTable->setItem(8, 1, new QTableWidgetItem());
        m_StatsTable->item(8, 1)->setText(QString::number(lastFrame.gpuMemoryUsage, 'f', 1));
        lastGPUMemory = lastFrame.gpuMemoryUsage;
    }

    // Only update CPU Memory if changed significantly
    if (std::abs(lastFrame.cpuMemoryUsage - lastCPUMemory) > 0.5)
    {
        if (!m_StatsTable->item(9, 0)) m_StatsTable->setItem(9, 0, new QTableWidgetItem("CPU Memory (MB)"));
        if (!m_StatsTable->item(9, 1)) m_StatsTable->setItem(9, 1, new QTableWidgetItem());
        m_StatsTable->item(9, 1)->setText(QString::number(lastFrame.cpuMemoryUsage, 'f', 1));
        lastCPUMemory = lastFrame.cpuMemoryUsage;
    }

    // Only update Bandwidth if changed significantly
    if (std::abs(lastFrame.bandwidthUsage - lastBandwidth) > 0.01)
    {
        if (!m_StatsTable->item(10, 0)) m_StatsTable->setItem(10, 0, new QTableWidgetItem("Bandwidth (GB/s)"));
        if (!m_StatsTable->item(10, 1)) m_StatsTable->setItem(10, 1, new QTableWidgetItem());
        m_StatsTable->item(10, 1)->setText(QString::number(lastFrame.bandwidthUsage, 'f', 2));
        lastBandwidth = lastFrame.bandwidthUsage;
    }

    // Rendering Mode - Current rendering approach (static, only set once)
    if (!m_StatsTable->item(11, 0)) m_StatsTable->setItem(11, 0, new QTableWidgetItem("Rendering Mode"));
    if (!m_StatsTable->item(11, 1)) 
    {
        m_StatsTable->setItem(11, 1, new QTableWidgetItem());
        QString modeStr;
        switch (PerformanceProfiler::GetInstance().GetRenderingMode())
        {
            case PerformanceProfiler::RenderingMode::CPU_DRIVEN: modeStr = "CPU-Driven"; break;
            case PerformanceProfiler::RenderingMode::GPU_DRIVEN: modeStr = "GPU-Driven"; break;
            case PerformanceProfiler::RenderingMode::HYBRID: modeStr = "Hybrid"; break;
        }
        m_StatsTable->item(11, 1)->setText(modeStr);
    }
}

void PerformanceWidget::UpdateBenchmarkProgress()
{
    if (PerformanceProfiler::GetInstance().IsBenchmarkRunning())
    {
        double progress = PerformanceProfiler::GetInstance().GetBenchmarkProgress();
        m_BenchmarkProgressBar->setValue(static_cast<int>(progress * 100));
        
        QString status = QString("Benchmark running... Frame %1/%2")
            .arg(PerformanceProfiler::GetInstance().GetBenchmarkCurrentFrame())
            .arg(PerformanceProfiler::GetInstance().GetBenchmarkTotalFrames());
        m_BenchmarkStatusLabel->setText(status);
    }
    else
    {
        m_BenchmarkProgressBar->setVisible(false);
        m_BenchmarkStatusLabel->setText("Ready to start benchmark");
    }
}

void PerformanceWidget::UpdateCharts()
{
    const auto& lastFrame = PerformanceProfiler::GetInstance().GetLastFrameTiming();
    
    // Update real-time chart widget with simple text display
    if (m_RealTimeChartWidget && m_RealTimeChartWidget->count() >= 4)
    {
        double fps = PerformanceProfiler::GetInstance().GetCurrentFPS();
        
        // Only update if values have changed significantly to reduce overhead
        static double lastFPS = -1.0;
        static double lastCPUTime = -1.0;
        static double lastGPUTime = -1.0;
        
        if (std::abs(fps - lastFPS) > 1.0) // Only update if FPS changed by more than 1.0
        {
            m_RealTimeChartWidget->item(1)->setText(QString("FPS: %1").arg(fps, 0, 'f', 1));
            lastFPS = fps;
        }
        
        if (std::abs(lastFrame.cpuTime - lastCPUTime) > 1.0) // Only update if CPU time changed by more than 1.0ms
        {
            m_RealTimeChartWidget->item(2)->setText(QString("CPU Time: %1 ms").arg(lastFrame.cpuTime, 0, 'f', 2));
            lastCPUTime = lastFrame.cpuTime;
        }
        
        if (std::abs(lastFrame.gpuTime - lastGPUTime) > 0.1) // Only update if GPU time changed by more than 0.1ms
        {
            m_RealTimeChartWidget->item(3)->setText(QString("GPU Time: %1 ms").arg(lastFrame.gpuTime, 0, 'f', 2));
            lastGPUTime = lastFrame.gpuTime;
        }
    }
}

void PerformanceWidget::OnStartBenchmark()
{
    SetupBenchmarkConfig();
    m_CurrentBenchmarkConfig.approach = static_cast<BenchmarkConfig::RenderingApproach>(m_RenderingModeCombo->currentIndex());
    m_CurrentBenchmarkConfig.objectCount = m_ObjectCountSpinBox->value();
    m_CurrentBenchmarkConfig.benchmarkDuration = m_BenchmarkDurationSpinBox->value();
    m_CurrentBenchmarkConfig.enableFrustumCulling = m_FrustumCullingCheckBox->isChecked();
    m_CurrentBenchmarkConfig.enableLOD = m_LODCheckBox->isChecked();
    m_CurrentBenchmarkConfig.enableOcclusionCulling = m_OcclusionCullingCheckBox->isChecked();
    m_CurrentBenchmarkConfig.sceneName = "Benchmark Scene";
    m_CurrentBenchmarkConfig.outputDirectory = "./benchmark_results/";

    m_BenchmarkRunning = true;
    m_StartBenchmarkButton->setEnabled(false);
    m_StopBenchmarkButton->setEnabled(true);
    m_BenchmarkProgressBar->setVisible(true);
    m_BenchmarkProgressBar->setValue(0);
    m_BenchmarkStatusLabel->setText("Benchmark started...");
    m_BenchmarkCurrentFrame = 0;
    m_LastBenchmarkResult = BenchmarkResult();

    // Start the timer for frame-by-frame benchmarking
    m_BenchmarkTimer->start(1); // 1ms interval for fast benchmarking
}

void PerformanceWidget::OnStopBenchmark()
{
    m_BenchmarkTimer->stop();
    m_BenchmarkRunning = false;
    m_StartBenchmarkButton->setEnabled(true);
    m_StopBenchmarkButton->setEnabled(false);
    m_BenchmarkProgressBar->setVisible(false);
    m_BenchmarkStatusLabel->setText("Benchmark stopped");
    LoadBenchmarkResults();
}

void PerformanceWidget::SetupBenchmarkConfig()
{
    // This would typically configure the rendering system based on the benchmark settings
    // For now, we'll just log the configuration
    QString configStr = QString("Benchmark Config:\n"
                               "Mode: %1\n"
                               "Objects: %2\n"
                               "Duration: %3 frames\n"
                               "Frustum Culling: %4\n"
                               "LOD: %5\n"
                               "Occlusion Culling: %6")
        .arg(m_RenderingModeCombo->currentText())
        .arg(m_ObjectCountSpinBox->value())
        .arg(m_BenchmarkDurationSpinBox->value())
        .arg(m_FrustumCullingCheckBox->isChecked() ? "Enabled" : "Disabled")
        .arg(m_LODCheckBox->isChecked() ? "Enabled" : "Disabled")
        .arg(m_OcclusionCullingCheckBox->isChecked() ? "Enabled" : "Disabled");
    
    m_ComparisonTextEdit->setText(configStr);
}

void PerformanceWidget::RunBenchmark(const BenchmarkConfig& config)
{
    LOG("Running benchmark: " + config.sceneName);
    if (!m_BenchmarkSystem) return;

    // Run the real benchmark (blocking for now)
    m_LastBenchmarkResult = m_BenchmarkSystem->RunBenchmark(config);
    // If CPU-driven, replace any previous CPU-driven result in history
    if (config.approach == BenchmarkConfig::RenderingApproach::CPU_DRIVEN) {
        auto it = std::remove_if(m_BenchmarkHistory.begin(), m_BenchmarkHistory.end(), [](const BenchmarkResult& r) { return r.approach == "CPU-Driven"; });
        m_BenchmarkHistory.erase(it, m_BenchmarkHistory.end());
        m_BenchmarkHistory.push_back(m_LastBenchmarkResult);
    } else if (config.approach == BenchmarkConfig::RenderingApproach::GPU_DRIVEN) {
        // Always keep the last CPU-driven result for comparison
        auto cpuIt = std::find_if(m_BenchmarkHistory.begin(), m_BenchmarkHistory.end(), [](const BenchmarkResult& r) { return r.approach == "CPU-Driven"; });
        if (cpuIt != m_BenchmarkHistory.end()) {
            // Keep CPU-driven result, add GPU-driven
            auto it = std::remove_if(m_BenchmarkHistory.begin(), m_BenchmarkHistory.end(), [](const BenchmarkResult& r) { return r.approach == "GPU-Driven"; });
            m_BenchmarkHistory.erase(it, m_BenchmarkHistory.end());
            m_BenchmarkHistory.push_back(m_LastBenchmarkResult);
        } else {
            // No CPU-driven result, just add GPU-driven
            m_BenchmarkHistory.push_back(m_LastBenchmarkResult);
        }
    } else {
        // For hybrid, just add
        m_BenchmarkHistory.push_back(m_LastBenchmarkResult);
    }
    LoadBenchmarkResults();
    m_BenchmarkRunning = false;
    m_StartBenchmarkButton->setEnabled(true);
    m_StopBenchmarkButton->setEnabled(false);
    m_BenchmarkProgressBar->setVisible(false);
    m_BenchmarkStatusLabel->setText("Benchmark completed");
    m_ExportResultsButton->setEnabled(true);
    m_ExportComparisonButton->setEnabled(true);
    LOG("Benchmark completed");
}

void PerformanceWidget::LoadBenchmarkResults()
{
    // Use the real result from the last benchmark run
    const BenchmarkResult& data = m_LastBenchmarkResult;
    m_BenchmarkResultsTable->setRowCount(12);
    m_BenchmarkResultsTable->setItem(0, 0, new QTableWidgetItem("Average FPS"));
    m_BenchmarkResultsTable->setItem(0, 1, new QTableWidgetItem(QString::number(data.averageFPS, 'f', 2)));
    m_BenchmarkResultsTable->setItem(1, 0, new QTableWidgetItem("Average Frame Time (ms)"));
    m_BenchmarkResultsTable->setItem(1, 1, new QTableWidgetItem(QString::number(data.averageFrameTime, 'f', 2)));
    m_BenchmarkResultsTable->setItem(2, 0, new QTableWidgetItem("Average GPU Time (ms)"));
    m_BenchmarkResultsTable->setItem(2, 1, new QTableWidgetItem(QString::number(data.averageGPUTime, 'f', 2)));
    m_BenchmarkResultsTable->setItem(3, 0, new QTableWidgetItem("Average CPU Time (ms)"));
    m_BenchmarkResultsTable->setItem(3, 1, new QTableWidgetItem(QString::number(data.averageCPUTime, 'f', 2)));
    m_BenchmarkResultsTable->setItem(4, 0, new QTableWidgetItem("Average Draw Calls"));
    m_BenchmarkResultsTable->setItem(4, 1, new QTableWidgetItem(QString::number(data.averageDrawCalls)));
    m_BenchmarkResultsTable->setItem(5, 0, new QTableWidgetItem("Average Triangles"));
    m_BenchmarkResultsTable->setItem(5, 1, new QTableWidgetItem(QString::number(data.averageTriangles)));
    m_BenchmarkResultsTable->setItem(6, 0, new QTableWidgetItem("Average Instances"));
    m_BenchmarkResultsTable->setItem(6, 1, new QTableWidgetItem(QString::number(data.averageInstances)));
    m_BenchmarkResultsTable->setItem(7, 0, new QTableWidgetItem("Average Indirect Draw Calls"));
    m_BenchmarkResultsTable->setItem(7, 1, new QTableWidgetItem(QString::number(data.averageIndirectDrawCalls)));
    m_BenchmarkResultsTable->setItem(8, 0, new QTableWidgetItem("Average Compute Dispatches"));
    m_BenchmarkResultsTable->setItem(8, 1, new QTableWidgetItem(QString::number(data.averageComputeDispatches)));
    m_BenchmarkResultsTable->setItem(9, 0, new QTableWidgetItem("Average GPU Memory (MB)"));
    m_BenchmarkResultsTable->setItem(9, 1, new QTableWidgetItem(QString::number(data.averageGPUMemoryUsage, 'f', 1)));
    m_BenchmarkResultsTable->setItem(10, 0, new QTableWidgetItem("Average CPU Memory (MB)"));
    m_BenchmarkResultsTable->setItem(10, 1, new QTableWidgetItem(QString::number(data.averageCPUMemoryUsage, 'f', 1)));
    m_BenchmarkResultsTable->setItem(11, 0, new QTableWidgetItem("Average Bandwidth (GB/s)"));
    m_BenchmarkResultsTable->setItem(11, 1, new QTableWidgetItem(QString::number(data.averageBandwidthUsage, 'f', 2)));
    DisplayComparisonResults();
    m_ExportResultsButton->setEnabled(true);
    m_ExportComparisonButton->setEnabled(true);
}

void PerformanceWidget::DisplayComparisonResults()
{
    if (m_BenchmarkHistory.size() < 2) return;
    // Find CPU and GPU results for comparison
    const BenchmarkResult* cpuData = nullptr;
    const BenchmarkResult* gpuData = nullptr;
    for (const auto& data : m_BenchmarkHistory) {
        if (data.approach == "CPU-Driven" && !cpuData) cpuData = &data;
        else if (data.approach == "GPU-Driven" && !gpuData) gpuData = &data;
    }
    if (cpuData && gpuData) {
        double fpsImprovement = ((gpuData->averageFPS - cpuData->averageFPS) / cpuData->averageFPS) * 100.0;
        double frameTimeImprovement = ((cpuData->averageFrameTime - gpuData->averageFrameTime) / cpuData->averageFrameTime) * 100.0;
        QString comparisonText =
            QString("CPU-Driven Results:\n  FPS: %1\n  Frame Time: %2 ms\n  GPU Time: %3 ms\n  CPU Time: %4 ms\n\nGPU-Driven Results:\n  FPS: %5\n  Frame Time: %6 ms\n  GPU Time: %7 ms\n  CPU Time: %8 ms\n\nImprovements:\n  FPS: %9%%\n  Frame Time: %10%%")
            .arg(cpuData->averageFPS, 0, 'f', 2)
            .arg(cpuData->averageFrameTime, 0, 'f', 2)
            .arg(cpuData->averageGPUTime, 0, 'f', 2)
            .arg(cpuData->averageCPUTime, 0, 'f', 2)
            .arg(gpuData->averageFPS, 0, 'f', 2)
            .arg(gpuData->averageFrameTime, 0, 'f', 2)
            .arg(gpuData->averageGPUTime, 0, 'f', 2)
            .arg(gpuData->averageCPUTime, 0, 'f', 2)
            .arg(fpsImprovement, 0, 'f', 1)
            .arg(frameTimeImprovement, 0, 'f', 1);
        m_ComparisonTextEdit->setText(comparisonText);
        if (m_ComparisonChartWidget) {
            m_ComparisonChartWidget->clear();
            m_ComparisonChartWidget->addItem("Performance Comparison Data");
            m_ComparisonChartWidget->addItem(QString("CPU-Driven FPS: %1").arg(cpuData->averageFPS, 0, 'f', 2));
            m_ComparisonChartWidget->addItem(QString("GPU-Driven FPS: %1").arg(gpuData->averageFPS, 0, 'f', 2));
            m_ComparisonChartWidget->addItem(QString("CPU-Driven Frame Time: %1 ms").arg(cpuData->averageFrameTime, 0, 'f', 2));
            m_ComparisonChartWidget->addItem(QString("GPU-Driven Frame Time: %1 ms").arg(gpuData->averageFrameTime, 0, 'f', 2));
            m_ComparisonChartWidget->addItem(QString("FPS Improvement: %1%").arg(fpsImprovement, 0, 'f', 1));
            m_ComparisonChartWidget->addItem(QString("Frame Time Improvement: %1%").arg(frameTimeImprovement, 0, 'f', 1));
        }
    }
}

void PerformanceWidget::OnExportResults()
{
    QString filename = QFileDialog::getSaveFileName(this, "Export Benchmark Results",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/benchmark_results.txt",
        "Text Files (*.txt);;CSV Files (*.csv)");
    
    if (!filename.isEmpty())
    {
        const auto& results = PerformanceProfiler::GetInstance().GetLastBenchmarkResults();
        PerformanceProfiler::GetInstance().ExportBenchmarkResults(filename.toStdString(), results);
        QMessageBox::information(this, "Export", "Benchmark results exported successfully!");
    }
}

void PerformanceWidget::OnExportComparison()
{
    QString filename = QFileDialog::getSaveFileName(this, "Export Comparison Report",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/comparison_report.txt",
        "Text Files (*.txt)");
    
    if (!filename.isEmpty() && m_BenchmarkHistory.size() >= 2)
    {
        // Find CPU and GPU results
        const BenchmarkResult* cpuData = nullptr;
        const BenchmarkResult* gpuData = nullptr;
        
        for (const auto& data : m_BenchmarkHistory)
        {
            if (data.approach == "CPU-Driven" && !cpuData)
                cpuData = &data;
            else if (data.approach == "GPU-Driven" && !gpuData)
                gpuData = &data;
        }
        
        if (cpuData && gpuData)
        {
            // Convert BenchmarkData to BenchmarkResults for export
            PerformanceProfiler::BenchmarkResults cpuResults, gpuResults;
            // This would require proper conversion - simplified for now
            PerformanceProfiler::GetInstance().ExportComparisonReport(filename.toStdString(), cpuResults, gpuResults);
            QMessageBox::information(this, "Export", "Comparison report exported successfully!");
        }
    }
}

// Configuration change handlers
void PerformanceWidget::OnRenderingModeChanged(int index) { /* Update configuration */ }
void PerformanceWidget::OnObjectCountChanged(int value) { /* Update configuration */ }
void PerformanceWidget::OnBenchmarkDurationChanged(int value) { /* Update configuration */ }
void PerformanceWidget::OnFrustumCullingToggled(bool enabled) { /* Update configuration */ }
void PerformanceWidget::OnLODToggled(bool enabled) { /* Update configuration */ }
void PerformanceWidget::OnOcclusionCullingToggled(bool enabled) { /* Update configuration */ }

void PerformanceWidget::StartBenchmark()
{
    LOG("PerformanceWidget::StartBenchmark called - automatically starting benchmark");
    
    // Set default benchmark configuration
    if (m_RenderingModeCombo)
        m_RenderingModeCombo->setCurrentIndex(0); // CPU-Driven
    
    if (m_ObjectCountSpinBox)
        m_ObjectCountSpinBox->setValue(1000);
    
    if (m_BenchmarkDurationSpinBox)
        m_BenchmarkDurationSpinBox->setValue(300);
    
    if (m_FrustumCullingCheckBox)
        m_FrustumCullingCheckBox->setChecked(true);
    
    if (m_LODCheckBox)
        m_LODCheckBox->setChecked(false);
    
    if (m_OcclusionCullingCheckBox)
        m_OcclusionCullingCheckBox->setChecked(false);
    
    // Start the benchmark
    OnStartBenchmark();
}

void PerformanceWidget::SwitchToBenchmarkTab()
{
    LOG("PerformanceWidget::SwitchToBenchmarkTab called");
    if (m_TabWidget)
    {
        m_TabWidget->setCurrentIndex(1); // Switch to benchmarking tab
        LOG("Switched to benchmarking tab");
    }
}

void PerformanceWidget::SetMainWindowTabIndex(int index)
{
    m_MainWindowTabIndex = index;
    LOG("PerformanceWidget::SetMainWindowTabIndex - Main window tab index set to: " + std::to_string(index));
    
    // Adjust timer frequency based on tab focus
    if (m_UpdateTimer)
    {
        if (index == 1) // Performance tab focused
        {
            m_UpdateTimer->setInterval(200); // 5 FPS for performance monitoring
        }
        else // Viewport tab focused
        {
            m_UpdateTimer->setInterval(1000); // 1 FPS when not focused to minimize overhead
        }
    }
}

void PerformanceWidget::OnInternalTabChanged(int index)
{
    m_InternalTabIndex = index;
    LOG("PerformanceWidget::OnInternalTabChanged - Internal tab index set to: " + std::to_string(index));
} 

void PerformanceWidget::OnBenchmarkFrame()
{
    if (!m_BenchmarkSystem || !m_BenchmarkRunning) {
        m_BenchmarkTimer->stop();
        return;
    }
    // Simulate a frame: run one frame of the benchmark and record metrics
    // (You may need to expose a method in RenderingBenchmark to process a single frame)
    m_BenchmarkSystem->RunBenchmarkFrame(m_CurrentBenchmarkConfig, m_LastBenchmarkResult, m_BenchmarkCurrentFrame);
    m_BenchmarkCurrentFrame++;
    m_BenchmarkProgressBar->setValue(static_cast<int>((double)m_BenchmarkCurrentFrame / m_CurrentBenchmarkConfig.benchmarkDuration * 100));
    m_BenchmarkStatusLabel->setText(QString("Benchmark running... Frame %1/%2").arg(m_BenchmarkCurrentFrame).arg(m_CurrentBenchmarkConfig.benchmarkDuration));
    if (m_BenchmarkCurrentFrame >= m_CurrentBenchmarkConfig.benchmarkDuration) {
        m_BenchmarkTimer->stop();
        m_BenchmarkRunning = false;
        m_StartBenchmarkButton->setEnabled(true);
        m_StopBenchmarkButton->setEnabled(false);
        m_BenchmarkProgressBar->setVisible(false);
        m_BenchmarkStatusLabel->setText("Benchmark completed");
        LoadBenchmarkResults();
        m_ExportResultsButton->setEnabled(true);
        m_ExportComparisonButton->setEnabled(true);
    }
} 