#include "debug.h"
#include <stdio.h>

TimerStruct::TimerStruct()
{
    printf("Constructor running! %i\n", num);
};

TimerStruct::~TimerStruct()
{
    printf("Destructor running!");
};

extern "C" void TestFunc()
{
    printf("This is a func");
};
