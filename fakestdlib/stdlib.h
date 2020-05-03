#ifndef FAKESTDLIB_STDLIB_H
#define FAKESTDLIB_STDLIB_H

#include <stddef.h>

#define stderr 0

extern void abort();
extern void *memcpy(void *dest, const void * src, size_t n);

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

static inline void qsort(void* base, size_t nitems, size_t size, int (*compar)(const void *, const void*)) {
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

#endif
