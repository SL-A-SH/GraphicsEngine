#include "PerformanceWidget.h"
#include "../../Core/System/PerformanceProfiler.h"
#include "../../Core/System/RenderingBenchmark.h"
#include "../../Core/System/Logger.h"
#include <string>
#include <sstream>
#include <iomanip>
// #include <imgui.h> // ImGui not available, using simplified console output

PerformanceWidget::PerformanceWidget()
    : m_isInitialized(false)
    , m_benchmarkSystem(nullptr)
    , m_runningBenchmark(false)
    , m_showDetailedMetrics(false)
    , m_benchmarkConfig()
{
    // Initialize benchmark configuration
    m_benchmarkConfig.approach = BenchmarkConfig::RenderingApproach::CPU_DRIVEN;
    m_benchmarkConfig.objectCount = 1000;
    m_benchmarkConfig.benchmarkDuration = 300;
    m_benchmarkConfig.enableFrustumCulling = true;
    m_benchmarkConfig.enableLOD = false;
    m_benchmarkConfig.enableOcclusionCulling = false;
    m_benchmarkConfig.sceneName = "Performance Test Scene";
    m_benchmarkConfig.outputDirectory = "./benchmark_results/";
}

PerformanceWidget::~PerformanceWidget()
{
    // Cleanup handled by unique_ptr
}

bool PerformanceWidget::Initialize(ID3D11Device* device, ID3D11DeviceContext* context, HWND hwnd)
{
    if (!device || !context || !hwnd)
    {
        LOG_ERROR("PerformanceWidget::Initialize - Invalid parameters");
        return false;
    }

    // Create and initialize the benchmark system
    m_benchmarkSystem = std::make_unique<RenderingBenchmark>();
    
    bool result = m_benchmarkSystem->Initialize(device, context, hwnd);
    if (!result)
    {
        LOG_ERROR("PerformanceWidget::Initialize - Failed to initialize benchmark system");
        m_benchmarkSystem.reset();
        return false;
    }

    m_isInitialized = true;
    LOG("PerformanceWidget initialized successfully");
    return true;
}

void PerformanceWidget::Update(float deltaTime)
{
    if (!m_isInitialized)
        return;

    // Update benchmark progress if running
    if (m_runningBenchmark && m_benchmarkSystem)
    {
        // Check if benchmark is complete
        double progress = m_benchmarkSystem->GetProgress();
        if (progress >= 1.0)
        {
            m_runningBenchmark = false;
            LOG("Benchmark completed");
        }
    }
}

void PerformanceWidget::Render()
{
    if (!m_isInitialized)
        return;

    // Simplified rendering - just log current metrics periodically
    static int frameCounter = 0;
    frameCounter++;
    
    // Log performance metrics every 60 frames (approximately once per second at 60 FPS)
    if (frameCounter % 60 == 0)
    {
        LogCurrentMetrics();
    }
}

void PerformanceWidget::LogCurrentMetrics()
{
    const auto& profiler = PerformanceProfiler::GetInstance();
    const auto& timing = profiler.GetLastFrameTiming();
    
    std::stringstream ss;
    ss << std::fixed << std::setprecision(1);
    
    ss << "=== PERFORMANCE METRICS ===\n";
    ss << "FPS: " << profiler.GetCurrentFPS() << "\n";
    ss << "Frame Time: " << std::setprecision(3) << profiler.GetAverageFrameTime() << " ms\n";
    
    // Rendering mode
    PerformanceProfiler::RenderingMode mode = profiler.GetRenderingMode();
    const char* modeName = (mode == PerformanceProfiler::RenderingMode::CPU_DRIVEN) ? "CPU-Driven" : "GPU-Driven";
    ss << "Rendering Mode: " << modeName << "\n";
    
    // GPU vs CPU frustum culling times
    if (timing.cpuFrustumCullingTime > 0.0)
    {
        ss << "CPU Frustum Culling: " << std::setprecision(0) << timing.cpuFrustumCullingTime << " μs\n";
    }
    
    if (timing.gpuFrustumCullingTime > 0.0)
    {
        ss << "GPU Frustum Culling: " << std::setprecision(0) << timing.gpuFrustumCullingTime << " μs\n";
    }
    
    // Object counts
    ss << "Total Objects: " << timing.totalObjects << "\n";
    ss << "Visible Objects: " << timing.visibleObjects << "\n";
    
    // Rendering statistics
    ss << "Draw Calls: " << timing.drawCalls << "\n";
    ss << "Triangles: " << timing.triangles << "\n";
    ss << "Instances: " << timing.instances << "\n";
    
    // Performance comparison (if both CPU and GPU times are available)
    if (timing.cpuFrustumCullingTime > 0.0 && timing.gpuFrustumCullingTime > 0.0)
    {
        double speedup = timing.cpuFrustumCullingTime / timing.gpuFrustumCullingTime;
        ss << "GPU Speedup: " << std::setprecision(2) << speedup << "x\n";
    }
    
    ss << "===========================";
    
    std::string logMessage = ss.str();
    LOG(logMessage);
}

