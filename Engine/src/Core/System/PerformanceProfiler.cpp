#include "PerformanceProfiler.h"
#include "Logger.h"
#include "CommonTimer.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <numeric>
#include <psapi.h>
#include <pdh.h>

PerformanceProfiler& PerformanceProfiler::GetInstance()
{
    static PerformanceProfiler instance;
    return instance;
}

PerformanceProfiler::PerformanceProfiler()
    : m_Device(nullptr)
    , m_Context(nullptr)
    , m_DisjointQuery(nullptr)
    , m_TimestampStartQuery(nullptr)
    , m_TimestampEndQuery(nullptr)
    , m_Enabled(false)
    , m_QueryInFlight(false)
    , m_CurrentMode(PerformanceProfiler::RenderingMode::CPU_DRIVEN)
    , m_BenchmarkRunning(false)
    , m_BenchmarkCurrentFrame(0)
    , m_Frequency(0.0)
    , m_LastFrameTime(0.0)
    , m_LastCPUFrustumCullingTime(0.0)
    , m_LastGPUFrustumCullingTime(0.0)
{
    // Initialize TimingData with all fields set to zero
    m_LastFrameTiming = {};
    m_LastFrameTiming.cpuTime = 0.0;
    m_LastFrameTiming.gpuTime = 0.0;
    m_LastFrameTiming.drawCalls = 0;
    m_LastFrameTiming.triangles = 0;
    m_LastFrameTiming.vertices = 0;
    m_LastFrameTiming.instances = 0;
    m_LastFrameTiming.indirectDrawCalls = 0;
    m_LastFrameTiming.computeDispatches = 0;
    m_LastFrameTiming.gpuMemoryUsage = 0.0;
    m_LastFrameTiming.cpuMemoryUsage = 0.0;
    m_LastFrameTiming.bandwidthUsage = 0.0;
    m_LastFrameTiming.cpuFrustumCullingTime = 0.0;
    m_LastFrameTiming.gpuFrustumCullingTime = 0.0;
    m_LastFrameTiming.totalObjects = 0;
    m_LastFrameTiming.visibleObjects = 0;
    m_LastFrameTiming.gpuUtilization = 0.0;
    m_LastFrameTiming.memoryThroughput = 0.0;
    m_LastFrameTiming.cullingEfficiency = 0.0;
    m_LastFrameTiming.frustumCullingSpeedup = 0.0;
    m_LastFrameTiming.renderingEfficiency = 0.0;
    m_LastFrameTiming.drawCallEfficiency = 0.0;
    
    // Initialize QueryPerformanceCounter frequency for consistent timing
    LARGE_INTEGER freq;
    if (QueryPerformanceFrequency(&freq))
    {
        m_Frequency = static_cast<double>(freq.QuadPart);
    }
}

PerformanceProfiler::~PerformanceProfiler()
{
    Shutdown();
}

void PerformanceProfiler::Initialize(ID3D11Device* device, ID3D11DeviceContext* context)
{
    m_Device = device;
    m_Context = context;
    CreateQueryObjects();
    m_Enabled = true;
    LOG("Enhanced performance profiler initialized");
}

void PerformanceProfiler::Shutdown()
{
    if (m_BenchmarkRunning)
    {
        StopBenchmark();
    }
    
    ReleaseQueryObjects();
    m_Device = nullptr;
    m_Context = nullptr;
    m_Enabled = false;
    LOG("Performance profiler shut down");
}

void PerformanceProfiler::CreateQueryObjects()
{
    // Create disjoint query for GPU timing
    D3D11_QUERY_DESC disjointDesc = {};
    disjointDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
    m_Device->CreateQuery(&disjointDesc, &m_DisjointQuery);

    // Create timestamp queries
    D3D11_QUERY_DESC timestampDesc = {};
    timestampDesc.Query = D3D11_QUERY_TIMESTAMP;
    m_Device->CreateQuery(&timestampDesc, &m_TimestampStartQuery);
    m_Device->CreateQuery(&timestampDesc, &m_TimestampEndQuery);
}

void PerformanceProfiler::ReleaseQueryObjects()
{
    if (m_DisjointQuery) m_DisjointQuery->Release();
    if (m_TimestampStartQuery) m_TimestampStartQuery->Release();
    if (m_TimestampEndQuery) m_TimestampEndQuery->Release();
}

