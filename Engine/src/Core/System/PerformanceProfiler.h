#ifndef _PERFORMANCE_PROFILER_H_
#define _PERFORMANCE_PROFILER_H_

#include <d3d11.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <stack>
#include <functional>

class PerformanceProfiler
{
public:
    // Enhanced timing data structure
    struct TimingData
    {
        double cpuTime;    // CPU time in milliseconds
        double gpuTime;    // GPU time in milliseconds
        uint32_t drawCalls;
        uint32_t triangles;
        uint32_t vertices;
        uint32_t instances;  // Number of instances rendered
        uint32_t indirectDrawCalls;  // Number of indirect draw calls
        uint32_t computeDispatches;  // Number of compute shader dispatches
        double gpuMemoryUsage;  // GPU memory usage in MB
        double cpuMemoryUsage;  // CPU memory usage in MB
        double bandwidthUsage;  // Memory bandwidth usage in GB/s
        std::unordered_map<std::string, TimingData> sections;
    };

    // Rendering mode for comparison
    enum class RenderingMode
    {
        CPU_DRIVEN,     // Traditional CPU-driven rendering
        GPU_DRIVEN,     // GPU-driven rendering with compute shaders
        HYBRID          // Mixed approach
    };

    // Benchmark configuration
    struct BenchmarkConfig
    {
        RenderingMode mode;
        uint32_t objectCount;
        uint32_t benchmarkDuration;  // Duration in frames
        bool enableFrustumCulling;
        bool enableLOD;
        bool enableOcclusionCulling;
        std::string sceneName;
    };

    // Benchmark results
    struct BenchmarkResults
    {
        BenchmarkConfig config;
        double averageFPS;
        double averageFrameTime;
        double averageGPUTime;
        double averageCPUTime;
        double averageDrawCalls;
        double averageTriangles;
        double averageInstances;
        double averageIndirectDrawCalls;
        double averageComputeDispatches;
        double averageGPUMemoryUsage;
        double averageCPUMemoryUsage;
        double averageBandwidthUsage;
        double minFPS;
        double maxFPS;
        double fpsVariance;
        std::vector<double> frameTimes;  // Raw frame time data
        std::vector<double> gpuTimes;    // Raw GPU time data
        std::vector<double> cpuTimes;    // Raw CPU time data
    };

    struct SectionData
    {
        std::string name;
        TimingData timing;
        double startTime;  // Duration since epoch in milliseconds
        ID3D11Query* startQuery;
        ID3D11Query* endQuery;
    };

    struct FrameData
    {
        TimingData timing;
        double timestamp;  // Duration since epoch in milliseconds
        RenderingMode mode;
        std::unordered_map<std::string, TimingData> sections;
    };

    static PerformanceProfiler& GetInstance();

    void Initialize(ID3D11Device* device, ID3D11DeviceContext* context);
    void Shutdown();

    // Start/End timing for a specific section
    void BeginSection(const std::string& name);
    void EndSection();

    // Frame timing
    void BeginFrame();
    void EndFrame();

    // Get timing data
    const TimingData& GetLastFrameTiming() const { return m_LastFrameTiming; }
    const std::vector<FrameData>& GetFrameHistory() const { return m_FrameHistory; }
    const std::unordered_map<std::string, TimingData>& GetLastFrameSections() const { return m_LastFrameTiming.sections; }
    double GetAverageFPS() const;
    double GetAverageFrameTime() const;
    double GetCurrentFPS() const; // Get current FPS matching Timer class calculation

    // Enable/Disable profiling
    void SetEnabled(bool enabled) { m_Enabled = enabled; }
    bool IsEnabled() const { return m_Enabled; }

    // Draw call tracking
    void IncrementDrawCalls() { m_LastFrameTiming.drawCalls++; }
    void AddTriangles(uint32_t count) { m_LastFrameTiming.triangles += count; }
    void AddVertices(uint32_t count) { m_LastFrameTiming.vertices += count; }
    void AddInstances(uint32_t count) { m_LastFrameTiming.instances += count; }
    void IncrementIndirectDrawCalls() { m_LastFrameTiming.indirectDrawCalls++; }
    void IncrementComputeDispatches() { m_LastFrameTiming.computeDispatches++; }

