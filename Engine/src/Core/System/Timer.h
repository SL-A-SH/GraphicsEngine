#ifndef _TIMER_H_
#define _TIMER_H_

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif

#include <windows.h>

class Timer
{
public:
    Timer();
    Timer(const Timer&);
    ~Timer();

    bool Initialize();
    void Frame();

    float const GetTime();
    float const GetFps();

private:
    float m_frequency;
    INT64 m_startTime;
    float m_frameTime;

    int m_fps;
    int m_count;
    float m_secondCounter;
};

#endif