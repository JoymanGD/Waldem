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

#ifdef WD_ENABLE_ASSERTS

	#define WD_ASSERT(...) { if(!(x)) { WD_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
	#define WD_CORE_ASSERT(...) { if(!(x)) { WD_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
#else
	#define WD_ASSERT(x, ...)
	#define WD_CORE_ASSERT(x, ...)
#endif

#define BIT(x) (1<<(x))