void PerformanceProfiler::BeginFrame()
{
    if (!m_Enabled) return;

    // Use CommonTimer for consistent timing
    m_FrameStartTime = CommonTimer::GetInstance().GetCurrentTimeMs();
    
    // Reset frame timing data (preserve CPU memory from previous frame for continuity)
    double previousCPUMemory = m_LastFrameTiming.cpuMemoryUsage;
    m_LastFrameTiming = { 0.0, 0.0, 0, 0, 0, 0, 0, 0, 0.0, previousCPUMemory, 0.0, 0.0, 0.0, 0, 0 };

    // Begin GPU timing
    if (!m_QueryInFlight)
    {
        m_Context->Begin(m_DisjointQuery);
        m_Context->End(m_TimestampStartQuery);
        m_QueryInFlight = true;
    }
}

void PerformanceProfiler::EndFrame()
{
    if (!m_Enabled) return;

    // End GPU timing
    if (m_QueryInFlight)
    {
        m_Context->End(m_TimestampEndQuery);
        m_Context->End(m_DisjointQuery);
        m_QueryInFlight = false;
    }

    // Get GPU timing results
    D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjointData;
    while (m_Context->GetData(m_DisjointQuery, &disjointData, sizeof(disjointData), 0) == S_FALSE)
    {
        // Wait for the query to complete
    }

    if (!disjointData.Disjoint)
    {
        UINT64 startTime, endTime;
        m_Context->GetData(m_TimestampStartQuery, &startTime, sizeof(startTime), 0);
        m_Context->GetData(m_TimestampEndQuery, &endTime, sizeof(endTime), 0);

        // Convert to milliseconds
        m_LastFrameTiming.gpuTime = (static_cast<double>(endTime) - static_cast<double>(startTime)) * 1000.0 / disjointData.Frequency;
    }

    // Calculate CPU time using CommonTimer for consistency
    double frameEndTime = CommonTimer::GetInstance().GetCurrentTimeMs();
    m_LastFrameTiming.cpuTime = frameEndTime - m_FrameStartTime;
    m_LastFrameTime = m_LastFrameTiming.cpuTime;

    // Update memory usage at end of frame when we have rendering data
    UpdateMemoryUsage();
    
    // Calculate efficiency metrics with current frame data
    CalculateEfficiencyMetrics();

    // Store frame data
    FrameData frameData;
    frameData.timing = m_LastFrameTiming;
    frameData.timestamp = frameEndTime;
    frameData.mode = m_CurrentMode;
    m_FrameHistory.push_back(frameData);

    // Keep only the last MAX_FRAME_HISTORY frames
    if (m_FrameHistory.size() > MAX_FRAME_HISTORY)
    {
        m_FrameHistory.erase(m_FrameHistory.begin());
    }

    // Process benchmark if running
    if (m_BenchmarkRunning)
    {
        ProcessBenchmarkFrame();
    }

    // Call real-time callback if set
    if (m_RealTimeCallback)
    {
        m_RealTimeCallback(m_LastFrameTiming);
    }
}

void PerformanceProfiler::BeginSection(const std::string& name)
{
    if (!m_Enabled) return;

    SectionData section;
    section.name = name;
    section.timing = { 0.0, 0.0, 0, 0, 0, 0, 0, 0, 0.0, 0.0, 0.0 };
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    section.startTime = static_cast<double>(duration.count());

    // Create GPU timestamp queries for this section
    section.startQuery = CreateTimestampQuery();
    section.endQuery = CreateTimestampQuery();

    // Record GPU timestamp
    m_Context->End(section.startQuery);

    m_ActiveSections.push(section);
}

void PerformanceProfiler::EndSection()
{
    if (!m_Enabled || m_ActiveSections.empty()) return;

    SectionData& section = m_ActiveSections.top();

    // Record end GPU timestamp
    m_Context->End(section.endQuery);

    // Calculate CPU time
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    double cpuEndTime = static_cast<double>(duration.count());
    section.timing.cpuTime = cpuEndTime - section.startTime;

    // Get GPU timing results
    UINT64 gpuStartTime, gpuEndTime;
    while (m_Context->GetData(section.startQuery, &gpuStartTime, sizeof(gpuStartTime), 0) == S_FALSE)
    {
        // Wait for the query to complete
    }
    while (m_Context->GetData(section.endQuery, &gpuEndTime, sizeof(gpuEndTime), 0) == S_FALSE)
    {
        // Wait for the query to complete
    }

    // Get the frequency from the disjoint query
    D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjointData;
    while (m_Context->GetData(m_DisjointQuery, &disjointData, sizeof(disjointData), 0) == S_FALSE)
    {
        // Wait for the query to complete
    }

    if (!disjointData.Disjoint)
    {
        section.timing.gpuTime = (static_cast<double>(gpuEndTime) - static_cast<double>(gpuStartTime)) * 1000.0 / disjointData.Frequency;
    }

    // Store section timing in the current frame
    m_LastFrameTiming.sections[section.name] = section.timing;

    // Clean up queries
    ReleaseTimestampQuery(section.startQuery);
    ReleaseTimestampQuery(section.endQuery);

    m_ActiveSections.pop();
}

