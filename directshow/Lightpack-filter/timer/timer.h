#ifndef __TIMER_H__
#define __TIMER_H__

#include <Windows.h>

class Timer {
public:
    Timer() 
        : mStart(0)
        , mElapsed(0)
    {
        unsigned __int64 freq;
        QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
        mTimerFreq = (1.0 / freq);
    }

    void start() {
        getNow(mStart);
    }

    double elapsed() {
        if (!mStart) return mElapsed;
        unsigned __int64 endTime;
        QueryPerformanceCounter((LARGE_INTEGER *)&endTime);
        return (endTime - mStart) * mTimerFreq;
    }

    void restart() {
        reset();
        start();
    }

    void stop() {
        mElapsed = elapsed();
        mStart = 0;
    }

    void reset() {
        mStart = 0;
        mElapsed = 0;
    }

private:
    double mTimerFreq;

    void getNow(unsigned __int64& value) {
        QueryPerformanceCounter((LARGE_INTEGER *)&value);
    }

    unsigned __int64 mStart;
    double mElapsed;
};

#endif // __TIMER_H__