#ifndef PERFORMANCE_WIDGET_H
#define PERFORMANCE_WIDGET_H

#include <d3d11.h>
#include <memory>
// #include <imgui.h> // ImGui not available, using simplified interface

// Forward declarations
class RenderingBenchmark;

// Include BenchmarkConfig definition
#include "../../Core/System/RenderingBenchmark.h"

class PerformanceWidget
{
public:
    PerformanceWidget();
    ~PerformanceWidget();

    // Initialization and cleanup
    bool Initialize(ID3D11Device* device, ID3D11DeviceContext* context, HWND hwnd);
    void Shutdown();

    // Main interface
    void Update(float deltaTime);
    void Render();

private:
    // Rendering methods
    void RenderCurrentMetrics();
    void RenderBenchmarkControls();
    void RenderDetailedMetrics();
    void LogCurrentMetrics();

    // Benchmark operations
    void RunSingleBenchmark();
    void RunBenchmarkSuite();

private:
    // Core state
    bool m_isInitialized;
    
    // Benchmark system
    std::unique_ptr<RenderingBenchmark> m_benchmarkSystem;
    bool m_runningBenchmark;
    
    // UI state
    bool m_showDetailedMetrics;
    
    // Configuration
    BenchmarkConfig m_benchmarkConfig;
};

#endif // PERFORMANCE_WIDGET_H