#pragma once

#include <stdio.h>
#include <stdlib.h>
#include "boagz\error_context.h"

#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}

#if BGZ_LOGGING_ON
	#define BGZ_ASSERT(expression) \
		Assert(expression) 

	#define BGZ_CONSOLE(...) \
		do { fprintf_s(stderr, __VA_ARGS__); } while (0)

	#define InvalidCodePath Assert(!"InvalidCodePath");

#else
	#define BGZ_ASSERT(expression) __noop
	#define BGZ_CONSOLE(...) __noop
	#define InvalidCodePath __noop

#endif

#if BGZ_ERRHANDLING_ON
	#define BGZ_ERRCTXT1(errorDescription) \
		Bgz::ErrContext ec(errorDescription);

	#define BGZ_ERRCTXT2(errorDescription, errorData) \
		Bgz::ErrContext ec(errorDescription, errorData);

	#define BGZ_ERRASSERT(condition, msg, ...) \
		do { if (!(condition)) Bgz::ErrorReport(#condition, __func__, __FILE__, __LINE__, msg, __VA_ARGS__); } while (0)

#else
	#define BGZ_ERRCTXT1(errorDescription) __noop
	#define BGZ_ERRCTXT2(errorDescription, errorData) __noop
	#define BGZ_ERRASSERT(condition, msg, ...) __noop

#endif

namespace Bgz
{
	template<typename... ArgTypes>
	inline auto ErrorReport(const char* errCondition, const char* functionName, const char* file, int lineNumber, const char* errMessage, ArgTypes... args) -> void
	{
		ErrContext::LogContext();

		fprintf_s(stderr, "    Function: %s failed from file: %s on line number: %i\n", functionName, file, lineNumber);
		fprintf_s(stderr, "    Assertion failed: %s  ", errCondition);
		fprintf_s(stderr, errMessage, args...);

		//Have this instead of exit(0) so console window stays open
		int dummy = 3;
		scanf_s("%d", &dummy);
	}
}