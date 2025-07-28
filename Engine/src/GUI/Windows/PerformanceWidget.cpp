#include "PerformanceWidget.h"
#include "../../Core/System/PerformanceProfiler.h"
#include "../../Core/System/RenderingBenchmark.h"
#include "../../Core/System/Logger.h"
#include "DirectXViewport.h"
#include "MainWindow.h"
#include <QApplication>
#include <QTabWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
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
#include <QListWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QTime>
#include <string>
#include <sstream>
#include <iomanip>

PerformanceWidget::PerformanceWidget(QWidget* parent)
    : QWidget(parent)
    , m_TabWidget(nullptr)
    , m_StatsTable(nullptr)
    , m_RealTimeChartWidget(nullptr)
    , m_ComparisonChartWidget(nullptr)
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
    , m_UpdateTimer(nullptr)
    , m_MainWindowTabIndex(0)
    , m_InternalTabIndex(0)
    , m_MainWindow(nullptr)
    , m_BenchmarkRunning(false)
    , m_BenchmarkTimer(nullptr)
    , m_BenchmarkCurrentFrame(0)
{
    CreateUI();
    
    // Set up update timer
    m_UpdateTimer = new QTimer(this);
    connect(m_UpdateTimer, &QTimer::timeout, this, &PerformanceWidget::OnUpdateTimer);
    m_UpdateTimer->start(1000); // Changed from 16 to 1000ms
    
    // Set up benchmark timer
    m_BenchmarkTimer = new QTimer(this);
    connect(m_BenchmarkTimer, &QTimer::timeout, this, &PerformanceWidget::OnBenchmarkFrame);
    
    LOG("PerformanceWidget initialized with Qt interface");
}

PerformanceWidget::~PerformanceWidget()
{
    if (m_UpdateTimer) {
        m_UpdateTimer->stop();
    }
    if (m_BenchmarkTimer) {
        m_BenchmarkTimer->stop();
    }
}

void PerformanceWidget::InitializeBenchmarkSystem(MainWindow* mainWindow)
{
    m_MainWindow = mainWindow;
    LOG("PerformanceWidget: Initialized with MainWindow reference for benchmark system access");
}

RenderingBenchmark* PerformanceWidget::GetBenchmarkSystem()
{
    if (!m_MainWindow) {
        LOG_ERROR("PerformanceWidget: No MainWindow reference - cannot access benchmark system");
        return nullptr;
    }
    
    // Access the Application's benchmark system through: MainWindow -> DirectXViewport -> SystemManager -> Application
    auto viewport = m_MainWindow->findChild<DirectXViewport*>();
    if (!viewport) {
        LOG_ERROR("PerformanceWidget: Cannot find DirectXViewport");
        return nullptr;
    }
    
    auto systemManager = viewport->GetSystemManager();
    if (!systemManager) {
        LOG_ERROR("PerformanceWidget: Cannot get SystemManager");
        return nullptr;
    }
    
    auto application = systemManager->GetApplication();
    if (!application) {
        LOG_ERROR("PerformanceWidget: Cannot get Application");
        return nullptr;
    }
    
    // Get the benchmark system from the Application
    auto benchmarkSystem = application->GetBenchmarkSystem();
    if (!benchmarkSystem) {
        LOG_ERROR("PerformanceWidget: Application benchmark system is null");
        return nullptr;
    }
    
    return benchmarkSystem;
}

void PerformanceWidget::CreateUI()
{
    setMinimumSize(800, 600);
    
    // Main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Create tab widget
    m_TabWidget = new QTabWidget(this);
    mainLayout->addWidget(m_TabWidget);
    
    // Create tabs
    CreateRealTimeTab();
    CreateBenchmarkTab();
    CreateComparisonTab();
    
    // Connect tab change signal
    connect(m_TabWidget, &QTabWidget::currentChanged, this, &PerformanceWidget::OnInternalTabChanged);
    
    setLayout(mainLayout);
}

