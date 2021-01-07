#ifndef FAKESTDLIB_STDLIB_H
#define FAKESTDLIB_STDLIB_H

#include <stddef.h>
#include <debug.h>

extern void abort();

void* malloc(size_t bytes);
void qsort(void* base, size_t nitems, size_t size, int (*compar)(const void *, const void*));

#ifdef NDEBUG
#define assert(condition) ((void)0)
#else
#define assert(condition) if (!(condition)) { printError(#condition); abort(); }
#endif

#endif
