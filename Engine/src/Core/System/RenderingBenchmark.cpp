#include "RenderingBenchmark.h"
#include "Logger.h"
#include "PerformanceProfiler.h"
#include <algorithm>
#include <numeric>
#include <iomanip>
#include <sstream>
#include <random>
#include <map>
#include <cmath>
#include <cstdint>
#include <d3dcompiler.h>
#include <fstream> // Added for file I/O

RenderingBenchmark::RenderingBenchmark()
    : m_Device(nullptr)
    , m_Context(nullptr)
    , m_Hwnd(nullptr)
    , m_Progress(0.0)
    , m_Status("Not initialized")
    , m_CameraPosition(0.0f, 0.0f, -300.0f)
    , m_CameraTarget(0.0f, 0.0f, 0.0f)
    , m_CameraRotation(0.0f)
    , m_DummyVertexBuffer(nullptr)
    , m_DummyIndexBuffer(nullptr)
    , m_DummyVertexShader(nullptr)
    , m_DummyPixelShader(nullptr)
    , m_DummyInputLayout(nullptr)
{
}

RenderingBenchmark::~RenderingBenchmark()
{
    ReleaseDummyBuffers();
}

bool RenderingBenchmark::Initialize(ID3D11Device* device, ID3D11DeviceContext* context, HWND hwnd)
{
    m_Device = device;
    m_Context = context;
    m_Hwnd = hwnd;

    // Initialize compute shaders
    m_FrustumCullingShader = std::make_unique<ComputeShader>();
    m_LODSelectionShader = std::make_unique<ComputeShader>();
    m_CommandGenerationShader = std::make_unique<ComputeShader>();

    bool result = true;
    result &= m_FrustumCullingShader->Initialize(device, hwnd, L"../Engine/assets/shaders/FrustumCullingComputeShader.hlsl", "main");
    result &= m_LODSelectionShader->Initialize(device, hwnd, L"../Engine/assets/shaders/LODSelectionComputeShader.hlsl", "main");
    result &= m_CommandGenerationShader->Initialize(device, hwnd, L"../Engine/assets/shaders/CommandGenerationComputeShader.hlsl", "main");

    if (!result)
    {
        LOG_ERROR("Failed to initialize compute shaders for benchmark");
        return false;
    }

    // Initialize GPU-driven renderer
    m_GPUDrivenRenderer = std::make_unique<GPUDrivenRenderer>();
    result = m_GPUDrivenRenderer->Initialize(device, hwnd, 100000); // Support up to 100k objects

    if (!result)
    {
        LOG_ERROR("Failed to initialize GPU-driven renderer for benchmark");
        return false;
    }

    // Initialize LOD levels - using full model index count for all LODs
    // In a real implementation, you would have different LOD meshes with different index counts
    m_LODLevels.resize(4);
    m_LODLevels[0] = { 50.0f, 61260, 0, 0, 0 };   // High detail (close)
    m_LODLevels[1] = { 150.0f, 61260, 0, 0, 0 };  // Medium detail
    m_LODLevels[2] = { 300.0f, 61260, 0, 0, 0 };  // Low detail
    m_LODLevels[3] = { 1000.0f, 61260, 0, 0, 0 }; // Very low detail (far)

    // Create dummy buffers for benchmarking
    if (!CreateDummyBuffers())
    {
        LOG_ERROR("Failed to create dummy buffers for benchmarking");
        return false;
    }

    m_Status = "Initialized";
    LOG("Rendering benchmark system initialized successfully");
    return true;
}

BenchmarkResult RenderingBenchmark::RunBenchmark(const BenchmarkConfig& config)
{
    m_Status = "Running benchmark: " + config.sceneName;
    m_Progress = 0.0;

    BenchmarkResult result;
    result.approach = (config.approach == BenchmarkConfig::RenderingApproach::CPU_DRIVEN) ? "CPU-Driven" :
                     (config.approach == BenchmarkConfig::RenderingApproach::GPU_DRIVEN) ? "GPU-Driven" : "Hybrid";
    result.objectCount = config.objectCount;
    result.visibleObjects = 0; // Will be set by individual benchmark methods

    // Generate test scene
    GenerateTestScene(config.objectCount, m_TestObjects);
    LOG("Generated test scene with " + std::to_string(m_TestObjects.size()) + " objects");

    // Run appropriate benchmark based on approach
    switch (config.approach)
    {
    case BenchmarkConfig::RenderingApproach::CPU_DRIVEN:
        result = RunCPUDrivenBenchmark(config);
        break;
    case BenchmarkConfig::RenderingApproach::GPU_DRIVEN:
        result = RunGPUDrivenBenchmark(config);
        break;
    case BenchmarkConfig::RenderingApproach::HYBRID:
        result = RunHybridBenchmark(config);
        break;
    }

    m_Status = "Benchmark completed";
    m_Progress = 1.0;

    LOG("Benchmark completed: " + result.approach + " with " + std::to_string(result.visibleObjects) + " visible objects");
    return result;
}

