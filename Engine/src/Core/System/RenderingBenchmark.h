#ifndef RENDERING_BENCHMARK_H
#define RENDERING_BENCHMARK_H

#include <d3d11.h>
#include <directxmath.h>
#include <vector>
#include <string>
#include <memory>
#include <chrono>
#include <fstream>
#include "PerformanceProfiler.h"
#include "../../Graphics/Shaders/ComputeShader.h"
#include "../../Graphics/Rendering/GPUDrivenRenderer.h"

using namespace DirectX;

// Forward declarations
class Application;

// Benchmark configuration structure
struct BenchmarkConfig
{
    enum class RenderingApproach
    {
        CPU_DRIVEN,
        GPU_DRIVEN,
        HYBRID
    };
    
    RenderingApproach approach = RenderingApproach::CPU_DRIVEN;
    uint32_t objectCount = 1000;
    uint32_t benchmarkDuration = 300; // Duration in frames
    bool enableFrustumCulling = true;
    bool enableLOD = true;
    bool enableOcclusionCulling = false;
    std::string sceneName = "Default Scene";
    std::string outputDirectory = "./benchmark_results/";
};

// Benchmark result structure
struct BenchmarkResult
{
    std::string approach;
    int objectCount = 0;
    int visibleObjects = 0;
    
    // Performance metrics
    double averageFrameTime = 0.0;
    double averageFPS = 0.0;
    double averageGPUTime = 0.0;
    double averageCPUTime = 0.0;
    int averageDrawCalls = 0;
    int averageTriangles = 0;
    int averageInstances = 0;
    int averageIndirectDrawCalls = 0;
    int averageComputeDispatches = 0;
    double averageGPUMemoryUsage = 0.0;
    double averageCPUMemoryUsage = 0.0;
    double averageBandwidthUsage = 0.0;
    
    // Efficiency metrics
    double averageGPUUtilization = 0.0;
    double averageCullingEfficiency = 0.0;
    double averageRenderingEfficiency = 0.0;
    double averageDrawCallEfficiency = 0.0;
    double averageModelDrawCallEfficiency = 0.0;
    double averageTotalSystemEfficiency = 0.0;
    double averageMemoryThroughput = 0.0;
    double averageFrustumCullingSpeedup = 0.0;
    
    // Raw data vectors
    std::vector<double> frameTimes;
    std::vector<double> gpuTimes;
    std::vector<double> cpuTimes;
    std::vector<int> drawCalls;
    std::vector<int> triangles;
    std::vector<int> instances;
    std::vector<int> indirectDrawCalls;
    std::vector<int> computeDispatches;
    std::vector<double> gpuMemoryUsage;
    std::vector<double> cpuMemoryUsage;
    std::vector<double> bandwidthUsage;
    
    // Efficiency metric raw data vectors
    std::vector<double> gpuUtilization;
    std::vector<double> cullingEfficiency;
    std::vector<double> renderingEfficiency;
    std::vector<double> drawCallEfficiency;
    std::vector<double> modelDrawCallEfficiency;
    std::vector<double> totalSystemEfficiency;
    std::vector<double> memoryThroughput;
    std::vector<double> frustumCullingSpeedup;
};

// LOD level structure
struct LODLevel
{
    float distance;
    int indexCount;
    int startIndexLocation;
    int baseVertexLocation;
    int startInstanceLocation;
};

class RenderingBenchmark
{
public:
    RenderingBenchmark();
    ~RenderingBenchmark();

    // Initialization
    bool Initialize(ID3D11Device* device, ID3D11DeviceContext* context, HWND hwnd, Application* application);

    // Benchmark execution
    BenchmarkResult RunBenchmark(const BenchmarkConfig& config);
    std::vector<BenchmarkResult> RunBenchmarkSuite();
    void RunBenchmarkFrame(const BenchmarkConfig& config, BenchmarkResult& result, int currentFrame);
    
    // Frame-by-frame benchmark execution for UI progress updates
    bool StartFrameByFrameBenchmark(const BenchmarkConfig& config);
    bool RunNextBenchmarkFrame(); // Returns true if benchmark is complete
    BenchmarkResult GetCurrentBenchmarkResult();
    void StopFrameByFrameBenchmark();

    // Progress tracking
    double GetProgress() const { return m_Progress; }
    std::string GetStatus() const { return m_Status; }

    // Result management
    bool SaveResults(const std::vector<BenchmarkResult>& results, const std::string& filename);
    bool SaveComparisonReport(const std::vector<BenchmarkResult>& results, const std::string& filename);

private:
    // Benchmark implementations
    BenchmarkResult RunCPUDrivenBenchmark(const BenchmarkConfig& config);
    BenchmarkResult RunGPUDrivenBenchmark(const BenchmarkConfig& config);
    BenchmarkResult RunHybridBenchmark(const BenchmarkConfig& config);

    // Scene generation
    void GenerateTestScene(int objectCount, std::vector<ObjectData>& objects);
    void GenerateGridScene(int objectCount, std::vector<ObjectData>& objects);
    void GenerateRandomScene(int objectCount, std::vector<ObjectData>& objects);
    void GenerateStressTestScene(int objectCount, std::vector<ObjectData>& objects);
    void GenerateRealScene(int objectCount, std::vector<ObjectData>& objects);

    // Camera and timing
    void UpdateCamera(float deltaTime);
    void BeginFrame();
    void EndFrame();
    void RecordMetrics(BenchmarkResult& result);
    void RecordMetricsWithSimulationTiming(BenchmarkResult& result, double simulationTimeMs);

    // Dummy resources for benchmarking
    bool CreateDummyBuffers();
    void ReleaseDummyBuffers();

    // File I/O
    void WriteCSVHeader(std::ofstream& file);
    void WriteResultToCSV(const BenchmarkResult& result, std::ofstream& file);
    void GeneratePerformanceCharts(const std::vector<BenchmarkResult>& results);

private:
    // DirectX resources
    ID3D11Device* m_Device;
    ID3D11DeviceContext* m_Context;
    HWND m_Hwnd;
    
    // Application reference for real rendering
    Application* m_Application;

    // Compute shaders
    std::unique_ptr<ComputeShader> m_FrustumCullingShader;
    std::unique_ptr<ComputeShader> m_LODSelectionShader;
    std::unique_ptr<ComputeShader> m_CommandGenerationShader;

    // GPU-driven renderer
    std::unique_ptr<GPUDrivenRenderer> m_GPUDrivenRenderer;

    // Progress tracking
    double m_Progress;
    std::string m_Status;
    
    // Frame-by-frame benchmark state
    bool m_FrameByFrameBenchmarkRunning;
    BenchmarkConfig m_CurrentFrameByFrameConfig;
    BenchmarkResult m_CurrentFrameByFrameResult;
    int m_CurrentFrameIndex;

    // Test data
    std::vector<ObjectData> m_TestObjects;
    std::vector<LODLevel> m_LODLevels;

    // Camera state
    XMFLOAT3 m_CameraPosition;
    XMFLOAT3 m_CameraTarget;
    float m_CameraRotation;

    // Timing
    std::chrono::high_resolution_clock::time_point m_FrameStartTime;
    std::chrono::high_resolution_clock::time_point m_FrameEndTime;

    // Dummy resources for benchmarking
    ID3D11Buffer* m_DummyVertexBuffer;
    ID3D11Buffer* m_DummyIndexBuffer;
    ID3D11VertexShader* m_DummyVertexShader;
    ID3D11PixelShader* m_DummyPixelShader;
    ID3D11InputLayout* m_DummyInputLayout;
};

#endif // RENDERING_BENCHMARK_H 