ID3D11Query* PerformanceProfiler::CreateTimestampQuery()
{
    ID3D11Query* query = nullptr;
    D3D11_QUERY_DESC desc = {};
    desc.Query = D3D11_QUERY_TIMESTAMP;
    m_Device->CreateQuery(&desc, &query);
    return query;
}

void PerformanceProfiler::ReleaseTimestampQuery(ID3D11Query* query)
{
    if (query)
    {
        query->Release();
    }
}

void PerformanceProfiler::UpdateMemoryUsage()
{
    // Update CPU memory usage
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc)))
    {
        m_LastFrameTiming.cpuMemoryUsage = static_cast<double>(pmc.WorkingSetSize) / (1024.0 * 1024.0); // Convert to MB
    }

    // Calculate GPU memory usage based on actual resources
    // More accurate estimation considering instanced vs individual rendering
    double estimatedGPUMemory = 0.0;
    
    // Base memory for vertex/index buffers (based on triangles, not draw calls)
    estimatedGPUMemory += m_LastFrameTiming.triangles * 0.001; // ~1KB per triangle for vertex data
    
    // Memory for textures - use instances count for more accurate estimation
    // Each instance needs texture memory, regardless of how it's drawn
    if (m_LastFrameTiming.instances > 0) {
        // Estimate texture memory per instance (diffuse, normal, metallic, roughness, etc.)
        estimatedGPUMemory += m_LastFrameTiming.instances * 0.5; // ~0.5MB per instance for all textures
    } else {
        // Fallback: estimate based on draw calls for non-instanced rendering
        estimatedGPUMemory += m_LastFrameTiming.drawCalls * 2.0; // ~2MB per draw call for textures
    }
    
    // Memory for render targets, depth buffers, and compute buffers
    estimatedGPUMemory += 50.0; // Base memory for render targets
    
    // Additional memory for GPU-driven rendering (compute buffers, etc.)
    if (m_CurrentMode == RenderingMode::GPU_DRIVEN && m_LastFrameTiming.computeDispatches > 0) {
        estimatedGPUMemory += 10.0; // Additional memory for compute shader buffers
    }
    
    m_LastFrameTiming.gpuMemoryUsage = estimatedGPUMemory;
    
    // Calculate bandwidth usage (rough estimate)
    // Based on triangles rendered and memory transfers
    double bandwidthEstimate = 0.0;
    if (m_LastFrameTiming.triangles > 0) {
        // Bandwidth for vertex data transfers
        bandwidthEstimate += (m_LastFrameTiming.triangles * 0.0001); // GB/s
        
        // Additional bandwidth for texture streaming
        if (m_LastFrameTiming.instances > 0) {
            bandwidthEstimate += (m_LastFrameTiming.instances * 0.0001); // GB/s for texture bandwidth
        } else {
            bandwidthEstimate += (m_LastFrameTiming.drawCalls * 0.001); // GB/s fallback
        }
    }
    
    m_LastFrameTiming.bandwidthUsage = bandwidthEstimate;
}

void PerformanceProfiler::ResetFrameCounters()
{
    // Reset counters that accumulate during a frame
    m_LastFrameTiming.drawCalls = 0;
    m_LastFrameTiming.triangles = 0;
    m_LastFrameTiming.instances = 0;
    m_LastFrameTiming.indirectDrawCalls = 0;
    m_LastFrameTiming.computeDispatches = 0;
    // Don't reset timing values, visible/total objects, or efficiency metrics
}

