
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "intrinsics.h"
#include "debug.h"

void BeginTimer(Timer* timer, char* scopeName, char* fileName, char* functionName, int lineNumber, int hitCountInit)
{
    timer->debugEvent = currentFrameDebugEventArray + numEvents;
    ThreadSafeAdd(&numEvents, 1);
    
    timer->debugEvent->scopeName = scopeName;
    timer->debugEvent->fileName = fileName;
    timer->debugEvent->functionName = functionName;
    timer->debugEvent->lineNumber = lineNumber;
    timer->debugEvent->startCycles = __rdtsc();
};

void EndTimer(Timer* timer)
{
    timer->debugEvent->endCycles = __rdtsc();
    
#if 0
    printf("fileName: %s\n", timer->debugEvent->fileName);
    printf("functionName: %s\n", timer->debugEvent->functionName);
    printf("line number: %i\n", timer->debugEvent->lineNumber);
    printf("hit count: %llu\n", (unsigned long long)timer->debugEvent->hitCount_cyclesElapsed >> 40);
    printf("cylces to complete: %llu\n\n", ((unsigned long long)timer->debugEvent->hitCount_cyclesElapsed & 0x000000FFFFFFFFFF));
#endif
};

void UpdateDebugState(DebugState* debugState)
{
    if(debugState->allStoredDebugEvents[0].timeStampCount < 100)
    {
        for(int i{}; i < debugState->numStoredDebugEvents; ++i)
        {
            for(int j{}; j < numEvents; ++j)
            {
                DebugEvent* debugEvent = currentFrameDebugEventArray + j;
                
                if(debugState->allStoredDebugEvents[i].scopeName == debugEvent->scopeName)
                {
                    debugState->allStoredDebugEvents[i].fileName = debugEvent->fileName;
                    debugState->allStoredDebugEvents[i].functionName = debugEvent->functionName;
                    debugState->allStoredDebugEvents[i].lineNumber = debugEvent->lineNumber;
                    
                    DebugTimeStamp* timeStamp = &debugState->allStoredDebugEvents[i].timeStamps[debugState->allStoredDebugEvents[i].timeStampCount];
                    timeStamp->cycleCount += debugEvent->endCycles - debugEvent->startCycles;
                    timeStamp->hitCount += 1;
                };
                
                if(j == (numEvents - 1))//Last iteration of loop
                    ++debugState->allStoredDebugEvents[i].timeStampCount;
            }
        };
    }
    else
    {
        //Max time stamps have been captured so begin new timestamp capture cycle
        for(int i{}; i < debugState->numStoredDebugEvents; ++i)
        {
            memset(debugState->allStoredDebugEvents[i].timeStamps, 0, (debugState->allStoredDebugEvents[i].timeStampCount * sizeof(DebugTimeStamp)));
            debugState->allStoredDebugEvents[i].timeStampCount = 0;
        };
    };
};

void EndOfFrame_ResetTimingInfo()
{
    for(int i{}; i < numEvents; ++i)
    {
        DebugEvent* debugEvent = currentFrameDebugEventArray + i;
    };
    
    numEvents = 0;
};

void InitDebugState(DebugState* debugState)
{
    bool noMatchingDebugEvents{true};
    for(int i{}; i < numEvents; ++i)
    {
        for(int j{}; j < numEvents; ++j)
        {
            if(debugState->allStoredDebugEvents[j].scopeName != currentFrameDebugEventArray[i].scopeName)
            {
                //continue
                noMatchingDebugEvents = true;
            }
            else
            {
                noMatchingDebugEvents = false;
                break;
            };
        };
        
        if(noMatchingDebugEvents)
        {
            DebugEvent* debugEvent = currentFrameDebugEventArray + i;
            
            debugState->allStoredDebugEvents[debugState->numStoredDebugEvents].scopeName = debugEvent->scopeName;
            debugState->allStoredDebugEvents[debugState->numStoredDebugEvents].fileName = debugEvent->fileName;
            debugState->allStoredDebugEvents[debugState->numStoredDebugEvents].functionName = debugEvent->functionName;
            debugState->allStoredDebugEvents[debugState->numStoredDebugEvents].lineNumber = debugEvent->lineNumber;
            ++debugState->numStoredDebugEvents;
        };
    };
};
