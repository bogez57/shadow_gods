#ifndef DEBUG_INCLUDE
#define DEBUG_INCLUDE

#include <stdint.h>
#include <intrin.h>
#include <utility>

template <typename F>
struct Defer {
    Defer( F f ) : f( f ) {}
    ~Defer( ) { f( ); }
    F f;
};

template <typename F>
Defer<F> makeDefer( F f ) {
    return Defer<F>( f );
};

#define __defer( line ) defer_ ## line
#define _defer( line ) __defer( line )

struct defer_dummy { };
template<typename F>
Defer<F> operator+( defer_dummy, F&& f )
{
    return makeDefer<F>( std::forward<F>( f ) );
}

#define defer auto _defer( __LINE__ ) = defer_dummy( ) + [ & ]( )

struct TimedScopeInfo
{
    char* fileName{};
    char* functionName{};
    int lineNumber{};
    
    //In implementation, the first 3 bytes are reserved for hit count numbers and the last 5 bytes are for number of cycles elapsed.
    //Have these numbers or'd together here for more guaranteed thread saftey since hitCount and cyclesElapsed are both calcualted within the same
    //atomic add function.
    uint64_t hitCount_cyclesElapsed{};
};

struct Timer
{
    uint64_t initialCycleCount{};
    uint32_t hitCountInit{};
    TimedScopeInfo* scopeInfo{};
};

struct DebugTimeStamp
{
    uint64_t cycleCount{};
    uint64_t hitCount{};
};

struct TimedScope
{
    char* fileName{};
    char* functionName{};
    int lineNumber{};
    
    int timeStampCount{100};
    DebugTimeStamp timeStamps[100];
};

struct DebugState
{
    int timedScopeCount{};
    TimedScope timedScopesInCode[200];
};

#define MAX_DEBUG_EVENT_COUNT 65533
uint64_t eventArrIndex;
uint64_t numEvents;
TimedScopeInfo debugEventArray[MAX_DEBUG_EVENT_COUNT];

__declspec(dllexport) void BeginTimer(Timer* timer, int counter, char* fileName, char* functionName, int lineNumber, int hitCountInit = 1);
__declspec(dllexport) void EndTimer(Timer* timer);
__declspec(dllexport) void EndOfFrame_ResetTimingInfo();
__declspec(dllexport) void UpdateDebugState(DebugState* debugState);
__declspec(dllexport) void EndOfFrame_ResetTimingInfo();
__declspec(dllexport) void InitDebugState(DebugState* debugState);

//This macro stuff makes it so you can define multiple TIME_SCOPEs in a block. Also has built in compile check so you can't declare 2 TIME_SCOPEs on same line
#define TIMED_SCOPE__(number, ...) Timer timer_##number{}; BeginTimer(&timer_##number,__COUNTER__, __FILE__, __FUNCTION__, __LINE__, ## __VA_ARGS__); defer { EndTimer(&timer_##number); }
#define TIMED_SCOPE_(number, ...) TIMED_SCOPE__(number, ## __VA_ARGS__)
#define TIMED_SCOPE(...) TIMED_SCOPE_(__LINE__, ## __VA_ARGS__)

#endif