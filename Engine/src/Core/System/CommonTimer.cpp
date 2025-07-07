#include "CommonTimer.h"
#include "Logger.h"

CommonTimer& CommonTimer::GetInstance()
{
    static CommonTimer instance;
    return instance;
}

CommonTimer::CommonTimer()
    : m_Frequency(0.0)
    , m_Initialized(false)
{
    LARGE_INTEGER freq;
    if (QueryPerformanceFrequency(&freq))
    {
        m_Frequency = static_cast<double>(freq.QuadPart);
        m_Initialized = true;
        LOG("CommonTimer initialized with frequency: " + std::to_string(m_Frequency));
    }
    else
    {
        LOG_ERROR("Failed to initialize CommonTimer - QueryPerformanceFrequency failed");
    }
}

CommonTimer::~CommonTimer()
{
}

double CommonTimer::GetCurrentTimeMs() const
{
    if (!m_Initialized) return 0.0;
    
    LARGE_INTEGER currentTime;
    if (QueryPerformanceCounter(&currentTime))
    {
        return static_cast<double>(currentTime.QuadPart) * 1000.0 / m_Frequency;
    }
    return 0.0;
}

double CommonTimer::CalculateFPS(double frameTimeMs) const
{
    if (frameTimeMs <= 0.0) return 0.0;
    return 1000.0 / frameTimeMs;
}

LARGE_INTEGER CommonTimer::GetCurrentTimestamp() const
{
    LARGE_INTEGER timestamp;
    QueryPerformanceCounter(&timestamp);
    return timestamp;
} 