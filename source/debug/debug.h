
#ifndef DEBUG_INCLUDE
#define DEBUG_INCLUDE

struct TimerStruct
{
    int num{4};
    
    TimerStruct();
    ~TimerStruct();
};


#define TIMED_BLOCK TimerStruct timer{};

#endif