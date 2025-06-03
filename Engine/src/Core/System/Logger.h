#pragma once

#include <string>
#include <fstream>
#include <iostream>

class Logger
{
public:
    static Logger& GetInstance()
    {
        static Logger instance;
        return instance;
    }

    void Initialize(const std::string& filename = "engine.log")
    {
        m_LogFile.open(filename);
        if (!m_LogFile.is_open())
        {
            std::cerr << "Failed to open log file: " << filename << std::endl;
        }
    }

    void Shutdown()
    {
        if (m_LogFile.is_open())
        {
            m_LogFile.close();
        }
    }

    template<typename T>
    void Log(const T& message)
    {
        std::cout << message << std::endl;
        if (m_LogFile.is_open())
        {
            m_LogFile << message << std::endl;
            m_LogFile.flush();
        }
    }

    template<typename T>
    void LogError(const T& message)
    {
        std::cerr << "ERROR: " << message << std::endl;
        if (m_LogFile.is_open())
        {
            m_LogFile << "ERROR: " << message << std::endl;
            m_LogFile.flush();
        }
    }

private:
    Logger() = default;
    ~Logger() { Shutdown(); }
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::ofstream m_LogFile;
};

// Convenience macros
#define LOG(message) Logger::GetInstance().Log(message)
#define LOG_ERROR(message) Logger::GetInstance().LogError(message) 