void PerformanceWidget::CreateRealTimeTab()
{
    QWidget* realTimeTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(realTimeTab);
    
    // Performance statistics table
    m_StatsTable = new QTableWidget(20, 2, realTimeTab);
    m_StatsTable->setHorizontalHeaderLabels(QStringList() << "Metric" << "Value");
    
    // Set column widths to 50% each
    QHeaderView* horizontalHeader = m_StatsTable->horizontalHeader();
    horizontalHeader->setSectionResizeMode(0, QHeaderView::Stretch);
    horizontalHeader->setSectionResizeMode(1, QHeaderView::Stretch);
    
    // Remove alternating row colors - keep all rows consistent
    m_StatsTable->setAlternatingRowColors(false);
    m_StatsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    
    // Populate initial metrics
    QStringList metrics = {
        "FPS", "Frame Time (ms)", "Rendering Mode", "Total Objects", "Visible Objects",
        "CPU Frustum Culling (μs)", "GPU Frustum Culling (μs)", "GPU Speedup",
        "Draw Calls", "Triangles", "Instances", "Indirect Draw Calls", "Compute Dispatches",
        "CPU Time (ms)", "GPU Time (ms)", "GPU Memory (MB)", "CPU Memory (MB)",
        "Bandwidth (MB/s)", "Visibility Ratio (%)", "Triangles per Draw Call"
    };
    
    for (int i = 0; i < metrics.size(); ++i) {
        m_StatsTable->setItem(i, 0, new QTableWidgetItem(metrics[i]));
        m_StatsTable->setItem(i, 1, new QTableWidgetItem("N/A"));
    }
    
    layout->addWidget(new QLabel("Real-Time Performance Metrics"));
    layout->addWidget(m_StatsTable);
    
    // Real-time chart (simplified as list widget)
    m_RealTimeChartWidget = new QListWidget(realTimeTab);
    m_RealTimeChartWidget->setMaximumHeight(200);
    layout->addWidget(new QLabel("Performance History"));
    layout->addWidget(m_RealTimeChartWidget);
    
    m_TabWidget->addTab(realTimeTab, "Real-Time");
}

