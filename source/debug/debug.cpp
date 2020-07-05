#include "debug.h"
#include <stdio.h>

TimedScopeInfo scopeInfoArray[];

void BeginTimer(Timer* timer, int counter, char* fileName, char* functionName, int lineNumber, int hitCount)
{
    timer->startTime = __rdtsc();
    
    timer->scopeInfo = scopeInfoArray + counter;
    timer->scopeInfo->fileName = fileName;
    timer->scopeInfo->functionName = functionName;
    timer->scopeInfo->lineNumber = lineNumber;
    timer->scopeInfo->hitCount += hitCount;
};

void EndTimer(Timer* timer)
{
    timer->endTime = __rdtsc();
    
    timer->scopeInfo->cpuCyclesElapsed = timer->endTime - timer->startTime;
    
    printf("cylces: %lu\n", (unsigned long)timer->scopeInfo->cpuCyclesElapsed);
    printf("fileName: %s\n", timer->scopeInfo->fileName);
    printf("functionName: %s\n", timer->scopeInfo->functionName);
    printf("line number: %i\n", timer->scopeInfo->lineNumber);
};