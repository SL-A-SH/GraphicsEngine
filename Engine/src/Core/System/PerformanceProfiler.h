#ifndef _PERFORMANCE_PROFILER_H_
#define _PERFORMANCE_PROFILER_H_

#include <d3d11.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <stack>

class PerformanceProfiler
{
public:
    struct TimingData
    {
        double cpuTime;    // CPU time in milliseconds
        double gpuTime;    // GPU time in milliseconds
        uint32_t drawCalls;
        uint32_t triangles;
        uint32_t vertices;
        std::unordered_map<std::string, TimingData> sections;
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

    // Enable/Disable profiling
    void SetEnabled(bool enabled) { m_Enabled = enabled; }
    bool IsEnabled() const { return m_Enabled; }

    // Draw call tracking
    void IncrementDrawCalls() { m_LastFrameTiming.drawCalls++; }
    void AddTriangles(uint32_t count) { m_LastFrameTiming.triangles += count; }
    void AddVertices(uint32_t count) { m_LastFrameTiming.vertices += count; }

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

    static const size_t MAX_FRAME_HISTORY = 300; // Store 5 seconds at 60 FPS
};

#endif