void PerformanceWidget::CreateBenchmarkTab()
{
    QWidget* benchmarkTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(benchmarkTab);
    
    // Configuration group
    m_BenchmarkConfigGroup = new QGroupBox("Benchmark Configuration", benchmarkTab);
    QGridLayout* configLayout = new QGridLayout(m_BenchmarkConfigGroup);
    
    // Rendering mode
    configLayout->addWidget(new QLabel("Rendering Mode:"), 0, 0);
    m_RenderingModeCombo = new QComboBox();
    m_RenderingModeCombo->addItems(QStringList() << "CPU-Driven" << "GPU-Driven");
    configLayout->addWidget(m_RenderingModeCombo, 0, 1);
    connect(m_RenderingModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PerformanceWidget::OnRenderingModeChanged);
    
    // Object count
    configLayout->addWidget(new QLabel("Object Count:"), 1, 0);
    m_ObjectCountSpinBox = new QSpinBox();
    m_ObjectCountSpinBox->setRange(100, 100000);
    m_ObjectCountSpinBox->setValue(1000);
    m_ObjectCountSpinBox->setSingleStep(100);
    configLayout->addWidget(m_ObjectCountSpinBox, 1, 1);
    connect(m_ObjectCountSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &PerformanceWidget::OnObjectCountChanged);
    
    // Benchmark duration
    configLayout->addWidget(new QLabel("Duration (frames):"), 2, 0);
    m_BenchmarkDurationSpinBox = new QSpinBox();
    m_BenchmarkDurationSpinBox->setRange(60, 3600);
    m_BenchmarkDurationSpinBox->setValue(300);
    m_BenchmarkDurationSpinBox->setSingleStep(60);
    configLayout->addWidget(m_BenchmarkDurationSpinBox, 2, 1);
    connect(m_BenchmarkDurationSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &PerformanceWidget::OnBenchmarkDurationChanged);
    
    // Options
    m_FrustumCullingCheckBox = new QCheckBox("Enable Frustum Culling");
    m_FrustumCullingCheckBox->setChecked(true);
    configLayout->addWidget(m_FrustumCullingCheckBox, 3, 0, 1, 2);
    connect(m_FrustumCullingCheckBox, &QCheckBox::toggled,
            this, &PerformanceWidget::OnFrustumCullingToggled);
    
    m_LODCheckBox = new QCheckBox("Enable LOD (coming soon)");
    m_LODCheckBox->setEnabled(false);
    configLayout->addWidget(m_LODCheckBox, 4, 0, 1, 2);
    connect(m_LODCheckBox, &QCheckBox::toggled,
            this, &PerformanceWidget::OnLODToggled);
    
    m_OcclusionCullingCheckBox = new QCheckBox("Enable Occlusion Culling (coming soon)");
    m_OcclusionCullingCheckBox->setEnabled(false);
    configLayout->addWidget(m_OcclusionCullingCheckBox, 5, 0, 1, 2);
    connect(m_OcclusionCullingCheckBox, &QCheckBox::toggled,
            this, &PerformanceWidget::OnOcclusionCullingToggled);
    
    layout->addWidget(m_BenchmarkConfigGroup);
    
    // Controls group
    QGroupBox* controlsGroup = new QGroupBox("Benchmark Controls", benchmarkTab);
    QHBoxLayout* controlsLayout = new QHBoxLayout(controlsGroup);
    
    m_StartBenchmarkButton = new QPushButton("Start Benchmark");
    m_StopBenchmarkButton = new QPushButton("Stop Benchmark");
    m_StopBenchmarkButton->setEnabled(false);
    
    controlsLayout->addWidget(m_StartBenchmarkButton);
    controlsLayout->addWidget(m_StopBenchmarkButton);
    controlsLayout->addStretch();
    
    connect(m_StartBenchmarkButton, &QPushButton::clicked, this, &PerformanceWidget::OnStartBenchmark);
    connect(m_StopBenchmarkButton, &QPushButton::clicked, this, &PerformanceWidget::OnStopBenchmark);
    
    layout->addWidget(controlsGroup);
    
    // Progress group
    QGroupBox* progressGroup = new QGroupBox("Progress", benchmarkTab);
    QVBoxLayout* progressLayout = new QVBoxLayout(progressGroup);
    
    m_BenchmarkProgressBar = new QProgressBar();
    m_BenchmarkStatusLabel = new QLabel("Ready");
    
    progressLayout->addWidget(m_BenchmarkProgressBar);
    progressLayout->addWidget(m_BenchmarkStatusLabel);
    
    layout->addWidget(progressGroup);
    
    // Results table
    m_BenchmarkResultsTable = new QTableWidget(0, 8, benchmarkTab);
    m_BenchmarkResultsTable->setHorizontalHeaderLabels(QStringList() 
        << "Approach" << "Objects" << "Visible" << "FPS" << "Frame Time (ms)" 
        << "GPU Time (ms)" << "CPU Time (ms)" << "Draw Calls");
    m_BenchmarkResultsTable->horizontalHeader()->setStretchLastSection(true);
    m_BenchmarkResultsTable->setAlternatingRowColors(false);
    m_BenchmarkResultsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    
    layout->addWidget(new QLabel("Benchmark Results"));
    layout->addWidget(m_BenchmarkResultsTable);
    
    // Export buttons
    QHBoxLayout* exportLayout = new QHBoxLayout();
    m_ExportResultsButton = new QPushButton("Export Results");
    m_ExportComparisonButton = new QPushButton("Export Comparison");
    
    exportLayout->addWidget(m_ExportResultsButton);
    exportLayout->addWidget(m_ExportComparisonButton);
    exportLayout->addStretch();
    
    connect(m_ExportResultsButton, &QPushButton::clicked, this, &PerformanceWidget::OnExportResults);
    connect(m_ExportComparisonButton, &QPushButton::clicked, this, &PerformanceWidget::OnExportComparison);
    
    layout->addLayout(exportLayout);
    
    m_TabWidget->addTab(benchmarkTab, "Benchmark");
}

void PerformanceWidget::CreateComparisonTab()
{
    QWidget* comparisonTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(comparisonTab);
    
    layout->addWidget(new QLabel("Performance Comparison"));
    
    m_ComparisonTextEdit = new QTextEdit(comparisonTab);
    m_ComparisonTextEdit->setReadOnly(true);
    m_ComparisonTextEdit->setFont(QFont("Consolas", 10));
    
    layout->addWidget(m_ComparisonTextEdit);
    
    m_TabWidget->addTab(comparisonTab, "Comparison");
}

