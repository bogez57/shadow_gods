
#include <stdio.h>
#include <assert.h>
#include "intrinsics.h"
#include "debug.h"

TimedScopeInfo* translationUnitScopeArrays[2];
int translationUnitScopeArrayCount = 0;

//#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

void BeginTimer(Timer* timer, int translationUnitIndex, int counter, char* fileName, char* functionName, int lineNumber, int hitCountInit)
{
    TimedScopeInfo* timedScopesArray = translationUnitScopeArrays[translationUnitIndex];
    timer->scopeInfo = timedScopesArray  + counter;
    
    timer->scopeInfo->fileName = fileName;
    timer->scopeInfo->functionName = functionName;
    timer->scopeInfo->lineNumber = lineNumber;
    timer->hitCountInit = hitCountInit;
    
    timer->initialCycleCount = __rdtsc();
};

void EndTimer(Timer* timer)
{
    uint64_t endTime = __rdtsc();
    uint64_t delta = ((endTime - timer->initialCycleCount) | ((uint64_t)timer->hitCountInit << 40));//So first 3 bytes are reserved for hit count number and last 5 bytes reserved for num of cycles elapsed
    
    ThreadSafeAdd(&timer->scopeInfo->hitCount_cyclesElapsed, delta);
    
#if 0
    printf("fileName: %s\n", timer->scopeInfo->fileName);
    printf("functionName: %s\n", timer->scopeInfo->functionName);
    printf("line number: %i\n", timer->scopeInfo->lineNumber);
    printf("hit count: %llu\n", (unsigned long long)timer->scopeInfo->hitCount_cyclesElapsed >> 40);
    printf("cylces to complete: %llu\n\n", ((unsigned long long)timer->scopeInfo->hitCount_cyclesElapsed & 0x000000FFFFFFFFFF));
#endif
};

void AddTranslationUnitTimedScopesArray(TimedScopeInfo* timedScopesArray)
{
    assert(translationUnitScopeArrayCount < 2);
    translationUnitScopeArrays[translationUnitScopeArrayCount++] = timedScopesArray;
};