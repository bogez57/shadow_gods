#ifndef INTRINSICS_H
#define INTRINSICS_H

#include <intrin.h>
#include <stdint.h>

inline unsigned int ThreadSafeAdd(uint64_t volatile* val, uint64_t addAmount)
{
    //This returns original value. Actual add result is from out param
    unsigned int result = _InterlockedExchangeAdd((long*)val, (long)addAmount);
    
    return result;
};

#endif //INTRINSICS_H