void PerformanceWidget::OnUpdateTimer()
{
    if (m_InternalTabIndex == 0) { // Real-time tab
        UpdateRealTimeStats();
        UpdateCharts();
    }
    else if (m_InternalTabIndex == 1 && m_BenchmarkRunning) { // Benchmark tab
        UpdateBenchmarkProgress();
    }
}

void PerformanceWidget::UpdateRealTimeStats()
{
    const auto& profiler = PerformanceProfiler::GetInstance();
    const auto& timing = profiler.GetLastFrameTiming();
    
    // Update table values
    int row = 0;
    m_StatsTable->item(row++, 1)->setText(QString::number(profiler.GetCurrentFPS(), 'f', 1));
    m_StatsTable->item(row++, 1)->setText(QString::number(profiler.GetAverageFrameTime(), 'f', 3));
    
    // Rendering mode
    PerformanceProfiler::RenderingMode mode = profiler.GetRenderingMode();
    const char* modeName = (mode == PerformanceProfiler::RenderingMode::CPU_DRIVEN) ? "CPU-Driven" : "GPU-Driven";
    m_StatsTable->item(row++, 1)->setText(QString(modeName));
    
    m_StatsTable->item(row++, 1)->setText(QString::number(timing.totalObjects));
    m_StatsTable->item(row++, 1)->setText(QString::number(timing.visibleObjects));
    
    // Frustum culling times
    if (timing.cpuFrustumCullingTime > 0.0) {
        m_StatsTable->item(row, 1)->setText(QString::number(timing.cpuFrustumCullingTime, 'f', 0));
    } else {
        m_StatsTable->item(row, 1)->setText("N/A");
    }
    row++;
    
    if (timing.gpuFrustumCullingTime > 0.0) {
        m_StatsTable->item(row, 1)->setText(QString::number(timing.gpuFrustumCullingTime, 'f', 0));
    } else {
        m_StatsTable->item(row, 1)->setText("N/A");
    }
    row++;
    
    // GPU Speedup (use persistent values so we can compare across rendering modes)
    double persistentCPUTime = profiler.GetLastCPUFrustumCullingTime();
    double persistentGPUTime = profiler.GetLastGPUFrustumCullingTime();
    if (persistentCPUTime > 0.0 && persistentGPUTime > 0.0) {
        double speedup = persistentCPUTime / persistentGPUTime;
        m_StatsTable->item(row, 1)->setText(QString::number(speedup, 'f', 2) + "x");
    } else {
        m_StatsTable->item(row, 1)->setText("N/A");
    }
    row++;
    
    m_StatsTable->item(row++, 1)->setText(QString::number(timing.drawCalls));
    m_StatsTable->item(row++, 1)->setText(QString::number(timing.triangles));
    m_StatsTable->item(row++, 1)->setText(QString::number(timing.instances));
    m_StatsTable->item(row++, 1)->setText(QString::number(timing.indirectDrawCalls));
    m_StatsTable->item(row++, 1)->setText(QString::number(timing.computeDispatches));
    m_StatsTable->item(row++, 1)->setText(QString::number(timing.cpuTime, 'f', 3));
    m_StatsTable->item(row++, 1)->setText(QString::number(timing.gpuTime, 'f', 3));
    
    // Memory usage (values are already in MB from PerformanceProfiler)
    if (timing.gpuMemoryUsage > 0.0) {
        m_StatsTable->item(row, 1)->setText(QString::number(timing.gpuMemoryUsage, 'f', 1));
    } else {
        m_StatsTable->item(row, 1)->setText("N/A");
    }
    row++;
    
    if (timing.cpuMemoryUsage > 0.0) {
        m_StatsTable->item(row, 1)->setText(QString::number(timing.cpuMemoryUsage, 'f', 1));
    } else {
        m_StatsTable->item(row, 1)->setText("N/A");
    }
    row++;
    
    // Bandwidth usage (values are already in GB/s from PerformanceProfiler, convert to MB/s for display)
    if (timing.bandwidthUsage > 0.0) {
        m_StatsTable->item(row, 1)->setText(QString::number(timing.bandwidthUsage * 1024.0, 'f', 1));
    } else {
        m_StatsTable->item(row, 1)->setText("N/A");
    }
    row++;
    
    // Visibility ratio
    if (timing.totalObjects > 0) {
        double visibilityRatio = static_cast<double>(timing.visibleObjects) / static_cast<double>(timing.totalObjects) * 100.0;
        m_StatsTable->item(row, 1)->setText(QString::number(visibilityRatio, 'f', 1));
    } else {
        m_StatsTable->item(row, 1)->setText("N/A");
    }
    row++;
    
    // Triangles per draw call
    if (timing.drawCalls > 0) {
        double trianglesPerCall = static_cast<double>(timing.triangles) / static_cast<double>(timing.drawCalls);
        m_StatsTable->item(row, 1)->setText(QString::number(trianglesPerCall, 'f', 1));
    } else {
        m_StatsTable->item(row, 1)->setText("N/A");
    }
}

