#ifndef _PERFORMANCE_LOGGER_H_
#define _PERFORMANCE_LOGGER_H_

#include <string>
#include <fstream>
#include <mutex>

class PerformanceLogger
{
public:
    static PerformanceLogger& GetInstance();
    
    // Initialize logging
    void Initialize();
    void Shutdown();
    
    // Log performance metrics
    void LogPerformanceMetrics(const std::string& tabName, 
                              double profilerFPS, 
                              double viewportFPS, 
                              double cpuTime, 
                              double gpuTime,
                              int drawCalls,
                              int triangles,
                              int instances);
    
    // Log tab focus changes
    void LogTabFocus(const std::string& tabName);
    
    // Log benchmark events
    void LogBenchmarkEvent(const std::string& event);

private:
    PerformanceLogger();
    ~PerformanceLogger();
    
    // Prevent copying
    PerformanceLogger(const PerformanceLogger&) = delete;
    PerformanceLogger& operator=(const PerformanceLogger&) = delete;
    
    std::ofstream m_LogFile;
    std::mutex m_LogMutex;
    bool m_Initialized;
    
    // Helper methods
    std::string GetTimestamp();
    void WriteToFile(const std::string& message);
};

#endif 