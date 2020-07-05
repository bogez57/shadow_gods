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
    uint64_t cpuCyclesElapsed{};
    char* fileName{};
    char* functionName{};
    int lineNumber{};
    int hitCount{};
};

struct Timer
{
    uint64_t startTime{};
    uint64_t endTime{};
    TimedScopeInfo* scopeInfo{};
};

void BeginTimer(Timer* timer, int counter, char* fileName, char* functionName, int lineNumber, int hitCount = 1);
void EndTimer(Timer* timer);

//This macro stuff makes it so you can define multiple TIME_SCOPEs in a block. Also has built in compile check so you can't declare 2 TIME_SCOPEs on same line
#define TIMED_SCOPE__(number, ...) Timer timer_##number{}; BeginTimer(&timer_##number, __COUNTER__, __FILE__, __FUNCTION__, __LINE__, ## __VA_ARGS__); defer { EndTimer(&timer_##number); }
#define TIMED_SCOPE_(number, ...) TIMED_SCOPE__(number, ## __VA_ARGS__)
#define TIMED_SCOPE(...) TIMED_SCOPE_(__LINE__, ## __VA_ARGS__)

#endif