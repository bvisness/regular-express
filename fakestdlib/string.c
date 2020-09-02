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
