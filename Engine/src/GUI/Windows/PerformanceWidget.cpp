#include "PerformanceWidget.h"
#include <QDateTime>
#include "../../Core/System/PerformanceProfiler.h"

PerformanceWidget::PerformanceWidget(QWidget* parent)
    : QWidget(parent)
    , m_StatsTable(nullptr)
    , m_UpdateTimer(nullptr)
{
    // Create layout
    QVBoxLayout* layout = new QVBoxLayout(this);
    setLayout(layout);

    // Create stats table
    CreateStatsTable();

    // Create update timer with a slower update rate
    m_UpdateTimer = new QTimer(this);
    connect(m_UpdateTimer, &QTimer::timeout, this, &PerformanceWidget::OnUpdateTimer);
    m_UpdateTimer->start(UPDATE_INTERVAL_MS);
}

PerformanceWidget::~PerformanceWidget()
{
    if (m_UpdateTimer)
    {
        m_UpdateTimer->stop();
        delete m_UpdateTimer;
    }
}

void PerformanceWidget::CreateStatsTable()
{
    m_StatsTable = new QTableWidget(this);
    m_StatsTable->setColumnCount(2);
    m_StatsTable->setHorizontalHeaderLabels({"Metric", "Value"});
    m_StatsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_StatsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_StatsTable->verticalHeader()->setVisible(false);
    m_StatsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout()->addWidget(m_StatsTable);
}

void PerformanceWidget::UpdateStatsTable()
{
    const auto& lastFrame = PerformanceProfiler::GetInstance().GetLastFrameTiming();
    
    m_StatsTable->setRowCount(5);
    
    // FPS
    m_StatsTable->setItem(0, 0, new QTableWidgetItem("FPS"));
    m_StatsTable->setItem(0, 1, new QTableWidgetItem(QString::number(1000.0 / lastFrame.cpuTime, 'f', 1)));

    // CPU Time
    m_StatsTable->setItem(1, 0, new QTableWidgetItem("CPU Time (ms)"));
    m_StatsTable->setItem(1, 1, new QTableWidgetItem(QString::number(lastFrame.cpuTime, 'f', 2)));

    // GPU Time
    m_StatsTable->setItem(2, 0, new QTableWidgetItem("GPU Time (ms)"));
    m_StatsTable->setItem(2, 1, new QTableWidgetItem(QString::number(lastFrame.gpuTime, 'f', 2)));

    // Draw Calls
    m_StatsTable->setItem(3, 0, new QTableWidgetItem("Draw Calls"));
    m_StatsTable->setItem(3, 1, new QTableWidgetItem(QString::number(lastFrame.drawCalls)));

    // Triangles
    m_StatsTable->setItem(4, 0, new QTableWidgetItem("Triangles"));
    m_StatsTable->setItem(4, 1, new QTableWidgetItem(QString::number(lastFrame.triangles)));
}

void PerformanceWidget::OnUpdateTimer()
{
    UpdateStatsTable();
} 