#include <string.h>

unsigned long strlen(const char* str) {
	long len = 0;
	while (1) {
		if (str[len] == 0) {
			break;
		}
		len++;
	}

	return len;
}

char* strchr(const char* str, int ch) {
    char* i = (char*) str;
    while (*i) {
        if (*i == ch) {
            return i;
        }
        i++;
    }

    return NULL;
}

int memcmp(const void* lhs, const void* rhs, size_t count) {
    for (size_t i = 0; i < count; i++) {
        unsigned char lbyte = *((unsigned char*)lhs + i);
        unsigned char rbyte = *((unsigned char*)rhs + i);
        if (lbyte < rbyte) return -1;
        if (lbyte > rbyte) return 1;
    }

    return 0;
}
