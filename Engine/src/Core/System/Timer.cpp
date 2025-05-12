#include "timer.h"


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
    INT64 frequency;


    // Get the cycles per second speed for this system.
    QueryPerformanceFrequency((LARGE_INTEGER*)&frequency);
    if (frequency == 0)
    {
        return false;
    }

    // Store it in floating point.
    m_frequency = (float)frequency;

    // Get the initial start time.
    QueryPerformanceCounter((LARGE_INTEGER*)&m_startTime);

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

    // Query the current time.
    QueryPerformanceCounter((LARGE_INTEGER*)&currentTime);

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