BenchmarkResult RenderingBenchmark::RunCPUDrivenBenchmark(const BenchmarkConfig& config)
{
    BenchmarkResult result;
    result.approach = "CPU-Driven";
    result.objectCount = config.objectCount;

    // Reset performance profiler
    PerformanceProfiler::GetInstance().BeginFrame();

    std::vector<int> visibleObjects; // Declare outside the loop
    for (int frame = 0; frame < config.benchmarkDuration; frame++)
    {
        BeginFrame();

        // Update camera
        UpdateCamera(16.67f); // 60 FPS

        // CPU-driven frustum culling
        visibleObjects.clear(); // Clear for each frame
        XMMATRIX viewMatrix, projectionMatrix;
        // Get view and projection matrices (simplified for benchmark)
        viewMatrix = XMMatrixLookAtLH(
            XMLoadFloat3(&m_CameraPosition),
            XMLoadFloat3(&m_CameraTarget),
            XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)
        );
        projectionMatrix = XMMatrixPerspectiveFovLH(XM_PIDIV4, 16.0f / 9.0f, 0.1f, 1000.0f);

        // Perform CPU frustum culling
        for (int i = 0; i < m_TestObjects.size(); i++)
        {
            const auto& obj = m_TestObjects[i];
            // Simplified frustum culling check
            XMFLOAT3 worldMin = {
                obj.boundingBoxMin.x * obj.scale.x + obj.position.x,
                obj.boundingBoxMin.y * obj.scale.y + obj.position.y,
                obj.boundingBoxMin.z * obj.scale.z + obj.position.z
            };
            XMFLOAT3 worldMax = {
                obj.boundingBoxMax.x * obj.scale.x + obj.position.x,
                obj.boundingBoxMax.y * obj.scale.y + obj.position.y,
                obj.boundingBoxMax.z * obj.scale.z + obj.position.z
            };

            // Simple distance-based culling
            XMFLOAT3 objCenter = {
                (worldMin.x + worldMax.x) * 0.5f,
                (worldMin.y + worldMax.y) * 0.5f,
                (worldMin.z + worldMax.z) * 0.5f
            };
            XMFLOAT3 toCamera = {
                m_CameraPosition.x - objCenter.x,
                m_CameraPosition.y - objCenter.y,
                m_CameraPosition.z - objCenter.z
            };
            float distance = sqrt(toCamera.x * toCamera.x + toCamera.y * toCamera.y + toCamera.z * toCamera.z);

            if (distance < 500.0f) // Simple distance culling
            {
                visibleObjects.push_back(i);
            }
        }

        // Simulate rendering visible objects
        for (int objIndex : visibleObjects)
        {
            // Simulate draw call
            PerformanceProfiler::GetInstance().IncrementDrawCalls();
            PerformanceProfiler::GetInstance().AddTriangles(12); // Assume 12 triangles per object
            PerformanceProfiler::GetInstance().AddInstances(1);
        }

        EndFrame();
        RecordMetrics(result);

        // Update progress
        m_Progress = static_cast<double>(frame + 1) / static_cast<double>(config.benchmarkDuration);
        m_Status = "CPU-Driven Benchmark: Frame " + std::to_string(frame + 1) + "/" + std::to_string(config.benchmarkDuration);
    }

    // Calculate averages
    if (!result.frameTimes.empty())
    {
        result.averageFrameTime = std::accumulate(result.frameTimes.begin(), result.frameTimes.end(), 0.0) / result.frameTimes.size();
        result.averageFPS = 1000.0 / result.averageFrameTime;
    }
    if (!result.gpuTimes.empty())
    {
        result.averageGPUTime = std::accumulate(result.gpuTimes.begin(), result.gpuTimes.end(), 0.0) / result.gpuTimes.size();
    }
    if (!result.cpuTimes.empty())
    {
        result.averageCPUTime = std::accumulate(result.cpuTimes.begin(), result.cpuTimes.end(), 0.0) / result.cpuTimes.size();
    }
    if (!result.drawCalls.empty())
    {
        result.averageDrawCalls = static_cast<int>(std::accumulate(result.drawCalls.begin(), result.drawCalls.end(), 0) / result.drawCalls.size());
    }
    if (!result.triangles.empty())
    {
        result.averageTriangles = static_cast<int>(std::accumulate(result.triangles.begin(), result.triangles.end(), 0) / result.triangles.size());
    }
    if (!result.instances.empty())
    {
        result.averageInstances = static_cast<int>(std::accumulate(result.instances.begin(), result.instances.end(), 0) / result.instances.size());
    }

    result.visibleObjects = static_cast<int>(visibleObjects.size());
    return result;
}

BenchmarkResult RenderingBenchmark::RunGPUDrivenBenchmark(const BenchmarkConfig& config)
{
    BenchmarkResult result;
    result.approach = "GPU-Driven";
    result.objectCount = config.objectCount;

    // Reset performance profiler
    PerformanceProfiler::GetInstance().BeginFrame();

    for (int frame = 0; frame < config.benchmarkDuration; frame++)
    {
        BeginFrame();

        // Update camera
        UpdateCamera(16.67f);

        // GPU-driven rendering using compute shaders
        if (m_GPUDrivenRenderer)
        {
            // Update object data
            m_GPUDrivenRenderer->UpdateObjects(m_Context, m_TestObjects);

            // Update camera data
            XMMATRIX viewMatrix = XMMatrixLookAtLH(
                XMLoadFloat3(&m_CameraPosition),
                XMLoadFloat3(&m_CameraTarget),
                XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)
            );
            XMMATRIX projectionMatrix = XMMatrixPerspectiveFovLH(XM_PIDIV4, 16.0f / 9.0f, 0.1f, 1000.0f);

            m_GPUDrivenRenderer->UpdateCamera(m_Context, m_CameraPosition, viewMatrix, projectionMatrix);

            // Perform GPU-driven rendering (simplified)
            // Note: The actual render call signature may need adjustment based on your GPUDrivenRenderer implementation
            // m_GPUDrivenRenderer->Render(m_Context, m_DummyVertexBuffer, m_DummyIndexBuffer, etc.);

            // Record metrics
            PerformanceProfiler::GetInstance().IncrementDrawCalls();
            PerformanceProfiler::GetInstance().AddTriangles(static_cast<uint32_t>(m_TestObjects.size()) * 12);
            PerformanceProfiler::GetInstance().AddInstances(static_cast<uint32_t>(m_TestObjects.size()));
        }

        EndFrame();
        RecordMetrics(result);

        // Update progress
        m_Progress = static_cast<double>(frame + 1) / static_cast<double>(config.benchmarkDuration);
        m_Status = "GPU-Driven Benchmark: Frame " + std::to_string(frame + 1) + "/" + std::to_string(config.benchmarkDuration);
    }

    // Calculate averages (same as CPU-driven)
    if (!result.frameTimes.empty())
    {
        result.averageFrameTime = std::accumulate(result.frameTimes.begin(), result.frameTimes.end(), 0.0) / result.frameTimes.size();
        result.averageFPS = 1000.0 / result.averageFrameTime;
    }
    if (!result.gpuTimes.empty())
    {
        result.averageGPUTime = std::accumulate(result.gpuTimes.begin(), result.gpuTimes.end(), 0.0) / result.gpuTimes.size();
    }
    if (!result.cpuTimes.empty())
    {
        result.averageCPUTime = std::accumulate(result.cpuTimes.begin(), result.cpuTimes.end(), 0.0) / result.cpuTimes.size();
    }
    if (!result.drawCalls.empty())
    {
        result.averageDrawCalls = static_cast<int>(std::accumulate(result.drawCalls.begin(), result.drawCalls.end(), 0) / result.drawCalls.size());
    }
    if (!result.triangles.empty())
    {
        result.averageTriangles = static_cast<int>(std::accumulate(result.triangles.begin(), result.triangles.end(), 0) / result.triangles.size());
    }
    if (!result.instances.empty())
    {
        result.averageInstances = static_cast<int>(std::accumulate(result.instances.begin(), result.instances.end(), 0) / result.instances.size());
    }

    result.visibleObjects = m_GPUDrivenRenderer ? m_GPUDrivenRenderer->GetRenderCount() : 0;
    return result;
}

