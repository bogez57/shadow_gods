
#include <stdio.h>
#include <assert.h>
#include "intrinsics.h"
#include "debug.h"

void BeginTimer(Timer* timer, int counter, char* fileName, char* functionName, int lineNumber, int hitCountInit)
{
    timer->scopeInfo = debugEventArray + numEvents;
    ThreadSafeAdd(&numEvents, 1);
    
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

void UpdateDebugState(DebugState* debugState)
{
    debugState->timedScopeCount = (int)numEvents;
    
    if(debugState->timedScopesInCode[0].timeStampCount < 100)
    {
        for(int i{}; i < debugState->timedScopeCount; ++i)
        {
            TimedScopeInfo* scopeInfo = debugEventArray + i;
            
            debugState->timedScopesInCode[i].fileName = scopeInfo->fileName;
            debugState->timedScopesInCode[i].functionName = scopeInfo->functionName;
            debugState->timedScopesInCode[i].lineNumber = scopeInfo->lineNumber;
            
            DebugTimeStamp* timeStamp = &debugState->timedScopesInCode[i].timeStamps[debugState->timedScopesInCode[i].timeStampCount];
            timeStamp->cycleCount = scopeInfo->hitCount_cyclesElapsed & 0x000000FFFFFFFFFF;
            timeStamp->hitCount = scopeInfo->hitCount_cyclesElapsed >> 40;
            
            ++debugState->timedScopesInCode[i].timeStampCount;
        };
    }
    else
    {
        //Max time stamps have been captured so begin new timestamp capture cycle
        for(int i{}; i < debugState->timedScopeCount; ++i)
        {
            debugState->timedScopesInCode[i].timeStampCount = 0;
        };
    };
};

void EndOfFrame_ResetTimingInfo()
{
    for(int i{}; i < numEvents; ++i)
    {
        TimedScopeInfo* scopeInfo = debugEventArray + i;
        scopeInfo->hitCount_cyclesElapsed = 0;//This is the only thing we have to reset currently as everything else just gets overwritten every frame
    };
    
    numEvents = 0;
};

void InitDebugState(DebugState* debugState)
{
    for(int i{}; i < numEvents; ++i)
    {
        TimedScopeInfo* scopeInfo = debugEventArray + i;
        
        debugState->timedScopesInCode[i].fileName = scopeInfo->fileName;
        debugState->timedScopesInCode[i].functionName = scopeInfo->functionName;
        debugState->timedScopesInCode[i].lineNumber = scopeInfo->lineNumber;
        ++debugState->timedScopeCount;
    };
};
