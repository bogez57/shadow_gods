#include "debug.h"
#include <stdio.h>

void BeginTimer(Timer* timer)
{
    timer->startTime = __rdtsc();
};

void EndTimer(Timer* timer)
{
    timer->endTime = __rdtsc();
    uint64_t cyclesElapsed = timer->endTime - timer->startTime;
    printf("cylces: %lu\n", (unsigned long)cyclesElapsed);
};