BenchmarkResult RenderingBenchmark::RunHybridBenchmark(const BenchmarkConfig& config)
{
    BenchmarkResult result;
    result.approach = "Hybrid";
    result.objectCount = config.objectCount;

    // Reset performance profiler
    PerformanceProfiler::GetInstance().BeginFrame();

    std::vector<int> cpuCulledObjects; // Declare outside the loop
    std::vector<int> gpuObjects; // Declare outside the loop
    
    for (int frame = 0; frame < config.benchmarkDuration; frame++)
    {
        BeginFrame();

        // Update camera
        UpdateCamera(16.67f);

        // Hybrid approach: CPU culling for distant objects, GPU for close objects
        cpuCulledObjects.clear(); // Clear for each frame
        gpuObjects.clear(); // Clear for each frame

        for (int i = 0; i < m_TestObjects.size(); i++)
        {
            const auto& obj = m_TestObjects[i];
            XMFLOAT3 objCenter = {
                (obj.boundingBoxMin.x + obj.boundingBoxMax.x) * 0.5f * obj.scale.x + obj.position.x,
                (obj.boundingBoxMin.y + obj.boundingBoxMax.y) * 0.5f * obj.scale.y + obj.position.y,
                (obj.boundingBoxMin.z + obj.boundingBoxMax.z) * 0.5f * obj.scale.z + obj.position.z
            };
            XMFLOAT3 toCamera = {
                m_CameraPosition.x - objCenter.x,
                m_CameraPosition.y - objCenter.y,
                m_CameraPosition.z - objCenter.z
            };
            float distance = sqrt(toCamera.x * toCamera.x + toCamera.y * toCamera.y + toCamera.z * toCamera.z);

            if (distance < 200.0f) // Close objects use GPU
            {
                gpuObjects.push_back(i);
            }
            else if (distance < 500.0f) // Medium distance use CPU
            {
                cpuCulledObjects.push_back(i);
            }
        }

        // Process CPU-culled objects
        for (int objIndex : cpuCulledObjects)
        {
            PerformanceProfiler::GetInstance().IncrementDrawCalls();
            PerformanceProfiler::GetInstance().AddTriangles(12);
            PerformanceProfiler::GetInstance().AddInstances(1);
        }

        // Process GPU objects
        if (m_GPUDrivenRenderer && !gpuObjects.empty())
        {
            std::vector<ObjectData> gpuObjectData;
            for (int index : gpuObjects)
            {
                gpuObjectData.push_back(m_TestObjects[index]);
            }

            m_GPUDrivenRenderer->UpdateObjects(m_Context, gpuObjectData);

            XMMATRIX viewMatrix = XMMatrixLookAtLH(
                XMLoadFloat3(&m_CameraPosition),
                XMLoadFloat3(&m_CameraTarget),
                XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)
            );
            XMMATRIX projectionMatrix = XMMatrixPerspectiveFovLH(XM_PIDIV4, 16.0f / 9.0f, 0.1f, 1000.0f);

            m_GPUDrivenRenderer->UpdateCamera(m_Context, m_CameraPosition, viewMatrix, projectionMatrix);

            PerformanceProfiler::GetInstance().IncrementDrawCalls();
            PerformanceProfiler::GetInstance().AddTriangles(static_cast<uint32_t>(gpuObjects.size()) * 12);
            PerformanceProfiler::GetInstance().AddInstances(static_cast<uint32_t>(gpuObjects.size()));
        }

        EndFrame();
        RecordMetrics(result);

        // Update progress
        m_Progress = static_cast<double>(frame + 1) / static_cast<double>(config.benchmarkDuration);
        m_Status = "Hybrid Benchmark: Frame " + std::to_string(frame + 1) + "/" + std::to_string(config.benchmarkDuration);
    }

    // Calculate averages
    if (!result.frameTimes.empty())
    {
        result.averageFrameTime = std::accumulate(result.frameTimes.begin(), result.frameTimes.end(), 0.0) / result.frameTimes.size();
        result.averageFPS = 1000.0 / result.averageFrameTime;
    }
    if (!result.gpuTimes.empty())
    {
        result.averageGPUTime = std::accumulate(result.gpuTimes.begin(), result.gpuTimes.end(), 0.0) / result.gpuTimes.size();
    }
    if (!result.cpuTimes.empty())
    {
        result.averageCPUTime = std::accumulate(result.cpuTimes.begin(), result.cpuTimes.end(), 0.0) / result.cpuTimes.size();
    }
    if (!result.drawCalls.empty())
    {
        result.averageDrawCalls = static_cast<int>(std::accumulate(result.drawCalls.begin(), result.drawCalls.end(), 0) / result.drawCalls.size());
    }
    if (!result.triangles.empty())
    {
        result.averageTriangles = static_cast<int>(std::accumulate(result.triangles.begin(), result.triangles.end(), 0) / result.triangles.size());
    }
    if (!result.instances.empty())
    {
        result.averageInstances = static_cast<int>(std::accumulate(result.instances.begin(), result.instances.end(), 0) / result.instances.size());
    }

    result.visibleObjects = static_cast<int>(cpuCulledObjects.size() + (m_GPUDrivenRenderer ? m_GPUDrivenRenderer->GetRenderCount() : 0));
    return result;
}