    // Memory tracking
    void SetGPUMemoryUsage(double usageMB) { m_LastFrameTiming.gpuMemoryUsage = usageMB; }
    void SetCPUMemoryUsage(double usageMB) { m_LastFrameTiming.cpuMemoryUsage = usageMB; }
    void SetBandwidthUsage(double usageGBs) { m_LastFrameTiming.bandwidthUsage = usageGBs; }

    // Rendering mode management
    void SetRenderingMode(RenderingMode mode) { m_CurrentMode = mode; }
    RenderingMode GetRenderingMode() const { return m_CurrentMode; }

    // Benchmarking system
    void StartBenchmark(const BenchmarkConfig& config);
    void StopBenchmark();
    bool IsBenchmarkRunning() const { return m_BenchmarkRunning; }
    const BenchmarkResults& GetLastBenchmarkResults() const { return m_LastBenchmarkResults; }
    
    // Get benchmark progress
    double GetBenchmarkProgress() const;
    uint32_t GetBenchmarkCurrentFrame() const { return m_BenchmarkCurrentFrame; }
    uint32_t GetBenchmarkTotalFrames() const { return m_BenchmarkConfig.benchmarkDuration; }

    // Comparison utilities
    BenchmarkResults CompareBenchmarks(const BenchmarkResults& cpuResults, const BenchmarkResults& gpuResults);
    double CalculatePerformanceImprovement(const BenchmarkResults& cpuResults, const BenchmarkResults& gpuResults);
    
    // Export functionality
    bool ExportBenchmarkResults(const std::string& filename, const BenchmarkResults& results);
    bool ExportComparisonReport(const std::string& filename, const BenchmarkResults& cpuResults, const BenchmarkResults& gpuResults);

    // Real-time monitoring
    void SetRealTimeCallback(std::function<void(const TimingData&)> callback) { m_RealTimeCallback = callback; }

private:
    PerformanceProfiler();
    ~PerformanceProfiler();

    // Prevent copying
    PerformanceProfiler(const PerformanceProfiler&) = delete;
    PerformanceProfiler& operator=(const PerformanceProfiler&) = delete;

    void CreateQueryObjects();
    void ReleaseQueryObjects();
    ID3D11Query* CreateTimestampQuery();
    void ReleaseTimestampQuery(ID3D11Query* query);

    // Memory monitoring
    void UpdateMemoryUsage();
    double GetGPUMemoryUsage();
    double GetCPUMemoryUsage();

    // Benchmark helpers
    void ProcessBenchmarkFrame();
    void CalculateBenchmarkStatistics(BenchmarkResults& results);
    double CalculateVariance(const std::vector<double>& values, double mean);

    ID3D11Device* m_Device;
    ID3D11DeviceContext* m_Context;
    ID3D11Query* m_DisjointQuery;
    ID3D11Query* m_TimestampStartQuery;
    ID3D11Query* m_TimestampEndQuery;

    bool m_Enabled;
    bool m_QueryInFlight;
    TimingData m_LastFrameTiming;
    std::vector<FrameData> m_FrameHistory;
    std::stack<SectionData> m_ActiveSections;
    double m_FrameStartTime;  // Duration since epoch in milliseconds

    // Rendering mode tracking
    RenderingMode m_CurrentMode;

    // Benchmarking system
    bool m_BenchmarkRunning;
    BenchmarkConfig m_BenchmarkConfig;
    BenchmarkResults m_LastBenchmarkResults;
    uint32_t m_BenchmarkCurrentFrame;
    std::vector<FrameData> m_BenchmarkFrameData;

    // Real-time monitoring
    std::function<void(const TimingData&)> m_RealTimeCallback;

    // Timing consistency with Timer class
    double m_Frequency;
    double m_LastFrameTime;

    static const size_t MAX_FRAME_HISTORY = 300; // Store 5 seconds at 60 FPS
    static const size_t MAX_BENCHMARK_FRAMES = 3000; // Store up to 50 seconds at 60 FPS
};

#endif