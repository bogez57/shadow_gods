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

struct Timer
{
    uint64_t startTime{};
    uint64_t endTime{};
};

void BeginTimer(Timer* timer);
void EndTimer(Timer* timer);

#define TIMED_BLOCK Timer timer{}; BeginTimer(&timer); defer { EndTimer(&timer); }

#endif