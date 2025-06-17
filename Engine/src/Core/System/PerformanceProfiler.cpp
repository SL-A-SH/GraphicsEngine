#include "PerformanceProfiler.h"
#include "Logger.h"

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
{
    m_LastFrameTiming = { 0.0, 0.0, 0, 0, 0 };
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
    LOG("Performance profiler initialized");
}

void PerformanceProfiler::Shutdown()
{
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

    auto now = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    m_FrameStartTime = static_cast<double>(duration.count());
    m_LastFrameTiming = { 0.0, 0.0, 0, 0, 0 };

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

    // Calculate CPU time
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    double frameEndTime = static_cast<double>(duration.count());
    m_LastFrameTiming.cpuTime = frameEndTime - m_FrameStartTime;

    // Store frame data
    FrameData frameData;
    frameData.timing = m_LastFrameTiming;
    frameData.timestamp = frameEndTime;
    m_FrameHistory.push_back(frameData);

    // Keep only the last MAX_FRAME_HISTORY frames
    if (m_FrameHistory.size() > MAX_FRAME_HISTORY)
    {
        m_FrameHistory.erase(m_FrameHistory.begin());
    }
}

void PerformanceProfiler::BeginSection(const std::string& name)
{
    if (!m_Enabled) return;

    SectionData section;
    section.name = name;
    section.timing = { 0.0, 0.0, 0, 0, 0 };
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

double PerformanceProfiler::GetAverageFPS() const
{
    if (m_FrameHistory.empty()) return 0.0;

    double totalFrameTime = 0.0;
    for (const auto& frame : m_FrameHistory)
    {
        totalFrameTime += frame.timing.cpuTime;
    }

    double averageFrameTime = totalFrameTime / m_FrameHistory.size();
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

    return totalFrameTime / m_FrameHistory.size();
} 