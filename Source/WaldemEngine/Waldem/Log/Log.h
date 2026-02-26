#pragma once

#include "Waldem/Core.h"
#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"

namespace Waldem
{
	class WALDEM_API Log
	{
	public:
		static void Init();
		inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return CoreLogger; }
		inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return ClientLogger; }

	private:
		inline static std::shared_ptr<spdlog::logger> CoreLogger;
		inline static std::shared_ptr<spdlog::logger> ClientLogger;
	};
}

//Core log macros
#define WD_CORE_TRACE(...)		::Waldem::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define WD_CORE_INFO(...)		::Waldem::Log::GetCoreLogger()->info(__VA_ARGS__)
#define WD_CORE_WARN(...)		::Waldem::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define WD_CORE_ERROR(...)		::Waldem::Log::GetCoreLogger()->error(__VA_ARGS__)
#define WD_CORE_FATAL(...)		::Waldem::Log::GetCoreLogger()->fatal(__VA_ARGS__)

//Client log macros
#define WD_TRACE(...)			::Waldem::Log::GetClientLogger()->trace(__VA_ARGS__)
#define WD_INFO(...)			::Waldem::Log::GetClientLogger()->info(__VA_ARGS__)
#define WD_WARN(...)			::Waldem::Log::GetClientLogger()->warn(__VA_ARGS__)
#define WD_ERROR(...)			::Waldem::Log::GetClientLogger()->error(__VA_ARGS__)
#define WD_FATAL(...)			::Waldem::Log::GetClientLogger()->fatal(__VA_ARGS__)