std::vector<BenchmarkResult> RenderingBenchmark::RunBenchmarkSuite()
{
    std::vector<BenchmarkResult> results;
    std::vector<int> objectCounts = { 100, 500, 1000, 5000, 10000 };
    std::vector<BenchmarkConfig::RenderingApproach> approaches = {
        BenchmarkConfig::RenderingApproach::CPU_DRIVEN,
        BenchmarkConfig::RenderingApproach::GPU_DRIVEN,
        BenchmarkConfig::RenderingApproach::HYBRID
    };

    int totalTests = static_cast<int>(objectCounts.size() * approaches.size());
    int currentTest = 0;

    for (int objectCount : objectCounts)
    {
        for (auto approach : approaches)
        {
            BenchmarkConfig config;
            config.approach = approach;
            config.objectCount = objectCount;
            config.benchmarkDuration = 300; // 5 seconds at 60 FPS
            config.enableFrustumCulling = true;
            config.enableLOD = true;
            config.enableOcclusionCulling = false;
            config.sceneName = "Benchmark Scene " + std::to_string(objectCount) + " objects";
            config.outputDirectory = "./benchmark_results/";

            m_Status = "Running " + std::to_string(objectCount) + " objects with " + 
                      (approach == BenchmarkConfig::RenderingApproach::CPU_DRIVEN ? "CPU" :
                       approach == BenchmarkConfig::RenderingApproach::GPU_DRIVEN ? "GPU" : "Hybrid") + " approach";

            BenchmarkResult result = RunBenchmark(config);
            results.push_back(result);

            currentTest++;
            m_Progress = static_cast<double>(currentTest) / static_cast<double>(totalTests);
        }
    }

    return results;
}

bool RenderingBenchmark::SaveResults(const std::vector<BenchmarkResult>& results, const std::string& filename)
{
    std::ofstream file(filename);
    if (!file.is_open())
    {
        LOG_ERROR("Failed to open file for writing: " + filename);
        return false;
    }

    WriteCSVHeader(file);

    for (const auto& result : results)
    {
        WriteResultToCSV(result, file);
    }

    file.close();
    LOG("Benchmark results saved to: " + filename);
    return true;
}

bool RenderingBenchmark::SaveComparisonReport(const std::vector<BenchmarkResult>& results, const std::string& filename)
{
    std::ofstream file(filename);
    if (!file.is_open())
    {
        LOG_ERROR("Failed to open file for writing: " + filename);
        return false;
    }

    file << "GPU-Driven Rendering Performance Comparison Report\n";
    file << "==================================================\n\n";

    // Group results by object count
    std::map<int, std::vector<BenchmarkResult>> groupedResults;
    for (const auto& result : results)
    {
        groupedResults[result.objectCount].push_back(result);
    }

    for (const auto& group : groupedResults)
    {
        int objectCount = group.first;
        const auto& groupResults = group.second;

        file << "Object Count: " << objectCount << "\n";
        file << "===============\n\n";

        file << "Approach          FPS     Frame Time  GPU Time   CPU Time   Draw Calls  Triangles  Visible\n";
        file << "                  (avg)   (ms)        (ms)       (ms)       (avg)       (avg)      Objects\n";
        file << "--------------------------------------------------------------------------------\n";

        for (const auto& result : groupResults)
        {
            file << std::left << std::setw(16) << result.approach
                 << std::right << std::setw(8) << std::fixed << std::setprecision(1) << result.averageFPS
                 << std::setw(11) << std::setprecision(2) << result.averageFrameTime
                 << std::setw(10) << std::setprecision(2) << result.averageGPUTime
                 << std::setw(10) << std::setprecision(2) << result.averageCPUTime
                 << std::setw(12) << result.averageDrawCalls
                 << std::setw(11) << result.averageTriangles
                 << std::setw(10) << result.visibleObjects << "\n";
        }

        file << "\n";

        // Calculate improvements
        if (groupResults.size() >= 2)
        {
            auto cpuResult = std::find_if(groupResults.begin(), groupResults.end(),
                [](const BenchmarkResult& r) { return r.approach == "CPU-Driven"; });
            auto gpuResult = std::find_if(groupResults.begin(), groupResults.end(),
                [](const BenchmarkResult& r) { return r.approach == "GPU-Driven"; });

            if (cpuResult != groupResults.end() && gpuResult != groupResults.end())
            {
                double fpsImprovement = ((gpuResult->averageFPS - cpuResult->averageFPS) / cpuResult->averageFPS) * 100.0;
                double frameTimeImprovement = ((cpuResult->averageFrameTime - gpuResult->averageFrameTime) / cpuResult->averageFrameTime) * 100.0;

                file << "Performance Improvements:\n";
                file << "FPS Improvement: " << std::fixed << std::setprecision(1) << fpsImprovement << "%\n";
                file << "Frame Time Improvement: " << std::setprecision(1) << frameTimeImprovement << "%\n\n";
            }
        }
    }

    file.close();
    LOG("Comparison report saved to: " + filename);
    return true;
}

