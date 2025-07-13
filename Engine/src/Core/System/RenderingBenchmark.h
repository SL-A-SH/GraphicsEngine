#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <chrono>
#include <memory>
#include "../../Graphics/D3D11/D3D11Device.h"
#include "../../Graphics/Shaders/ComputeShader.h"
#include "../../Graphics/Rendering/GPUDrivenRenderer.h"
#include "../../Graphics/Rendering/IndirectDrawBuffer.h"

// Benchmark result structure
struct BenchmarkResult
{
    BenchmarkResult()
        : approach(""), objectCount(0), visibleObjects(0), averageFPS(0.0), averageFrameTime(0.0),
          averageGPUTime(0.0), averageCPUTime(0.0), averageDrawCalls(0), averageTriangles(0),
          averageInstances(0), averageIndirectDrawCalls(0), averageComputeDispatches(0),
          averageGPUMemoryUsage(0.0), averageCPUMemoryUsage(0.0), averageBandwidthUsage(0.0)
    {
        frameTimes.clear();
        gpuTimes.clear();
        cpuTimes.clear();
        drawCalls.clear();
        triangles.clear();
        instances.clear();
        indirectDrawCalls.clear();
        computeDispatches.clear();
        gpuMemoryUsage.clear();
        cpuMemoryUsage.clear();
        bandwidthUsage.clear();
    }
    std::string approach;
    int objectCount;
    int visibleObjects;
    double averageFPS;
    double averageFrameTime;
    double averageGPUTime;
    double averageCPUTime;
    int averageDrawCalls;
    int averageTriangles;
    int averageInstances;
    int averageIndirectDrawCalls;
    int averageComputeDispatches;
    double averageGPUMemoryUsage;
    double averageCPUMemoryUsage;
    double averageBandwidthUsage;
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
};

// Benchmark configuration
struct BenchmarkConfig
{
    enum class RenderingApproach
    {
        CPU_DRIVEN,
        GPU_DRIVEN,
        HYBRID
    };

    RenderingApproach approach;
    int objectCount;
    int benchmarkDuration;
    bool enableFrustumCulling;
    bool enableLOD;
    bool enableOcclusionCulling;
    std::string sceneName;
    std::string outputDirectory;
};

class RenderingBenchmark
{
public:
    RenderingBenchmark();
    ~RenderingBenchmark();

    // Initialize the benchmark system
    bool Initialize(ID3D11Device* device, ID3D11DeviceContext* context, HWND hwnd);

    // Run a single benchmark test
    BenchmarkResult RunBenchmark(const BenchmarkConfig& config);

    // Run comprehensive benchmark suite
    std::vector<BenchmarkResult> RunBenchmarkSuite();

    // Save benchmark results to files
    bool SaveResults(const std::vector<BenchmarkResult>& results, const std::string& filename);
    bool SaveComparisonReport(const std::vector<BenchmarkResult>& results, const std::string& filename);

    // Generate test scene with specified number of objects
    void GenerateTestScene(int objectCount, std::vector<ObjectData>& objects);

    // Update camera for testing
    void UpdateCamera(float deltaTime);

    // Get current benchmark progress
    double GetProgress() const { return m_Progress; }

    // Get current benchmark status
    std::string GetStatus() const { return m_Status; }

    // Add this method to support frame-by-frame benchmarking
    void RunBenchmarkFrame(const BenchmarkConfig& config, BenchmarkResult& result, int currentFrame);

private:
    // Benchmark execution methods
    BenchmarkResult RunCPUDrivenBenchmark(const BenchmarkConfig& config);
    BenchmarkResult RunGPUDrivenBenchmark(const BenchmarkConfig& config);
    BenchmarkResult RunHybridBenchmark(const BenchmarkConfig& config);

    // Performance measurement
    void BeginFrame();
    void EndFrame();
    void RecordMetrics(BenchmarkResult& result);

    // File output helpers
    void WriteCSVHeader(std::ofstream& file);
    void WriteResultToCSV(const BenchmarkResult& result, std::ofstream& file);
    void GeneratePerformanceCharts(const std::vector<BenchmarkResult>& results);

    // Test scene generation
    void GenerateGridScene(int objectCount, std::vector<ObjectData>& objects);
    void GenerateRandomScene(int objectCount, std::vector<ObjectData>& objects);
    void GenerateStressTestScene(int objectCount, std::vector<ObjectData>& objects);
    
    // Dummy buffer creation for benchmarking
    bool CreateDummyBuffers();
    void ReleaseDummyBuffers();

    // Member variables
    ID3D11Device* m_Device;
    ID3D11DeviceContext* m_Context;
    HWND m_Hwnd;

    // Compute shaders
    std::unique_ptr<ComputeShader> m_FrustumCullingShader;
    std::unique_ptr<ComputeShader> m_LODSelectionShader;
    std::unique_ptr<ComputeShader> m_CommandGenerationShader;

    // GPU-driven renderer
    std::unique_ptr<GPUDrivenRenderer> m_GPUDrivenRenderer;

    // Benchmark state
    double m_Progress;
    std::string m_Status;
    std::chrono::high_resolution_clock::time_point m_FrameStartTime;
    std::chrono::high_resolution_clock::time_point m_FrameEndTime;

    // Camera for testing
    XMFLOAT3 m_CameraPosition;
    XMFLOAT3 m_CameraTarget;
    float m_CameraRotation;

    // Test data
    std::vector<ObjectData> m_TestObjects;
    std::vector<LODLevel> m_LODLevels;
    
    // Dummy buffers for benchmarking
    ID3D11Buffer* m_DummyVertexBuffer;
    ID3D11Buffer* m_DummyIndexBuffer;
    ID3D11VertexShader* m_DummyVertexShader;
    ID3D11PixelShader* m_DummyPixelShader;
    ID3D11InputLayout* m_DummyInputLayout;
}; 