void PerformanceWidget::RenderCurrentMetrics()
{
    // Simplified version - no ImGui, just console logging
    LogCurrentMetrics();
}

void PerformanceWidget::RenderBenchmarkControls()
{
    // Simplified version - no ImGui interface for now
    if (!m_benchmarkSystem)
    {
        LOG("Benchmark system not available");
        return;
    }
    
    if (m_runningBenchmark)
    {
        double progress = m_benchmarkSystem->GetProgress();
        std::string status = m_benchmarkSystem->GetStatus();
        
        std::stringstream ss;
        ss << "Benchmark Progress: " << std::fixed << std::setprecision(1) << (progress * 100.0) << "%";
        ss << " - " << status;
        LOG(ss.str());
    }
}

void PerformanceWidget::RenderDetailedMetrics()
{
    // Simplified version - no ImGui, just console logging
    const auto& profiler = PerformanceProfiler::GetInstance();
    const auto& timing = profiler.GetLastFrameTiming();
    
    std::stringstream ss;
    ss << std::fixed << std::setprecision(3);
    
    ss << "=== DETAILED PERFORMANCE METRICS ===\n";
    
    // Timing details
    ss << "CPU Time: " << timing.cpuTime << " ms\n";
    ss << "GPU Time: " << timing.gpuTime << " ms\n";
    ss << "Frame Time: " << profiler.GetAverageFrameTime() << " ms\n";
    
    if (timing.cpuFrustumCullingTime > 0.0)
    {
        ss << "CPU Frustum Culling: " << std::setprecision(0) << timing.cpuFrustumCullingTime << " μs\n";
    }
    
    if (timing.gpuFrustumCullingTime > 0.0)
    {
        ss << "GPU Frustum Culling: " << std::setprecision(0) << timing.gpuFrustumCullingTime << " μs\n";
    }
    
    // Rendering details
    ss << "Draw Calls: " << timing.drawCalls << "\n";
    ss << "Indirect Draw Calls: " << timing.indirectDrawCalls << "\n";
    ss << "Compute Dispatches: " << timing.computeDispatches << "\n";
    ss << "Triangles: " << timing.triangles << "\n";
    ss << "Instances: " << timing.instances << "\n";
    ss << "Total Objects: " << timing.totalObjects << "\n";
    ss << "Visible Objects: " << timing.visibleObjects << "\n";
    
    // Memory details
    if (timing.gpuMemoryUsage > 0.0)
    {
        ss << "GPU Memory: " << std::setprecision(1) << (timing.gpuMemoryUsage / (1024.0 * 1024.0)) << " MB\n";
    }
    
    if (timing.cpuMemoryUsage > 0.0)
    {
        ss << "CPU Memory: " << std::setprecision(1) << (timing.cpuMemoryUsage / (1024.0 * 1024.0)) << " MB\n";
    }
    
    if (timing.bandwidthUsage > 0.0)
    {
        ss << "Bandwidth: " << std::setprecision(1) << (timing.bandwidthUsage / (1024.0 * 1024.0)) << " MB/s\n";
    }
    
    ss << "=====================================";
    
    std::string logMessage = ss.str();
    LOG(logMessage);
}

void PerformanceWidget::RunSingleBenchmark()
{
    if (!m_benchmarkSystem || m_runningBenchmark)
        return;
    
    LOG("Starting single benchmark: " + m_benchmarkConfig.sceneName);
    m_runningBenchmark = true;
    
    // Note: In a real implementation, you might want to run this on a separate thread
    // For now, we'll just set the flag and let the benchmark system handle it
}

void PerformanceWidget::RunBenchmarkSuite()
{
    if (!m_benchmarkSystem || m_runningBenchmark)
        return;
    
    LOG("Starting benchmark suite");
    m_runningBenchmark = true;
    
    // Note: In a real implementation, you might want to run this on a separate thread
    // For now, we'll just set the flag and let the benchmark system handle it
}

void PerformanceWidget::Shutdown()
{
    if (m_benchmarkSystem)
    {
        m_benchmarkSystem.reset();
    }
    
    m_isInitialized = false;
    LOG("PerformanceWidget shut down");
} 