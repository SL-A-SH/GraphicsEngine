#ifndef _PERFORMANCE_WIDGET_H_
#define _PERFORMANCE_WIDGET_H_

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QTableWidget>
#include <QTimer>
//#include <QPainter>
//
//#include <QtCharts/QChart>
//#include <QtCharts/QChartView>
//#include <QtCharts/QLineSeries>
//#include <QtCharts/QValueAxis>

class PerformanceWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PerformanceWidget(QWidget* parent = nullptr);
    ~PerformanceWidget();

    //void UpdateData();

private slots:
    void OnUpdateTimer();

private:
    void CreateStatsTable();
    void UpdateStatsTable();

    /*QChart* m_FPSChart;
    QChart* m_GPUTimeChart;
    QChart* m_CPUTimeChart;
    QLineSeries* m_FPSSeries;
    QLineSeries* m_GPUTimeSeries;
    QLineSeries* m_CPUTimeSeries;
    QTableWidget* m_StatsTable;
    QTimer* m_UpdateTimer;*/

    QTableWidget* m_StatsTable;
    QTimer* m_UpdateTimer;
    static const int UPDATE_INTERVAL_MS = 1000; // Update once per second instead of every frame
};

#endif