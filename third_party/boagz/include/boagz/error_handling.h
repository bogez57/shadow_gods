#pragma once

#include <stdio.h>
#include <assert.h>

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

#define InvalidCodePath assert(!"InvalidCodePath")
#define InvalidDefaultCase default: {InvalidCodePath;} break
#define BGZ_ASSERT(condition, msg, ...)                                                   \
do                                                                                    \
{                                                                                     \
if (!(condition))                                                                 \
Bgz::ErrorReport(#condition, __func__, __FILE__, __LINE__, msg, __VA_ARGS__); \
} while (0)

#endif

namespace Bgz
{
    template <typename... ArgTypes>
        __forceinline void ErrorReport(const char* errCondition, const char* functionName, const char* file, int lineNumber, const char* errMessage, ArgTypes... args)
    {
        fprintf_s(stderr, "    Function: %s failed from file: %s on line number: %i\n", functionName, file, lineNumber);
        fprintf_s(stderr, "    Assertion failed: %s  ", errCondition);
        fprintf_s(stderr, errMessage, args...);
        
#if _MSC_VER
        __debugbreak();
#endif
        
#if 0
        //Have this instead of exit(0) so console window stays open
        int dummy = 3;
        scanf_s("%d", &dummy);
#endif
    }
} // namespace Bgz