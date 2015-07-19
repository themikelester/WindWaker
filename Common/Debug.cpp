#include "Debug.h"
#include <stdio.h>
#include <stdarg.h>
#include <iostream>

#pragma warning( once : 4385 )

char _DEBUG_BUFFER[256];
void warn(const char* msg, ...)
{
	va_list argList;
	va_start(argList, msg);
	char buff[161];
	vsnprintf_s(buff, 161, msg, argList);
	va_end(argList);
	std::cout << buff << std::endl;
}