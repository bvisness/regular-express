#include <stdlib.h>

extern void* __heap_base;

void* nextAddress;
void* malloc(size_t bytes) {
	if (!nextAddress) {
		nextAddress = &__heap_base;
	}

	void* address = nextAddress;
	nextAddress += bytes;
	printString("Malloc Results (result, next)");
	printInt(address);
	printInt(nextAddress);
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
