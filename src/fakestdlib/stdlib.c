#include <stdlib.h>

extern void* __heap_base;

void* nextAddress;
void* malloc(size_t bytes) {
	if (!nextAddress) {
		nextAddress = &__heap_base;
	}

	void* address = nextAddress;
	nextAddress += bytes;
	return address;
}

/* Byte-wise swap two items of size SIZE. Thanks, glibc! */
#define SWAP(a, b, size)                   \
  do                                       \
    {                                      \
      size_t __size = (size);              \
      char *__a = (a), *__b = (b);         \
      do                                   \
        {                                  \
          char __tmp = *__a;               \
          *__a++ = *__b;                   \
          *__b++ = __tmp;                  \
        } while (--__size > 0);            \
    } while (0)

void qsort(void* base, size_t nitems, size_t size, int (*compar)(const void *, const void*)) {
	// Hey guess what, I'm lazy and this is an insertion sort
	for (int i = 0; i < nitems; i++) {
		for (int j = i; j >= 1; j--) {
			void* ptrA = base+(j*size)-size;
			void* ptrB = base+(j*size);

			int cmp = compar(ptrA, ptrB);
			if (cmp > 0) {
				SWAP(ptrA, ptrB, size);
			} else {
				break;
			}
		}
	}
}

int isspace(int c)
{
	return (
		c == ' '
		|| c == '\n'
		|| c == '\t'
		|| c == '\r'
		|| c == '\v'
	);
}

// from musl
int isdigit(int c)
{
	return (unsigned)c-'0' < 10;
}

// from musl
int atoi(const char *s)
{
	int n=0, neg=0;
	while (isspace(*s)) s++;
	switch (*s) {
	case '-': neg=1;
	case '+': s++;
	}
	/* Compute n as a negative number to avoid overflow on INT_MIN */
	while (isdigit(*s))
		n = 10*n - (*s++ - '0');
	return neg ? n : -n;
}