void PerformanceProfiler::CalculateEfficiencyMetrics()
{
    // 1. Culling Efficiency (Visible/Total objects ratio)
    if (m_LastFrameTiming.totalObjects > 0)
    {
        m_LastFrameTiming.cullingEfficiency = static_cast<double>(m_LastFrameTiming.visibleObjects) / static_cast<double>(m_LastFrameTiming.totalObjects);
    }
    else
    {
        m_LastFrameTiming.cullingEfficiency = 1.0; // No culling needed
    }
    
    // 2. Frustum Culling Speedup (GPU vs CPU timing comparison)
    if (m_LastCPUFrustumCullingTime > 0.0 && m_LastFrameTiming.gpuFrustumCullingTime > 0.0)
    {
        m_LastFrameTiming.frustumCullingSpeedup = m_LastCPUFrustumCullingTime / m_LastFrameTiming.gpuFrustumCullingTime;
    }
    else if (m_LastCPUFrustumCullingTime > 0.0)
    {
        m_LastFrameTiming.frustumCullingSpeedup = 1.0; // No speedup data available
    }
    else
    {
        m_LastFrameTiming.frustumCullingSpeedup = 0.0; // No comparison data
    }
    
    // 3. Rendering Efficiency (Triangles per millisecond)
    if (m_LastFrameTiming.cpuTime > 0.0)
    {
        m_LastFrameTiming.renderingEfficiency = static_cast<double>(m_LastFrameTiming.triangles) / m_LastFrameTiming.cpuTime;
    }
    else
    {
        m_LastFrameTiming.renderingEfficiency = 0.0;
    }
    
    // 4. Draw Call Efficiency (Objects per draw call ratio)
    if (m_LastFrameTiming.drawCalls > 0)
    {
        // For GPU-driven rendering, use visible objects; for CPU-driven, use total instances
        uint32_t effectiveObjects = (m_CurrentMode == RenderingMode::GPU_DRIVEN) ? 
                                   m_LastFrameTiming.visibleObjects : m_LastFrameTiming.instances;
        m_LastFrameTiming.drawCallEfficiency = static_cast<double>(effectiveObjects) / static_cast<double>(m_LastFrameTiming.drawCalls);
    }
    else
    {
        m_LastFrameTiming.drawCallEfficiency = 0.0;
    }
    
    // 5. Estimate GPU Utilization based on rendering mode and workload
    // This is a rough estimate since true GPU utilization requires vendor-specific APIs
    if (m_LastFrameTiming.cpuTime > 0.0)
    {
        // Different base utilization for different rendering modes
        double baseUtilization = 0.0;
        
        if (m_CurrentMode == RenderingMode::GPU_DRIVEN)
        {
            // GPU-driven: Higher base utilization due to compute shaders + parallel processing
            double computeWorkload = static_cast<double>(m_LastFrameTiming.computeDispatches) * 8.0; // Compute shader bonus
            double triangleEfficiency = static_cast<double>(m_LastFrameTiming.triangles) / 100000.0; // Per 100k triangles
            double drawCallEfficiency = m_LastFrameTiming.drawCallEfficiency / 10.0; // Efficiency bonus
            
            baseUtilization = 55.0 + computeWorkload + triangleEfficiency + drawCallEfficiency;
            baseUtilization = (baseUtilization > 85.0) ? 85.0 : (baseUtilization < 50.0) ? 50.0 : baseUtilization;
        }
        else
        {
            // CPU-driven: Lower utilization due to individual draw calls + CPU bottlenecks
            double triangleWorkload = static_cast<double>(m_LastFrameTiming.triangles) / 200000.0; // Per 200k triangles
            double drawCallPenalty = static_cast<double>(m_LastFrameTiming.drawCalls) / 100.0; // Many draw calls reduce efficiency
            
            baseUtilization = 35.0 + triangleWorkload - drawCallPenalty;
            baseUtilization = (baseUtilization > 65.0) ? 65.0 : (baseUtilization < 25.0) ? 25.0 : baseUtilization;
        }
        
        m_LastFrameTiming.gpuUtilization = baseUtilization;
    }
    else
    {
        m_LastFrameTiming.gpuUtilization = 0.0;
    }
    
    // 6. Memory Throughput calculation (more accurate than bandwidth usage)
    if (m_LastFrameTiming.cpuTime > 0.0)
    {
        // Calculate effective memory throughput based on actual data transferred
        double vertexBytes = m_LastFrameTiming.triangles * 72.0; // 24 bytes per vertex * 3 vertices per triangle
        double matrixBytes = m_LastFrameTiming.instances * 64.0; // 64 bytes per world matrix
        double totalBytes = vertexBytes + matrixBytes;
        double totalGB = totalBytes / (1024.0 * 1024.0 * 1024.0); // Convert to GB
        double timeInSeconds = m_LastFrameTiming.cpuTime / 1000.0; // Convert ms to seconds
        
        m_LastFrameTiming.memoryThroughput = totalGB / timeInSeconds; // GB/s
    }
    else
    {
        m_LastFrameTiming.memoryThroughput = 0.0;
    }
}

double PerformanceProfiler::GetGPUMemoryUsage()
{
    // This would require vendor-specific APIs (NVAPI, AMD API, etc.)
    // For now, return the estimated value
    return m_LastFrameTiming.gpuMemoryUsage;
}

