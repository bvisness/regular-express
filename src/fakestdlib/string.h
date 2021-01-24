#pragma once

#include <stdlib.h>

extern void* memset(void* ptr, int value, size_t num);
extern void* memcpy(void *dest, const void * src, size_t n);
int memcmp(const void* lhs, const void* rhs, size_t count);

unsigned long strlen(const char* str);
char* strchr(const char* str, int ch);
