
#include <stdio.h>
#include "boagz/error_context.h"

namespace Bgz
{
	thread_local const char *errorContextDescriptions[MAX_CONTEXTS];
	thread_local const char *errorContextData[MAX_CONTEXTS];
	thread_local unsigned int numContexts = 0;

	ErrContext::ErrContext(const char *c_ErrorContextDescription, const char *c_ErrorContexData /* default value = "" */)
	{
		errorContextDescriptions[numContexts] = c_ErrorContextDescription;
		errorContextData[numContexts] = c_ErrorContexData;
		++numContexts;
	}

	ErrContext::~ErrContext()
	{
		--numContexts;
	}

	auto ErrContext::LogContext() -> void
	{
		fprintf_s(stderr, "Number of contexts %i", MAX_CONTEXTS);
		if (numContexts > MAX_CONTEXTS)
		{
			fprintf_s(stderr, "Too many contexts!!!!!!");
			int dummy = 3;
			scanf_s("%d", &dummy);
		};

		for (unsigned int i = 0; i < numContexts; ++i)
		{
			//Add extra space to separate context reporting from whatever log messages came before it
			if (i == 0)
				fprintf_s(stderr, "\n\n");

			if (errorContextDescriptions[i] == nullptr)
				fprintf_s(stderr, "ERROR with an error context description string being null!!!");
			if (errorContextData[i] == nullptr)
				fprintf_s(stderr, "ERROR with error context data being null!!!");

			fprintf_s(stderr, "  %s: %s\n", errorContextDescriptions[i], errorContextData[i]);
		}
	}
}