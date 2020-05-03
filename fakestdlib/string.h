#include <stdlib.h>

#ifndef FAKESTDLIB_STRING_H
#define FAKESTDLIB_STRING_H

extern void* memset(void* ptr, int value, size_t num);
extern void *memcpy(void *dest, const void * src, size_t n);

static inline unsigned long strlen(const char* str) {
	long len = 0;
	while (1) {
		if (str[len] == 0) {
			break;
		}
		len++;
	}

	return len;
}

#endif
