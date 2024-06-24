#pragma once

#ifdef WD_PLATFORM_WINDOWS
	#ifdef WD_BUILD_DLL
		#define WALDEM_API __declspec(dllexport)
	#else
		#define WALDEM_API __declspec(dllimport)
	#endif
#else
	#error Waldem only support Windows!
#endif

#ifdef WD_DEBUG
	#define WD_ENABLE_ASSERTS
#endif

#ifdef WD_ENABLE_ASSERTS
	#define WD_ASSERT(x, ...) { if(!(x)) { WD_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
	#define WD_CORE_ASSERT(x, ...) { if(!(x)) { WD_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
#else
	#define WD_ASSERT(x, ...)
	#define WD_CORE_ASSERT(x, ...)
#endif

#define BIT(x) (1<<(x))

#define WD_BIND_EVENT_FN(fn) std::bind(&fn, this, std::placeholders::_1)