void RenderingBenchmark::GenerateTestScene(int objectCount, std::vector<ObjectData>& objects)
{
    objects.clear();
    objects.reserve(objectCount);

    if (objectCount <= 1000)
    {
        GenerateGridScene(objectCount, objects);
    }
    else if (objectCount <= 10000)
    {
        GenerateRandomScene(objectCount, objects);
    }
    else
    {
        GenerateStressTestScene(objectCount, objects);
    }
}

void RenderingBenchmark::GenerateGridScene(int objectCount, std::vector<ObjectData>& objects)
{
    int gridSize = static_cast<int>(sqrt(objectCount));
    float spacing = 10.0f;

    for (int x = 0; x < gridSize && objects.size() < objectCount; x++)
    {
        for (int z = 0; z < gridSize && objects.size() < objectCount; z++)
        {
            ObjectData obj;
            obj.position = { x * spacing - (gridSize * spacing * 0.5f), 0.0f, z * spacing - (gridSize * spacing * 0.5f) };
            obj.scale = { 1.0f, 1.0f, 1.0f };
            obj.rotation = { 0.0f, 0.0f, 0.0f };
            obj.boundingBoxMin = { -0.5f, -0.5f, -0.5f };
            obj.boundingBoxMax = { 0.5f, 0.5f, 0.5f };
            obj.objectIndex = static_cast<uint32_t>(objects.size());

            objects.push_back(obj);
        }
    }
}

void RenderingBenchmark::GenerateRandomScene(int objectCount, std::vector<ObjectData>& objects)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> posDist(-500.0f, 500.0f);
    std::uniform_real_distribution<float> scaleDist(0.5f, 2.0f);

    for (int i = 0; i < objectCount; i++)
    {
        ObjectData obj;
        obj.position = { posDist(gen), 0.0f, posDist(gen) };
        obj.scale = { scaleDist(gen), scaleDist(gen), scaleDist(gen) };
        obj.rotation = { 0.0f, 0.0f, 0.0f };
        obj.boundingBoxMin = { -0.5f, -0.5f, -0.5f };
        obj.boundingBoxMax = { 0.5f, 0.5f, 0.5f };
        obj.objectIndex = static_cast<uint32_t>(i);

        objects.push_back(obj);
    }
}

void RenderingBenchmark::GenerateStressTestScene(int objectCount, std::vector<ObjectData>& objects)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> posDist(-1000.0f, 1000.0f);
    std::uniform_real_distribution<float> scaleDist(0.1f, 5.0f);

    for (int i = 0; i < objectCount; i++)
    {
        ObjectData obj;
        obj.position = { posDist(gen), posDist(gen) * 0.1f, posDist(gen) };
        obj.scale = { scaleDist(gen), scaleDist(gen), scaleDist(gen) };
        obj.rotation = { 0.0f, 0.0f, 0.0f };
        obj.boundingBoxMin = { -0.5f, -0.5f, -0.5f };
        obj.boundingBoxMax = { 0.5f, 0.5f, 0.5f };
        obj.objectIndex = static_cast<uint32_t>(i);

        objects.push_back(obj);
    }
}

void RenderingBenchmark::UpdateCamera(float deltaTime)
{
    m_CameraRotation += deltaTime * 0.001f; // Slow rotation

    float radius = 300.0f;
    m_CameraPosition.x = cos(m_CameraRotation) * radius;
    m_CameraPosition.z = sin(m_CameraRotation) * radius;
    m_CameraPosition.y = 50.0f + sin(m_CameraRotation * 2.0f) * 20.0f;
}

void RenderingBenchmark::BeginFrame()
{
    m_FrameStartTime = std::chrono::high_resolution_clock::now();
    PerformanceProfiler::GetInstance().BeginFrame();
}

void RenderingBenchmark::EndFrame()
{
    m_FrameEndTime = std::chrono::high_resolution_clock::now();
    PerformanceProfiler::GetInstance().EndFrame();
}

