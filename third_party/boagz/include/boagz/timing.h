#pragma once

#define WIN32_LEAN_AND_MEAN
#include "Windows.h"
#include <stdint.h>

#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}

inline auto 
MonitorRefreshHz() -> unsigned int
{
    DEVMODE DeviceMode{};
    EnumDisplaySettings(0, ENUM_CURRENT_SETTINGS, &DeviceMode);
    unsigned int MonitorRefreshRate = (unsigned int)DeviceMode.dmDisplayFrequency;

    return MonitorRefreshRate;
}

class Timer
{
public:
    Timer() = default;

    auto Init() -> void;
    auto SecondsElapsed() -> float;
    auto MilliSecondsElapsed() -> float;
    auto UpdateTimeCount() -> void;

private:
    bool IsInitialized;
    uint64_t ClockTicksPerSecond;
    uint64_t LastClockTickCount;
};

inline auto 
Timer::Init() -> void
{
    this->IsInitialized = true;

    LARGE_INTEGER CPUPerformancefFreq;
    QueryPerformanceFrequency(&CPUPerformancefFreq);
    this->ClockTicksPerSecond = (uint64_t)CPUPerformancefFreq.QuadPart;

    LARGE_INTEGER Win64CurrentTickCount;
    QueryPerformanceCounter(&Win64CurrentTickCount);
    this->LastClockTickCount = (uint64_t)Win64CurrentTickCount.QuadPart;
}

inline auto 
Timer::SecondsElapsed() -> float
{
    Assert(this->IsInitialized);

    LARGE_INTEGER Win64CurrentTickCount;
    QueryPerformanceCounter(&Win64CurrentTickCount);
    uint64_t TimeElapsedInClockTicks{(uint64_t)Win64CurrentTickCount.QuadPart - this->LastClockTickCount};

    float SecondsElapsed{((float)TimeElapsedInClockTicks / (float)this->ClockTicksPerSecond)};

    return SecondsElapsed;
}
inline auto 
Timer::MilliSecondsElapsed() -> float
{
    Assert(this->IsInitialized);

    LARGE_INTEGER Win64CurrentTickCount;
    QueryPerformanceCounter(&Win64CurrentTickCount);
    uint64_t TimeElapsedInClockTicks{(uint64_t)Win64CurrentTickCount.QuadPart - this->LastClockTickCount};

    float MilliSecondsElapsed = (1000.0f * ((float)TimeElapsedInClockTicks / (float)ClockTicksPerSecond));

    return MilliSecondsElapsed;
}

inline auto 
Timer::UpdateTimeCount() -> void
{
    LARGE_INTEGER Win64CurrentTickCount;
    QueryPerformanceCounter(&Win64CurrentTickCount);

    this->LastClockTickCount = (uint64_t)Win64CurrentTickCount.QuadPart;
}

inline auto 
SecondsElapsedSince(uint64_t StartingClockTickCount, float ClockTicksPerSecond) -> float
{
    LARGE_INTEGER Win64CurrentTickCount;
    QueryPerformanceCounter(&Win64CurrentTickCount);
    uint64_t TimeElapsedInClockTicks{(uint64_t)Win64CurrentTickCount.QuadPart - StartingClockTickCount};

    float SecondsElapsed{((float)TimeElapsedInClockTicks / ClockTicksPerSecond)};

    return SecondsElapsed;
}

inline auto 
MilliSecondsElapsedSince(uint64_t StartingClockTickCount, float ClockTicksPerSecond) -> float
{
    LARGE_INTEGER Win64CurrentTickCount;
    QueryPerformanceCounter(&Win64CurrentTickCount);
    uint64_t TimeElapsedInClockTicks{(uint64_t)Win64CurrentTickCount.QuadPart - StartingClockTickCount};

    float MilliSecondsElapsed = (1000.0f * ((float)TimeElapsedInClockTicks / ClockTicksPerSecond));

    return MilliSecondsElapsed;
}
