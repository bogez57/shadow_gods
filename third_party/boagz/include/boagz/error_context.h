#pragma once

namespace Bgz
{
	class ErrContext
	{
	public:
		ErrContext(const char* ErrorContextDescription, const char* ErrorContexData = "");
		~ErrContext();
		ErrContext(const ErrContext& copy) = delete;
		void operator=(const ErrContext& copy) = delete;

		static auto LogContext() -> void;
	};
}