double PerformanceProfiler::GetCPUMemoryUsage()
{
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc)))
    {
        return static_cast<double>(pmc.WorkingSetSize) / (1024.0 * 1024.0);
    }
    return 0.0;
}

void PerformanceProfiler::StartBenchmark(const BenchmarkConfig& config)
{
    if (m_BenchmarkRunning)
    {
        LOG_WARNING("Benchmark already running, stopping current benchmark");
        StopBenchmark();
    }

    m_BenchmarkConfig = config;
    m_BenchmarkCurrentFrame = 0;
    m_BenchmarkFrameData.clear();
    m_BenchmarkRunning = true;
    m_CurrentMode = config.mode;

    LOG("Starting benchmark: " + config.sceneName + " with " + std::to_string(config.objectCount) + " objects");
    
    std::string modeStr;
    if (config.mode == PerformanceProfiler::RenderingMode::CPU_DRIVEN)
        modeStr = "CPU_DRIVEN";
    else if (config.mode == PerformanceProfiler::RenderingMode::GPU_DRIVEN)
        modeStr = "GPU_DRIVEN";
    else
        modeStr = "HYBRID";
    LOG("Rendering mode: " + modeStr);
}

void PerformanceProfiler::StopBenchmark()
{
    if (!m_BenchmarkRunning) return;

    m_BenchmarkRunning = false;
    
    // Calculate final statistics
    CalculateBenchmarkStatistics(m_LastBenchmarkResults);
    
    LOG("Benchmark completed. Average FPS: " + std::to_string(m_LastBenchmarkResults.averageFPS));
    LOG("Average frame time: " + std::to_string(m_LastBenchmarkResults.averageFrameTime) + "ms");
    LOG("Average GPU time: " + std::to_string(m_LastBenchmarkResults.averageGPUTime) + "ms");
    LOG("Average CPU time: " + std::to_string(m_LastBenchmarkResults.averageCPUTime) + "ms");
}

void PerformanceProfiler::ProcessBenchmarkFrame()
{
    if (m_BenchmarkCurrentFrame >= m_BenchmarkConfig.benchmarkDuration)
    {
        StopBenchmark();
        return;
    }

    // Store frame data for benchmark
    FrameData frameData;
    frameData.timing = m_LastFrameTiming;
    frameData.timestamp = m_FrameStartTime;
    frameData.mode = m_CurrentMode;
    m_BenchmarkFrameData.push_back(frameData);

    m_BenchmarkCurrentFrame++;

    // Keep only the last MAX_BENCHMARK_FRAMES
    if (m_BenchmarkFrameData.size() > MAX_BENCHMARK_FRAMES)
    {
        m_BenchmarkFrameData.erase(m_BenchmarkFrameData.begin());
    }
}

void PerformanceProfiler::CalculateBenchmarkStatistics(BenchmarkResults& results)
{
    results.config = m_BenchmarkConfig;
    
    if (m_BenchmarkFrameData.empty())
    {
        LOG_ERROR("No benchmark data available");
        return;
    }

    // Extract raw data
    std::vector<double> frameTimes, gpuTimes, cpuTimes, fpsValues;
    
    for (const auto& frame : m_BenchmarkFrameData)
    {
        frameTimes.push_back(frame.timing.cpuTime);
        gpuTimes.push_back(frame.timing.gpuTime);
        cpuTimes.push_back(frame.timing.cpuTime);
        
        if (frame.timing.cpuTime > 0)
        {
            fpsValues.push_back(1000.0 / frame.timing.cpuTime);
        }
    }

    // Calculate averages
    results.averageFrameTime = std::accumulate(frameTimes.begin(), frameTimes.end(), 0.0) / static_cast<double>(frameTimes.size());
    results.averageGPUTime = std::accumulate(gpuTimes.begin(), gpuTimes.end(), 0.0) / static_cast<double>(gpuTimes.size());
    results.averageCPUTime = std::accumulate(cpuTimes.begin(), cpuTimes.end(), 0.0) / static_cast<double>(cpuTimes.size());
    results.averageFPS = std::accumulate(fpsValues.begin(), fpsValues.end(), 0.0) / static_cast<double>(fpsValues.size());

    // Calculate min/max FPS
    if (!fpsValues.empty())
    {
        results.minFPS = *std::min_element(fpsValues.begin(), fpsValues.end());
        results.maxFPS = *std::max_element(fpsValues.begin(), fpsValues.end());
        results.fpsVariance = CalculateVariance(fpsValues, results.averageFPS);
    }

    // Calculate other averages
    double totalDrawCalls = 0, totalTriangles = 0, totalInstances = 0;
    double totalIndirectDrawCalls = 0, totalComputeDispatches = 0;
    double totalGPUMemory = 0, totalCPUMemory = 0, totalBandwidth = 0;

    for (const auto& frame : m_BenchmarkFrameData)
    {
        totalDrawCalls += static_cast<double>(frame.timing.drawCalls);
        totalTriangles += static_cast<double>(frame.timing.triangles);
        totalInstances += static_cast<double>(frame.timing.instances);
        totalIndirectDrawCalls += static_cast<double>(frame.timing.indirectDrawCalls);
        totalComputeDispatches += static_cast<double>(frame.timing.computeDispatches);
        totalGPUMemory += frame.timing.gpuMemoryUsage;
        totalCPUMemory += frame.timing.cpuMemoryUsage;
        totalBandwidth += frame.timing.bandwidthUsage;
    }

    size_t frameCount = m_BenchmarkFrameData.size();
    results.averageDrawCalls = totalDrawCalls / static_cast<double>(frameCount);
    results.averageTriangles = totalTriangles / static_cast<double>(frameCount);
    results.averageInstances = totalInstances / static_cast<double>(frameCount);
    results.averageIndirectDrawCalls = totalIndirectDrawCalls / static_cast<double>(frameCount);
    results.averageComputeDispatches = totalComputeDispatches / static_cast<double>(frameCount);
    results.averageGPUMemoryUsage = totalGPUMemory / static_cast<double>(frameCount);
    results.averageCPUMemoryUsage = totalCPUMemory / static_cast<double>(frameCount);
    results.averageBandwidthUsage = totalBandwidth / static_cast<double>(frameCount);

    // Store raw data
    results.frameTimes = frameTimes;
    results.gpuTimes = gpuTimes;
    results.cpuTimes = cpuTimes;
}

