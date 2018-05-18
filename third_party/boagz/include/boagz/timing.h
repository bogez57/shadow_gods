#pragma once
#define WIN32_LEAN_AND_MEAN
#include "Windows.h"

#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}

namespace bgz
{
    class Timer
    {
    public:
        Timer() = default;

        auto StartClockTimer() -> unsigned long long;
        auto UpdateClockTimer() -> void;
        auto StartCPUCycleTimer() -> unsigned long long;
        auto UpdateCPUCycleTimer() -> unsigned long long;

        auto ElapsedTimeInMS() -> float;
        auto ElapsedTimeInSecs() -> float;

    private:
        unsigned long long ClockTicksPerSecond;
        unsigned long long LastClockTickCount;
        unsigned long long EndingClockTickCount;

        unsigned long long LastCPUCycleCount;
        unsigned long long EndingCPUCycleCount;

        float MilliSecsElapsed;
        float SecondsElapsed;
    };

    auto Timer::StartClockTimer() -> unsigned long long
    {
        LARGE_INTEGER Win64CurrentTickCount;
        QueryPerformanceCounter(&Win64CurrentTickCount);

        LARGE_INTEGER PerformanceFreq;
        QueryPerformanceFrequency(&PerformanceFreq);

        LastClockTickCount= (unsigned long long)Win64CurrentTickCount.QuadPart;
        ClockTicksPerSecond = (unsigned long long)PerformanceFreq.QuadPart;

        return LastClockTickCount;
    }

    auto Timer::UpdateClockTimer() -> void
    {
        LARGE_INTEGER Win64CurrentTickCount;
        QueryPerformanceCounter(&Win64CurrentTickCount);
        EndingClockTickCount = (unsigned long long)Win64CurrentTickCount.QuadPart;

        MilliSecsElapsed = ((float)(EndingClockTickCount - LastClockTickCount) / (float)ClockTicksPerSecond) * 1000;
        SecondsElapsed = ((float)(EndingClockTickCount - LastClockTickCount) / (float)ClockTicksPerSecond);

        LastClockTickCount = EndingClockTickCount;
    }

    auto Timer::StartCPUCycleTimer() -> unsigned long long
    {
        LastCPUCycleCount = __rdtsc();

        return LastCPUCycleCount;
    }

    auto Timer::UpdateCPUCycleTimer() -> unsigned long long
    {
        EndingCPUCycleCount = __rdtsc();

        unsigned long long CyclesElapsed = EndingCPUCycleCount - LastCPUCycleCount;

        LastCPUCycleCount = EndingCPUCycleCount;

        return CyclesElapsed;
    }

    auto Timer::ElapsedTimeInMS() -> float
    {
        return MilliSecsElapsed;
    } 
    
    auto Timer::ElapsedTimeInSecs() -> float 
    {
        return SecondsElapsed;
    } 
} // namespace bgz