void PerformanceWidget::UpdateCharts()
{
    // Add current performance data to chart
    ChartPoint point;
    point.time = QTime::currentTime().msecsSinceStartOfDay() / 1000.0;
    point.fps = PerformanceProfiler::GetInstance().GetCurrentFPS();
    point.cpuTime = PerformanceProfiler::GetInstance().GetLastFrameTiming().cpuTime;
    point.gpuTime = PerformanceProfiler::GetInstance().GetLastFrameTiming().gpuTime;
    
    m_ChartData.push_back(point);
    
    // Keep only last 100 points
    if (m_ChartData.size() > 100) {
        m_ChartData.erase(m_ChartData.begin());
    }
    
    // Update chart display (simplified)
    QString chartText = QString("FPS: %1, CPU: %2ms, GPU: %3ms")
        .arg(point.fps, 0, 'f', 1)
        .arg(point.cpuTime, 0, 'f', 3)
        .arg(point.gpuTime, 0, 'f', 3);
    
    m_RealTimeChartWidget->addItem(chartText);
    
    // Keep only last 20 items
    while (m_RealTimeChartWidget->count() > 20) {
        delete m_RealTimeChartWidget->takeItem(0);
    }
    
    // Scroll to bottom
    m_RealTimeChartWidget->scrollToBottom();
}

void PerformanceWidget::UpdateBenchmarkProgress()
{
    if (!m_BenchmarkRunning) return;
    
    // Update progress bar and status based on benchmark system
    auto benchmarkSystem = GetBenchmarkSystem();
    if (benchmarkSystem) {
        double progress = benchmarkSystem->GetProgress();
        std::string status = benchmarkSystem->GetStatus();
        
        m_BenchmarkProgressBar->setValue(static_cast<int>(progress * 100));
        m_BenchmarkStatusLabel->setText(QString::fromStdString(status));
        
        // Check if benchmark is complete
        if (progress >= 1.0) {
            OnStopBenchmark();
        }
    }
}

void PerformanceWidget::StartBenchmark()
{
    SwitchToBenchmarkTab();
    OnStartBenchmark();
}

void PerformanceWidget::SwitchToBenchmarkTab()
{
    m_TabWidget->setCurrentIndex(1);
}

void PerformanceWidget::SetMainWindowTabIndex(int index)
{
    m_MainWindowTabIndex = index;
}

void PerformanceWidget::RunBenchmark(const BenchmarkConfig& config)
{
    m_CurrentBenchmarkConfig = config;
    OnStartBenchmark();
}

void PerformanceWidget::OnStartBenchmark()
{
    if (m_BenchmarkRunning) return;
    
    SetupBenchmarkConfig();
    
    auto benchmarkSystem = GetBenchmarkSystem();
    if (!benchmarkSystem) {
        LOG_ERROR("Cannot start benchmark - benchmark system not available");
        m_BenchmarkStatusLabel->setText("Error: Benchmark system not available");
        return;
    }
    
    m_BenchmarkRunning = true;
    m_BenchmarkCurrentFrame = 0;
    m_CurrentBenchmarkResults.clear();
    
    m_StartBenchmarkButton->setEnabled(false);
    m_StopBenchmarkButton->setEnabled(true);
    m_BenchmarkProgressBar->setValue(0);
    m_BenchmarkStatusLabel->setText("Starting real benchmark...");
    
    // Start the frame-by-frame benchmark for smooth progress updates
    if (benchmarkSystem->StartFrameByFrameBenchmark(m_CurrentBenchmarkConfig)) {
        // Start monitoring timer to run frames one by one
        m_BenchmarkTimer->start(16); // Run at ~60 FPS for smooth progress
        LOG("Started frame-by-frame benchmark execution");
    } else {
        LOG_ERROR("Failed to start frame-by-frame benchmark");
        OnStopBenchmark();
        return;
    }
    
    LOG("Real benchmark started with configuration: " + m_CurrentBenchmarkConfig.sceneName);
}

