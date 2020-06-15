#pragma once

#include <stdio.h>

#if BGZ_LOGGING_ON

#define BGZ_CONSOLE(...)                \
do                                  \
{                                   \
fprintf_s(stderr, __VA_ARGS__); \
} while (0)
#else
#define BGZ_CONSOLE(...) __noop

#endif

#if BGZ_ERRHANDLING_ON

#define InvalidCodePath BGZ_ASSERT(1 == 0, "")
#define InvalidDefaultCase default: {InvalidCodePath;} break
#define BGZ_ASSERT(condition, msg) ((condition) || (__debugbreak(), 0))

#endif