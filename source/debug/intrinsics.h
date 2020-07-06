#ifndef INTRINSICS_H
#define INTRINSICS_H

#include <intrin.h>
#include <stdint.h>

inline int64_t ThreadSafeAdd(volatile uint64_t* val, uint64_t addAmount)
{
    //This returns original value. Actual add result is from out param
    int64_t originalAmount = _InterlockedExchangeAdd64((__int64*)val, (__int64)addAmount);
    
    return originalAmount;
};

#endif //INTRINSICS_H
