#pragma once

#ifdef _DEBUG
	#define ENABLE_ASSERTIONS 1
	#define ENABLE_WARNINGS 1
	#define ENABLE_LOGGING 1
#else // RELEASE
	#define ENABLE_ASSERTIONS 1
	#define ENABLE_WARNINGS 1
	#define ENABLE_LOGGING 0
#endif