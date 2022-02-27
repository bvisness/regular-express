#pragma once

#include <stddef.h>
#include <debug.h>

extern void abort();

void* malloc(size_t bytes);
void qsort(void* base, size_t nitems, size_t size, int (*compar)(const void *, const void*));
int isdigit(int c);
int atoi(const char *s);
int isspace(int c);

#ifdef NDEBUG
#define assert(condition) ((void)0)
#else
#define assert(condition) if (!(condition)) { printError(#condition); abort(); }
#endif