double PerformanceProfiler::CalculateVariance(const std::vector<double>& values, double mean)
{
    if (values.empty()) return 0.0;
    
    double sum = 0.0;
    for (double value : values)
    {
        double diff = value - mean;
        sum += diff * diff;
    }
    return sum / static_cast<double>(values.size());
}

double PerformanceProfiler::GetBenchmarkProgress() const
{
    if (!m_BenchmarkRunning || m_BenchmarkConfig.benchmarkDuration == 0)
        return 0.0;
    
    return static_cast<double>(m_BenchmarkCurrentFrame) / static_cast<double>(m_BenchmarkConfig.benchmarkDuration);
}

double PerformanceProfiler::GetAverageFPS() const
{
    if (m_FrameHistory.empty()) return 0.0;

    double totalFrameTime = 0.0;
    for (const auto& frame : m_FrameHistory)
    {
        totalFrameTime += frame.timing.cpuTime;
    }

    double averageFrameTime = totalFrameTime / static_cast<double>(m_FrameHistory.size());
    return 1000.0 / averageFrameTime; // Convert to FPS
}

double PerformanceProfiler::GetAverageFrameTime() const
{
    if (m_FrameHistory.empty()) return 0.0;

    double totalFrameTime = 0.0;
    for (const auto& frame : m_FrameHistory)
    {
        totalFrameTime += frame.timing.cpuTime;
    }

    return totalFrameTime / static_cast<double>(m_FrameHistory.size());
}

double PerformanceProfiler::GetCurrentFPS() const
{
    if (m_LastFrameTime <= 0.0) return 0.0;
    double fps = CommonTimer::GetInstance().CalculateFPS(m_LastFrameTime);
    
    return fps;
}

PerformanceProfiler::BenchmarkResults PerformanceProfiler::CompareBenchmarks(const BenchmarkResults& cpuResults, const BenchmarkResults& gpuResults)
{
    BenchmarkResults comparison;
    comparison.config = cpuResults.config; // Use CPU config as base
    
    // Calculate performance improvements
    comparison.averageFPS = gpuResults.averageFPS;
    comparison.averageFrameTime = gpuResults.averageFrameTime;
    comparison.averageGPUTime = gpuResults.averageGPUTime;
    comparison.averageCPUTime = gpuResults.averageCPUTime;
    
    // Calculate percentage improvements
    double fpsImprovement = ((gpuResults.averageFPS - cpuResults.averageFPS) / cpuResults.averageFPS) * 100.0;
    double frameTimeImprovement = ((cpuResults.averageFrameTime - gpuResults.averageFrameTime) / cpuResults.averageFrameTime) * 100.0;
    
    LOG("Performance Comparison Results:");
    LOG("FPS Improvement: " + std::to_string(fpsImprovement) + "%");
    LOG("Frame Time Improvement: " + std::to_string(frameTimeImprovement) + "%");
    
    return comparison;
}

