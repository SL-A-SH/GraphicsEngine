#include "PerformanceLogger.h"
#include "Logger.h"
#include <chrono>
#include <iomanip>
#include <sstream>

PerformanceLogger& PerformanceLogger::GetInstance()
{
    static PerformanceLogger instance;
    return instance;
}

PerformanceLogger::PerformanceLogger()
    : m_Initialized(false)
{
}

PerformanceLogger::~PerformanceLogger()
{
    Shutdown();
}

void PerformanceLogger::Initialize()
{
    if (m_Initialized) return;
    
    // Create log file with timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << "performance_log_" << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S") << ".txt";
    
    m_LogFile.open(ss.str(), std::ios::out);
    if (m_LogFile.is_open())
    {
        m_Initialized = true;
        WriteToFile("=== PERFORMANCE LOGGING STARTED ===");
        WriteToFile("Timestamp: " + GetTimestamp());
        WriteToFile("=====================================");
        LOG("Performance logging started: " + ss.str());
    }
    else
    {
        LOG_ERROR("Failed to open performance log file: " + ss.str());
    }
}

void PerformanceLogger::Shutdown()
{
    if (m_Initialized && m_LogFile.is_open())
    {
        WriteToFile("=== PERFORMANCE LOGGING ENDED ===");
        m_LogFile.close();
        m_Initialized = false;
        LOG("Performance logging stopped");
    }
}

void PerformanceLogger::LogPerformanceMetrics(const std::string& tabName, 
                                            double profilerFPS, 
                                            double viewportFPS, 
                                            double cpuTime, 
                                            double gpuTime,
                                            int drawCalls,
                                            int triangles,
                                            int instances)
{
    if (!m_Initialized) return;
    
    std::stringstream ss;
    ss << "\n--- PERFORMANCE METRICS [" << tabName << "] ---" << std::endl;
    ss << "Timestamp: " << GetTimestamp() << std::endl;
    ss << "Profiler FPS: " << std::fixed << std::setprecision(1) << profilerFPS << std::endl;
    ss << "Viewport FPS: " << std::fixed << std::setprecision(1) << viewportFPS << std::endl;
    ss << "CPU Time: " << std::fixed << std::setprecision(2) << cpuTime << " ms" << std::endl;
    ss << "GPU Time: " << std::fixed << std::setprecision(2) << gpuTime << " ms" << std::endl;
    ss << "Draw Calls: " << drawCalls << std::endl;
    ss << "Triangles: " << triangles << std::endl;
    ss << "Instances: " << instances << std::endl;
    ss << "----------------------------------------" << std::endl;
    
    WriteToFile(ss.str());
}

void PerformanceLogger::LogTabFocus(const std::string& tabName)
{
    if (!m_Initialized) return;
    
    std::stringstream ss;
    ss << "\n*** TAB FOCUS CHANGED ***" << std::endl;
    ss << "Timestamp: " << GetTimestamp() << std::endl;
    ss << "Active Tab: " << tabName << std::endl;
    ss << "**************************" << std::endl;
    
    WriteToFile(ss.str());
}

void PerformanceLogger::LogBenchmarkEvent(const std::string& event)
{
    if (!m_Initialized) return;
    
    std::stringstream ss;
    ss << "\n### BENCHMARK EVENT ###" << std::endl;
    ss << "Timestamp: " << GetTimestamp() << std::endl;
    ss << "Event: " << event << std::endl;
    ss << "#######################" << std::endl;
    
    WriteToFile(ss.str());
}

std::string PerformanceLogger::GetTimestamp()
{
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

void PerformanceLogger::WriteToFile(const std::string& message)
{
    std::lock_guard<std::mutex> lock(m_LogMutex);
    if (m_LogFile.is_open())
    {
        m_LogFile << message << std::endl;
        m_LogFile.flush();
    }
} 