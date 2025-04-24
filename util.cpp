#include "util.h"

#include <cstdio>
#include <cstring>
#include <memory>
#include <stdarg.h>

std::string format(const char *fmt, ...){
	va_list args;

	char buffer[4096];
	memset(buffer, 0, 4096);

	va_start(args, fmt);

	vsnprintf(buffer, sizeof(buffer), fmt, args);

	va_end(args);

	return std::string(buffer);
}