double PerformanceProfiler::CalculatePerformanceImprovement(const BenchmarkResults& cpuResults, const BenchmarkResults& gpuResults)
{
    if (cpuResults.averageFPS <= 0) return 0.0;
    
    return ((gpuResults.averageFPS - cpuResults.averageFPS) / cpuResults.averageFPS) * 100.0;
}

bool PerformanceProfiler::ExportBenchmarkResults(const std::string& filename, const BenchmarkResults& results)
{
    std::ofstream file(filename);
    if (!file.is_open())
    {
        LOG_ERROR("Failed to open file for writing: " + filename);
        return false;
    }

    file << "Benchmark Results Report\n";
    file << "========================\n\n";
    
    file << "Configuration:\n";
    file << "  Scene: " << results.config.sceneName << "\n";
    file << "  Object Count: " << results.config.objectCount << "\n";
    std::string renderModeStr;
    if (results.config.mode == PerformanceProfiler::RenderingMode::CPU_DRIVEN)
        renderModeStr = "CPU_DRIVEN";
    else if (results.config.mode == PerformanceProfiler::RenderingMode::GPU_DRIVEN)
        renderModeStr = "GPU_DRIVEN";
    else
        renderModeStr = "HYBRID";
    file << "  Rendering Mode: " << renderModeStr << "\n";
    file << "  Frustum Culling: " << (results.config.enableFrustumCulling ? "Enabled" : "Disabled") << "\n";
    file << "  LOD: " << (results.config.enableLOD ? "Enabled" : "Disabled") << "\n";
    file << "  Occlusion Culling: " << (results.config.enableOcclusionCulling ? "Enabled" : "Disabled") << "\n\n";
    
    file << "Performance Metrics:\n";
    file << "  Average FPS: " << std::fixed << std::setprecision(2) << results.averageFPS << "\n";
    file << "  Average Frame Time: " << results.averageFrameTime << " ms\n";
    file << "  Average GPU Time: " << results.averageGPUTime << " ms\n";
    file << "  Average CPU Time: " << results.averageCPUTime << " ms\n";
    file << "  Min FPS: " << results.minFPS << "\n";
    file << "  Max FPS: " << results.maxFPS << "\n";
    file << "  FPS Variance: " << results.fpsVariance << "\n\n";
    
    file << "Rendering Statistics:\n";
    file << "  Average Draw Calls: " << results.averageDrawCalls << "\n";
    file << "  Average Triangles: " << results.averageTriangles << "\n";
    file << "  Average Instances: " << results.averageInstances << "\n";
    file << "  Average Indirect Draw Calls: " << results.averageIndirectDrawCalls << "\n";
    file << "  Average Compute Dispatches: " << results.averageComputeDispatches << "\n\n";
    
    file << "Memory Usage:\n";
    file << "  Average GPU Memory: " << results.averageGPUMemoryUsage << " MB\n";
    file << "  Average CPU Memory: " << results.averageCPUMemoryUsage << " MB\n";
    file << "  Average Bandwidth: " << results.averageBandwidthUsage << " GB/s\n\n";
    
    file << "Raw Frame Data:\n";
    file << "Frame,CPU_Time,GPU_Time\n";
    for (size_t i = 0; i < results.frameTimes.size(); ++i)
    {
        file << i << "," << results.frameTimes[i] << "," << results.gpuTimes[i] << "\n";
    }

    file.close();
    LOG("Benchmark results exported to: " + filename);
    return true;
}