void RenderingBenchmark::RecordMetrics(BenchmarkResult& result)
{
    auto frameTime = std::chrono::duration_cast<std::chrono::microseconds>(m_FrameEndTime - m_FrameStartTime).count() / 1000.0;
    const auto& timing = PerformanceProfiler::GetInstance().GetLastFrameTiming();

    result.frameTimes.push_back(frameTime);
    result.gpuTimes.push_back(timing.gpuTime);
    result.cpuTimes.push_back(timing.cpuTime);
    result.drawCalls.push_back(timing.drawCalls);
    result.triangles.push_back(timing.triangles);
    result.instances.push_back(timing.instances);
    result.indirectDrawCalls.push_back(timing.indirectDrawCalls);
    result.computeDispatches.push_back(timing.computeDispatches);
    result.gpuMemoryUsage.push_back(timing.gpuMemoryUsage);
    result.cpuMemoryUsage.push_back(timing.cpuMemoryUsage);
    result.bandwidthUsage.push_back(timing.bandwidthUsage);

    // Calculate running averages
    if (!result.frameTimes.empty())
    {
        result.averageFrameTime = std::accumulate(result.frameTimes.begin(), result.frameTimes.end(), 0.0) / result.frameTimes.size();
        result.averageFPS = 1000.0 / result.averageFrameTime;
    }
    if (!result.gpuTimes.empty())
    {
        result.averageGPUTime = std::accumulate(result.gpuTimes.begin(), result.gpuTimes.end(), 0.0) / result.gpuTimes.size();
    }
    if (!result.cpuTimes.empty())
    {
        result.averageCPUTime = std::accumulate(result.cpuTimes.begin(), result.cpuTimes.end(), 0.0) / result.cpuTimes.size();
    }
    if (!result.drawCalls.empty())
    {
        result.averageDrawCalls = static_cast<int>(std::accumulate(result.drawCalls.begin(), result.drawCalls.end(), 0) / result.drawCalls.size());
    }
    if (!result.triangles.empty())
    {
        result.averageTriangles = static_cast<int>(std::accumulate(result.triangles.begin(), result.triangles.end(), 0) / result.triangles.size());
    }
    if (!result.instances.empty())
    {
        result.averageInstances = static_cast<int>(std::accumulate(result.instances.begin(), result.instances.end(), 0) / result.instances.size());
    }
    if (!result.indirectDrawCalls.empty())
    {
        result.averageIndirectDrawCalls = static_cast<int>(std::accumulate(result.indirectDrawCalls.begin(), result.indirectDrawCalls.end(), 0) / result.indirectDrawCalls.size());
    }
    if (!result.computeDispatches.empty())
    {
        result.averageComputeDispatches = static_cast<int>(std::accumulate(result.computeDispatches.begin(), result.computeDispatches.end(), 0) / result.computeDispatches.size());
    }
    if (!result.gpuMemoryUsage.empty())
    {
        result.averageGPUMemoryUsage = std::accumulate(result.gpuMemoryUsage.begin(), result.gpuMemoryUsage.end(), 0.0) / result.gpuMemoryUsage.size();
    }
    if (!result.cpuMemoryUsage.empty())
    {
        result.averageCPUMemoryUsage = std::accumulate(result.cpuMemoryUsage.begin(), result.cpuMemoryUsage.end(), 0.0) / result.cpuMemoryUsage.size();
    }
    if (!result.bandwidthUsage.empty())
    {
        result.averageBandwidthUsage = std::accumulate(result.bandwidthUsage.begin(), result.bandwidthUsage.end(), 0.0) / result.bandwidthUsage.size();
    }
}

void RenderingBenchmark::WriteCSVHeader(std::ofstream& file)
{
    file << "Approach,ObjectCount,VisibleObjects,AverageFPS,AverageFrameTime,AverageGPUTime,AverageCPUTime,"
         << "AverageDrawCalls,AverageTriangles,AverageInstances,AverageIndirectDrawCalls,AverageComputeDispatches,"
         << "AverageGPUMemoryUsage,AverageCPUMemoryUsage,AverageBandwidthUsage\n";
}

void RenderingBenchmark::WriteResultToCSV(const BenchmarkResult& result, std::ofstream& file)
{
    file << result.approach << ","
         << result.objectCount << ","
         << result.visibleObjects << ","
         << std::fixed << std::setprecision(2) << result.averageFPS << ","
         << std::setprecision(2) << result.averageFrameTime << ","
         << std::setprecision(2) << result.averageGPUTime << ","
         << std::setprecision(2) << result.averageCPUTime << ","
         << result.averageDrawCalls << ","
         << result.averageTriangles << ","
         << result.averageInstances << ","
         << result.averageIndirectDrawCalls << ","
         << result.averageComputeDispatches << ","
         << std::setprecision(2) << result.averageGPUMemoryUsage << ","
         << std::setprecision(2) << result.averageCPUMemoryUsage << ","
         << std::setprecision(2) << result.averageBandwidthUsage << "\n";
}

void RenderingBenchmark::GeneratePerformanceCharts(const std::vector<BenchmarkResult>& results)
{
    // Group results by object count for chart generation
    std::map<int, std::vector<BenchmarkResult>> groupedResults;
    for (const auto& result : results)
    {
        groupedResults[result.objectCount].push_back(result);
    }

    // Generate performance comparison charts
    for (const auto& group : groupedResults)
    {
        int objectCount = group.first;
        const auto& groupResults = group.second;

        // Find best performing approach for this object count
        auto bestResult = std::max_element(groupResults.begin(), groupResults.end(),
            [](const BenchmarkResult& a, const BenchmarkResult& b) {
                return a.averageFPS < b.averageFPS;
            });

        if (bestResult != groupResults.end())
        {
            LOG("Best performing approach for " + std::to_string(objectCount) + 
                " objects: " + bestResult->approach + " with " + 
                std::to_string(bestResult->averageFPS) + " FPS");
        }
    }

    LOG("Performance charts generated for " + std::to_string(groupedResults.size()) + " object count groups");
} 