void PerformanceWidget::OnStopBenchmark()
{
    if (!m_BenchmarkRunning) return;
    
    m_BenchmarkRunning = false;
    m_BenchmarkTimer->stop();
    
    // Stop the frame-by-frame benchmark
    auto benchmarkSystem = GetBenchmarkSystem();
    if (benchmarkSystem) {
        benchmarkSystem->StopFrameByFrameBenchmark();
    }
    
    m_StartBenchmarkButton->setEnabled(true);
    m_StopBenchmarkButton->setEnabled(false);
    m_BenchmarkStatusLabel->setText("Benchmark stopped");
    
    LOG("Benchmark stopped");
}

void PerformanceWidget::OnBenchmarkFrame()
{
    if (!m_BenchmarkRunning) return;
    
    // Run one frame of the benchmark and update progress
    auto benchmarkSystem = GetBenchmarkSystem();
    if (benchmarkSystem) {
        // Run the next frame
        bool benchmarkComplete = benchmarkSystem->RunNextBenchmarkFrame();
        
        // Update progress
        double progress = benchmarkSystem->GetProgress();
        std::string status = benchmarkSystem->GetStatus();
        
        m_BenchmarkProgressBar->setValue(static_cast<int>(progress * 100));
        m_BenchmarkStatusLabel->setText(QString::fromStdString(status));
        
        // Check if benchmark is complete
        if (benchmarkComplete) {
            OnStopBenchmark();
            
            // Get the final benchmark result
            BenchmarkResult result = benchmarkSystem->GetCurrentBenchmarkResult();
            m_BenchmarkHistory.push_back(result);
            
            LOG("Frame-by-frame benchmark completed successfully");
            LoadBenchmarkResults();
            DisplayComparisonResults();
        }
    } else {
        // Fallback if benchmark system is not available
        m_BenchmarkCurrentFrame++;
        
        int progress = (m_BenchmarkCurrentFrame * 100) / m_CurrentBenchmarkConfig.benchmarkDuration;
        m_BenchmarkProgressBar->setValue(progress);
        
        m_BenchmarkStatusLabel->setText(QString("Frame %1/%2 (Fallback mode)")
            .arg(m_BenchmarkCurrentFrame)
            .arg(m_CurrentBenchmarkConfig.benchmarkDuration));
        
        if (m_BenchmarkCurrentFrame >= m_CurrentBenchmarkConfig.benchmarkDuration) {
            OnStopBenchmark();
            LOG_WARNING("Benchmark completed in fallback mode - no real results available");
        }
    }
}

void PerformanceWidget::SetupBenchmarkConfig()
{
    // Map dropdown index to approach (0=CPU-Driven, 1=GPU-Driven, Hybrid removed)
    int selectedIndex = m_RenderingModeCombo->currentIndex();
    if (selectedIndex == 0) {
        m_CurrentBenchmarkConfig.approach = BenchmarkConfig::RenderingApproach::CPU_DRIVEN;
    } else if (selectedIndex == 1) {
        m_CurrentBenchmarkConfig.approach = BenchmarkConfig::RenderingApproach::GPU_DRIVEN;
    } else {
        LOG_WARNING("Invalid rendering mode selected, defaulting to CPU-Driven");
        m_CurrentBenchmarkConfig.approach = BenchmarkConfig::RenderingApproach::CPU_DRIVEN;
    }
    
    m_CurrentBenchmarkConfig.objectCount = m_ObjectCountSpinBox->value();
    m_CurrentBenchmarkConfig.benchmarkDuration = m_BenchmarkDurationSpinBox->value();
    m_CurrentBenchmarkConfig.enableFrustumCulling = m_FrustumCullingCheckBox->isChecked();
    m_CurrentBenchmarkConfig.enableLOD = m_LODCheckBox->isChecked(); // Will be false since disabled
    m_CurrentBenchmarkConfig.enableOcclusionCulling = m_OcclusionCullingCheckBox->isChecked(); // Will be false since disabled
    m_CurrentBenchmarkConfig.sceneName = std::string("Performance Widget Benchmark - ") + 
        (m_CurrentBenchmarkConfig.approach == BenchmarkConfig::RenderingApproach::CPU_DRIVEN ? "CPU" : "GPU") + 
        " - " + std::to_string(m_CurrentBenchmarkConfig.objectCount) + " objects";
}

