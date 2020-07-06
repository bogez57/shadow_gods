
#include <stdio.h>
#include "intrinsics.h"
#include "debug.h"

TimedScopeInfo scopeInfoArray[];

void BeginTimer(Timer* timer, int counter, char* fileName, char* functionName, int lineNumber, int hitCountInit)
{
    timer->scopeInfo = scopeInfoArray + counter;
    
    timer->scopeInfo->fileName = fileName;
    timer->scopeInfo->functionName = functionName;
    timer->scopeInfo->lineNumber = lineNumber;
    timer->hitCount = hitCountInit;
    
    timer->startCycles_count = __rdtsc();
};

void EndTimer(Timer* timer)
{
    uint64_t endTime = __rdtsc();
    uint64_t delta = ((endTime - timer->startCycles_count) | ((uint64_t)timer->hitCount << 40));//So first 3 bytes are reserved for hit count number and last 5 bytes reserved for num of cycles elapsed
    
    ThreadSafeAdd(&timer->scopeInfo->hitCount_cyclesElapsed, delta);
    
    printf("fileName: %s\n", timer->scopeInfo->fileName);
    printf("functionName: %s\n", timer->scopeInfo->functionName);
    printf("line number: %i\n", timer->scopeInfo->lineNumber);
    printf("hit count: %llu\n", (unsigned long long)timer->scopeInfo->hitCount_cyclesElapsed >> 40);
    printf("cylces to complete: %llu\n\n", ((unsigned long long)timer->scopeInfo->hitCount_cyclesElapsed & 0x000000FFFFFFFFFF));
};
