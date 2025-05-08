#ifndef _TIMERCLASS_H_
#define _TIMERCLASS_H_

#include <windows.h>

class Timer
{
public:
    Timer();
    Timer(const Timer&);
    ~Timer();

    bool Initialize();
    void Frame();

    float GetTime();

private:
    float m_frequency;
    INT64 m_startTime;
    float m_frameTime;
};

#endif