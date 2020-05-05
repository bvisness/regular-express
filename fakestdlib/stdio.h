#ifndef FAKESTDLIB_STDIO_H
#define FAKESTDLIB_STDIO_H

#include <stb_sprintf.h>

#include <debug.h>

#define stdout 0
#define stderr 1

#define sprintf stbsp_sprintf
int printf(const char* format, ...);
int fprintf(int nope, const char* format, ...);
double strtod(const char* str, char** endptr);

#endif
