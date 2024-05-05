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

#define BIT(x) (1<<(x))