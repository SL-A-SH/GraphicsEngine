#ifndef _TIMER_H_
#define _TIMER_H_

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