#include "timer.h"
#include "Logger.h"
#include "CommonTimer.h"

Timer::Timer()
{
}


Timer::Timer(const Timer& other)
{
}


Timer::~Timer()
{
}

bool Timer::Initialize()
{
    // Use CommonTimer for consistent frequency
    m_frequency = static_cast<float>(CommonTimer::GetInstance().GetFrequency());
    if (m_frequency == 0)
    {
        return false;
    }

    // Get the initial start time using CommonTimer
    LARGE_INTEGER startTime = CommonTimer::GetInstance().GetCurrentTimestamp();
    m_startTime = startTime.QuadPart;

    m_fps = 0;
    m_count = 0;
    m_secondCounter = 0.0f;
    m_frameTime = 0.0f;

    return true;
}

void Timer::Frame()
{
    INT64 currentTime;
    INT64 elapsedTicks;

    m_count++;

    // Query the current time using CommonTimer
    LARGE_INTEGER currentTimeStamp = CommonTimer::GetInstance().GetCurrentTimestamp();
    currentTime = currentTimeStamp.QuadPart;

    // Calculate the difference in time since the last time we queried for the current time.
    elapsedTicks = currentTime - m_startTime;

    // Calculate the frame time.
    m_frameTime = (float)elapsedTicks / m_frequency;

    // Add to the second counter
    m_secondCounter += m_frameTime;

    // If one second has passed
    if (m_secondCounter >= 1.0f)
    {
        m_fps = m_count;
        m_count = 0;
        m_secondCounter = 0.0f;
    }

    // Restart the timer.
    m_startTime = currentTime;

    return;
}

float const Timer::GetTime()
{
    return m_frameTime;
}

float const Timer::GetFps()
{
    return m_fps;
}