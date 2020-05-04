#ifndef FAKESTDLIB_STDIO_H
#define FAKESTDLIB_STDIO_H

#include <stdarg.h>
#include <stdlib.h>
#include <stb_sprintf.h>

#include <debug.h>

#define sprintf stbsp_sprintf
int fprintf(int nope, const char* format, ...);
double strtod(const char* str, char** endptr);

#endif
