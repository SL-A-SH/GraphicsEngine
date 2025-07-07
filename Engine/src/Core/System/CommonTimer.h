#ifndef _COMMON_TIMER_H_
#define _COMMON_TIMER_H_

#include <windows.h>

class CommonTimer
{
public:
    static CommonTimer& GetInstance();
    
    // Get current time in milliseconds
    double GetCurrentTimeMs() const;
    
    // Get frequency for conversion
    double GetFrequency() const { return m_Frequency; }
    
    // Calculate FPS from frame time
    double CalculateFPS(double frameTimeMs) const;
    
    // Get high-resolution timestamp
    LARGE_INTEGER GetCurrentTimestamp() const;

private:
    CommonTimer();
    ~CommonTimer();
    
    // Prevent copying
    CommonTimer(const CommonTimer&) = delete;
    CommonTimer& operator=(const CommonTimer&) = delete;
    
    double m_Frequency;
    bool m_Initialized;
};

#endif 