bool PerformanceProfiler::ExportComparisonReport(const std::string& filename, const BenchmarkResults& cpuResults, const BenchmarkResults& gpuResults)
{
    std::ofstream file(filename);
    if (!file.is_open())
    {
        LOG_ERROR("Failed to open file for writing: " + filename);
        return false;
    }

    double fpsImprovement = CalculatePerformanceImprovement(cpuResults, gpuResults);
    double frameTimeImprovement = ((cpuResults.averageFrameTime - gpuResults.averageFrameTime) / cpuResults.averageFrameTime) * 100.0;

    file << "GPU-Driven Rendering Performance Comparison Report\n";
    file << "==================================================\n\n";
    
    file << "Test Configuration:\n";
    file << "  Scene: " << cpuResults.config.sceneName << "\n";
    file << "  Object Count: " << cpuResults.config.objectCount << "\n";
    file << "  Benchmark Duration: " << cpuResults.config.benchmarkDuration << " frames\n\n";
    
    file << "Performance Comparison:\n";
    file << "  Metric                    CPU-Driven    GPU-Driven    Improvement\n";
    file << "  -----------------------------------------------------------------\n";
    file << "  Average FPS               " << std::fixed << std::setprecision(2) << std::setw(10) << cpuResults.averageFPS 
         << "    " << std::setw(10) << gpuResults.averageFPS << "    " << std::setw(10) << fpsImprovement << "%\n";
    file << "  Average Frame Time (ms)   " << std::setw(10) << cpuResults.averageFrameTime 
         << "    " << std::setw(10) << gpuResults.averageFrameTime << "    " << std::setw(10) << frameTimeImprovement << "%\n";
    file << "  Average GPU Time (ms)     " << std::setw(10) << cpuResults.averageGPUTime 
         << "    " << std::setw(10) << gpuResults.averageGPUTime << "    " << std::setw(10) 
         << ((cpuResults.averageGPUTime - gpuResults.averageGPUTime) / cpuResults.averageGPUTime) * 100.0 << "%\n";
    file << "  Average CPU Time (ms)     " << std::setw(10) << cpuResults.averageCPUTime 
         << "    " << std::setw(10) << gpuResults.averageCPUTime << "    " << std::setw(10) 
         << ((cpuResults.averageCPUTime - gpuResults.averageCPUTime) / cpuResults.averageCPUTime) * 100.0 << "%\n\n";
    
    file << "Rendering Efficiency:\n";
    file << "  Metric                    CPU-Driven    GPU-Driven    Change\n";
    file << "  ------------------------------------------------------------\n";
    file << "  Draw Calls                " << std::setw(10) << cpuResults.averageDrawCalls 
         << "    " << std::setw(10) << gpuResults.averageDrawCalls << "    " << std::setw(10) 
         << ((double)gpuResults.averageDrawCalls / cpuResults.averageDrawCalls) * 100.0 << "%\n";
    file << "  Indirect Draw Calls       " << std::setw(10) << cpuResults.averageIndirectDrawCalls 
         << "    " << std::setw(10) << gpuResults.averageIndirectDrawCalls << "    " << std::setw(10) 
         << ((double)gpuResults.averageIndirectDrawCalls / (cpuResults.averageIndirectDrawCalls + 1)) * 100.0 << "%\n";
    file << "  Compute Dispatches        " << std::setw(10) << cpuResults.averageComputeDispatches 
         << "    " << std::setw(10) << gpuResults.averageComputeDispatches << "    " << std::setw(10) 
         << ((double)gpuResults.averageComputeDispatches / (cpuResults.averageComputeDispatches + 1)) * 100.0 << "%\n\n";
    
    file << "Memory Usage:\n";
    file << "  Metric                    CPU-Driven    GPU-Driven    Change\n";
    file << "  ------------------------------------------------------------\n";
    file << "  GPU Memory (MB)           " << std::setw(10) << cpuResults.averageGPUMemoryUsage 
         << "    " << std::setw(10) << gpuResults.averageGPUMemoryUsage << "    " << std::setw(10) 
         << ((gpuResults.averageGPUMemoryUsage - cpuResults.averageGPUMemoryUsage) / cpuResults.averageGPUMemoryUsage) * 100.0 << "%\n";
    file << "  CPU Memory (MB)           " << std::setw(10) << cpuResults.averageCPUMemoryUsage 
         << "    " << std::setw(10) << gpuResults.averageCPUMemoryUsage << "    " << std::setw(10) 
         << ((gpuResults.averageCPUMemoryUsage - cpuResults.averageCPUMemoryUsage) / cpuResults.averageCPUMemoryUsage) * 100.0 << "%\n";
    file << "  Bandwidth (GB/s)          " << std::setw(10) << cpuResults.averageBandwidthUsage 
         << "    " << std::setw(10) << gpuResults.averageBandwidthUsage << "    " << std::setw(10) 
         << ((gpuResults.averageBandwidthUsage - cpuResults.averageBandwidthUsage) / (cpuResults.averageBandwidthUsage + 0.1)) * 100.0 << "%\n\n";
    
    file << "Conclusion:\n";
    if (fpsImprovement > 0)
    {
        file << "GPU-driven rendering shows a " << fpsImprovement << "% improvement in performance\n";
        file << "compared to CPU-driven rendering for this scene configuration.\n";
    }
    else
    {
        file << "CPU-driven rendering performs " << -fpsImprovement << "% better than GPU-driven rendering\n";
        file << "for this scene configuration. This may indicate overhead in the GPU-driven approach.\n";
    }

    file.close();
    LOG("Comparison report exported to: " + filename);
    return true;
} 