void RenderingBenchmark::RunBenchmarkFrame(const BenchmarkConfig& config, BenchmarkResult& result, int currentFrame)
{
    // Only process up to the benchmark duration
    if (currentFrame >= config.benchmarkDuration) return;

    BeginFrame();
    UpdateCamera(16.67f); // Simulate 60 FPS

    if (config.approach == BenchmarkConfig::RenderingApproach::CPU_DRIVEN)
    {
        // CPU-driven frustum culling and rendering
        int visibleCount = 0;
        XMMATRIX viewMatrix = XMMatrixLookAtLH(
            XMLoadFloat3(&m_CameraPosition),
            XMLoadFloat3(&m_CameraTarget),
            XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)
        );
        XMMATRIX projectionMatrix = XMMatrixPerspectiveFovLH(XM_PIDIV4, 16.0f / 9.0f, 0.1f, 1000.0f);
        for (int i = 0; i < m_TestObjects.size(); i++)
        {
            const auto& obj = m_TestObjects[i];
            XMFLOAT3 worldMin = {
                obj.boundingBoxMin.x * obj.scale.x + obj.position.x,
                obj.boundingBoxMin.y * obj.scale.y + obj.position.y,
                obj.boundingBoxMin.z * obj.scale.z + obj.position.z
            };
            XMFLOAT3 worldMax = {
                obj.boundingBoxMax.x * obj.scale.x + obj.position.x,
                obj.boundingBoxMax.y * obj.scale.y + obj.position.y,
                obj.boundingBoxMax.z * obj.scale.z + obj.position.z
            };
            XMFLOAT3 objCenter = {
                (worldMin.x + worldMax.x) * 0.5f,
                (worldMin.y + worldMax.y) * 0.5f,
                (worldMin.z + worldMax.z) * 0.5f
            };
            XMFLOAT3 toCamera = {
                m_CameraPosition.x - objCenter.x,
                m_CameraPosition.y - objCenter.y,
                m_CameraPosition.z - objCenter.z
            };
            float distance = sqrt(toCamera.x * toCamera.x + toCamera.y * toCamera.y + toCamera.z * toCamera.z);
            if (distance < 500.0f)
            {
                visibleCount++;
                PerformanceProfiler::GetInstance().IncrementDrawCalls();
                PerformanceProfiler::GetInstance().AddTriangles(12);
                PerformanceProfiler::GetInstance().AddInstances(1);
            }
        }
        EndFrame();
        RecordMetrics(result);
        result.visibleObjects = visibleCount;
    }
    else if (config.approach == BenchmarkConfig::RenderingApproach::GPU_DRIVEN)
    {
        if (m_GPUDrivenRenderer)
        {
            m_GPUDrivenRenderer->UpdateObjects(m_Context, m_TestObjects);
            XMMATRIX viewMatrix = XMMatrixLookAtLH(
                XMLoadFloat3(&m_CameraPosition),
                XMLoadFloat3(&m_CameraTarget),
                XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)
            );
            XMMATRIX projectionMatrix = XMMatrixPerspectiveFovLH(XM_PIDIV4, 16.0f / 9.0f, 0.1f, 1000.0f);
            m_GPUDrivenRenderer->UpdateCamera(m_Context, m_CameraPosition, viewMatrix, projectionMatrix);
            
            PerformanceProfiler::GetInstance().IncrementDrawCalls();
            PerformanceProfiler::GetInstance().AddTriangles(static_cast<uint32_t>(m_TestObjects.size()) * 12);
            PerformanceProfiler::GetInstance().AddInstances(static_cast<uint32_t>(m_TestObjects.size()));
            EndFrame();
            RecordMetrics(result);
            result.visibleObjects = m_GPUDrivenRenderer->GetRenderCount();
        }
    }
    else if (config.approach == BenchmarkConfig::RenderingApproach::HYBRID)
    {
        int cpuCount = 0, gpuCount = 0;
        std::vector<int> cpuCulledObjects, gpuObjects;
        for (int i = 0; i < m_TestObjects.size(); i++)
        {
            const auto& obj = m_TestObjects[i];
            XMFLOAT3 objCenter = {
                (obj.boundingBoxMin.x + obj.boundingBoxMax.x) * 0.5f * obj.scale.x + obj.position.x,
                (obj.boundingBoxMin.y + obj.boundingBoxMax.y) * 0.5f * obj.scale.y + obj.position.y,
                (obj.boundingBoxMin.z + obj.boundingBoxMax.z) * 0.5f * obj.scale.z + obj.position.z
            };
            XMFLOAT3 toCamera = {
                m_CameraPosition.x - objCenter.x,
                m_CameraPosition.y - objCenter.y,
                m_CameraPosition.z - objCenter.z
            };
            float distance = sqrt(toCamera.x * toCamera.x + toCamera.y * toCamera.y + toCamera.z * toCamera.z);
            if (distance < 200.0f)
            {
                gpuObjects.push_back(i);
            }
            else if (distance < 500.0f)
            {
                cpuCulledObjects.push_back(i);
            }
        }
        for (int objIndex : cpuCulledObjects)
        {
            cpuCount++;
            PerformanceProfiler::GetInstance().IncrementDrawCalls();
            PerformanceProfiler::GetInstance().AddTriangles(12);
            PerformanceProfiler::GetInstance().AddInstances(1);
        }
        if (m_GPUDrivenRenderer && !gpuObjects.empty())
        {
            std::vector<ObjectData> gpuObjectData;
            for (int index : gpuObjects)
            {
                gpuObjectData.push_back(m_TestObjects[index]);
            }
            m_GPUDrivenRenderer->UpdateObjects(m_Context, gpuObjectData);
            XMMATRIX viewMatrix = XMMatrixLookAtLH(
                XMLoadFloat3(&m_CameraPosition),
                XMLoadFloat3(&m_CameraTarget),
                XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)
            );
            XMMATRIX projectionMatrix = XMMatrixPerspectiveFovLH(XM_PIDIV4, 16.0f / 9.0f, 0.1f, 1000.0f);
            m_GPUDrivenRenderer->UpdateCamera(m_Context, m_CameraPosition, viewMatrix, projectionMatrix);
            
            gpuCount = m_GPUDrivenRenderer->GetRenderCount();
            PerformanceProfiler::GetInstance().IncrementDrawCalls();
            PerformanceProfiler::GetInstance().AddTriangles(gpuCount * 12);
            PerformanceProfiler::GetInstance().AddInstances(gpuCount);
        }
        EndFrame();
        RecordMetrics(result);
        result.visibleObjects = cpuCount + gpuCount;
    }
}

