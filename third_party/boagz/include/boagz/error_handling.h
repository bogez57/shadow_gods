#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
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

	void UseDefaultOSConsole()
	{
		//Create a console for this application
		AllocConsole();

		// Get STDOUT handle
		HANDLE ConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
		int SystemOutput = _open_osfhandle(intptr_t(ConsoleOutput), _O_TEXT);
		FILE *COutputHandle = _fdopen(SystemOutput, "w");

		// Get STDERR handle
		HANDLE ConsoleError = GetStdHandle(STD_ERROR_HANDLE);
		int SystemError = _open_osfhandle(intptr_t(ConsoleError), _O_TEXT);
		FILE *CErrorHandle = _fdopen(SystemError, "w");

		// Get STDIN handle
		HANDLE ConsoleInput = GetStdHandle(STD_INPUT_HANDLE);
		int SystemInput = _open_osfhandle(intptr_t(ConsoleInput), _O_TEXT);
		FILE *CInputHandle = _fdopen(SystemInput, "r");

		// Redirect the CRT standard input, output, and error handles to the console
		freopen_s(&CInputHandle, "CONIN$", "r", stdin);
		freopen_s(&COutputHandle, "CONOUT$", "w", stdout);
		freopen_s(&CErrorHandle, "CONOUT$", "w", stderr);
	};
}