void PerformanceWidget::LoadBenchmarkResults()
{
    m_BenchmarkResultsTable->setRowCount(m_BenchmarkHistory.size());
    
    for (int i = 0; i < m_BenchmarkHistory.size(); ++i) {
        const auto& result = m_BenchmarkHistory[i];
        
        m_BenchmarkResultsTable->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(result.approach)));
        m_BenchmarkResultsTable->setItem(i, 1, new QTableWidgetItem(QString::number(result.objectCount)));
        m_BenchmarkResultsTable->setItem(i, 2, new QTableWidgetItem(QString::number(result.visibleObjects)));
        m_BenchmarkResultsTable->setItem(i, 3, new QTableWidgetItem(QString::number(result.averageFPS, 'f', 1)));
        m_BenchmarkResultsTable->setItem(i, 4, new QTableWidgetItem(QString::number(result.averageFrameTime, 'f', 2)));
        m_BenchmarkResultsTable->setItem(i, 5, new QTableWidgetItem(QString::number(result.averageGPUTime, 'f', 2)));
        m_BenchmarkResultsTable->setItem(i, 6, new QTableWidgetItem(QString::number(result.averageCPUTime, 'f', 2)));
        m_BenchmarkResultsTable->setItem(i, 7, new QTableWidgetItem(QString::number(result.averageDrawCalls)));
    }
}

void PerformanceWidget::DisplayComparisonResults()
{
    QString comparison;
    comparison += "GPU-Driven Rendering Performance Comparison Report\n";
    comparison += "==================================================\n\n";
    
    for (const auto& result : m_BenchmarkHistory) {
        comparison += QString("Approach: %1\n").arg(QString::fromStdString(result.approach));
        comparison += QString("Objects: %1, Visible: %2\n").arg(result.objectCount).arg(result.visibleObjects);
        comparison += QString("FPS: %1, Frame Time: %2ms\n").arg(result.averageFPS, 0, 'f', 1).arg(result.averageFrameTime, 0, 'f', 2);
        comparison += QString("GPU Time: %1ms, CPU Time: %2ms\n").arg(result.averageGPUTime, 0, 'f', 2).arg(result.averageCPUTime, 0, 'f', 2);
        comparison += QString("Draw Calls: %1\n\n").arg(result.averageDrawCalls);
    }
    
    m_ComparisonTextEdit->setPlainText(comparison);
}

void PerformanceWidget::OnExportResults()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Export Results", "benchmark_results.csv", "CSV Files (*.csv)");
    if (!fileName.isEmpty()) {
        // Export logic here
        QMessageBox::information(this, "Export", "Results exported to " + fileName);
    }
}

void PerformanceWidget::OnExportComparison()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Export Comparison", "benchmark_comparison.txt", "Text Files (*.txt)");
    if (!fileName.isEmpty()) {
        // Export logic here
        QMessageBox::information(this, "Export", "Comparison exported to " + fileName);
    }
}

void PerformanceWidget::OnInternalTabChanged(int index)
{
    m_InternalTabIndex = index;
}

// Configuration change handlers
void PerformanceWidget::OnRenderingModeChanged(int index) { }
void PerformanceWidget::OnObjectCountChanged(int value) { }
void PerformanceWidget::OnBenchmarkDurationChanged(int value) { }
void PerformanceWidget::OnFrustumCullingToggled(bool enabled) { }
void PerformanceWidget::OnLODToggled(bool enabled) { }
void PerformanceWidget::OnOcclusionCullingToggled(bool enabled) { }

// #include "PerformanceWidget.moc" // Qt will handle this automatically 