bool RenderingBenchmark::CreateDummyBuffers()
{
    // Create a simple dummy vertex buffer (just a cube)
    struct DummyVertex
    {
        float x, y, z;
        float nx, ny, nz;
        float u, v;
    };

    // Cube vertices
    DummyVertex vertices[] = {
        // Front face
        {-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f},
        { 0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f},
        { 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f},
        {-0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f},
        // Back face
        {-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f},
        { 0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f},
        { 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f},
        {-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f}
    };

    D3D11_BUFFER_DESC vertexBufferDesc = {};
    vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexBufferDesc.ByteWidth = sizeof(vertices);
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vertexData = {};
    vertexData.pSysMem = vertices;

    HRESULT result = m_Device->CreateBuffer(&vertexBufferDesc, &vertexData, &m_DummyVertexBuffer);
    if (FAILED(result))
    {
        LOG_ERROR("Failed to create dummy vertex buffer - HRESULT: " + std::to_string(result));
        return false;
    }

    // Create dummy index buffer
    uint32_t indices[] = {
        0, 1, 2, 2, 3, 0,  // Front
        4, 5, 6, 6, 7, 4,  // Back
        0, 4, 7, 7, 3, 0,  // Left
        1, 5, 6, 6, 2, 1,  // Right
        3, 2, 6, 6, 7, 3,  // Top
        0, 1, 5, 5, 4, 0   // Bottom
    };

    D3D11_BUFFER_DESC indexBufferDesc = {};
    indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    indexBufferDesc.ByteWidth = sizeof(indices);
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA indexData = {};
    indexData.pSysMem = indices;

    result = m_Device->CreateBuffer(&indexBufferDesc, &indexData, &m_DummyIndexBuffer);
    if (FAILED(result))
    {
        LOG_ERROR("Failed to create dummy index buffer - HRESULT: " + std::to_string(result));
        return false;
    }

    // Create dummy vertex shader
    const char* vertexShaderCode = R"(
        struct VS_INPUT
        {
            float3 position : POSITION;
            float3 normal : NORMAL;
            float2 texcoord : TEXCOORD0;
        };

        struct VS_OUTPUT
        {
            float4 position : SV_POSITION;
            float3 normal : NORMAL;
            float2 texcoord : TEXCOORD0;
        };

        VS_OUTPUT main(VS_INPUT input)
        {
            VS_OUTPUT output;
            output.position = float4(input.position, 1.0f);
            output.normal = input.normal;
            output.texcoord = input.texcoord;
            return output;
        }
    )";

    ID3DBlob* vertexShaderBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;
    result = D3DCompile(vertexShaderCode, strlen(vertexShaderCode), nullptr, nullptr, nullptr, 
                        "main", "vs_4_0", 0, 0, &vertexShaderBlob, &errorBlob);
    if (FAILED(result))
    {
        LOG_ERROR("Failed to compile dummy vertex shader");
        if (errorBlob) errorBlob->Release();
        return false;
    }

    result = m_Device->CreateVertexShader(vertexShaderBlob->GetBufferPointer(), 
                                         vertexShaderBlob->GetBufferSize(), nullptr, &m_DummyVertexShader);
    if (FAILED(result))
    {
        LOG_ERROR("Failed to create dummy vertex shader");
        vertexShaderBlob->Release();
        return false;
    }

    // Create dummy pixel shader
    const char* pixelShaderCode = R"(
        struct PS_INPUT
        {
            float4 position : SV_POSITION;
            float3 normal : NORMAL;
            float2 texcoord : TEXCOORD0;
        };

        float4 main(PS_INPUT input) : SV_TARGET
        {
            return float4(1.0f, 1.0f, 1.0f, 1.0f);
        }
    )";

    ID3DBlob* pixelShaderBlob = nullptr;
    result = D3DCompile(pixelShaderCode, strlen(pixelShaderCode), nullptr, nullptr, nullptr, 
                        "main", "ps_4_0", 0, 0, &pixelShaderBlob, &errorBlob);
    if (FAILED(result))
    {
        LOG_ERROR("Failed to compile dummy pixel shader");
        if (errorBlob) errorBlob->Release();
        vertexShaderBlob->Release();
        return false;
    }

    result = m_Device->CreatePixelShader(pixelShaderBlob->GetBufferPointer(), 
                                        pixelShaderBlob->GetBufferSize(), nullptr, &m_DummyPixelShader);
    pixelShaderBlob->Release();
    if (FAILED(result))
    {
        LOG_ERROR("Failed to create dummy pixel shader");
        vertexShaderBlob->Release();
        return false;
    }

    // Create dummy input layout (keep vertexShaderBlob alive until after this)
    D3D11_INPUT_ELEMENT_DESC inputLayoutDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };

    result = m_Device->CreateInputLayout(inputLayoutDesc, 3, vertexShaderBlob->GetBufferPointer(), 
                                        vertexShaderBlob->GetBufferSize(), &m_DummyInputLayout);
    vertexShaderBlob->Release(); // Release after creating input layout
    if (FAILED(result))
    {
        LOG_ERROR("Failed to create dummy input layout - HRESULT: " + std::to_string(result));
        return false;
    }

    LOG("Dummy buffers created successfully for benchmarking");
    return true;
}

void RenderingBenchmark::ReleaseDummyBuffers()
{
    if (m_DummyVertexBuffer) { m_DummyVertexBuffer->Release(); m_DummyVertexBuffer = nullptr; }
    if (m_DummyIndexBuffer) { m_DummyIndexBuffer->Release(); m_DummyIndexBuffer = nullptr; }
    if (m_DummyVertexShader) { m_DummyVertexShader->Release(); m_DummyVertexShader = nullptr; }
    if (m_DummyPixelShader) { m_DummyPixelShader->Release(); m_DummyPixelShader = nullptr; }
    if (m_DummyInputLayout) { m_DummyInputLayout->Release(); m_DummyInputLayout = nullptr; }
} 