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
