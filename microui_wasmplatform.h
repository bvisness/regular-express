#define NULL 0
#define size_t unsigned long
#define stderr 0

// TODO: Actually do these things
int fprintf(int nope, const char* format, ...) { return 0; }
void * memset ( void * ptr, int value, size_t num ) { return NULL; }
void * memcpy ( void * destination, const void * source, size_t num) { return NULL; }
unsigned long strlen(const char * str) { return 0; }
int sprintf ( char * str, const char * format, ... ) { return 0; }
double strtod( const char* str, char** str_end ) { return 0; }

void swap(void *v[], int left, int right) {
    void* tmp = v[left];
    v[left] = v[right];
    v[right] = tmp;
}

void qsort(void *v[], int left, int right, int comp(const void *, const void *)) {
    int i, last;

    if (left >= right)
        return;
    swap(v, left, (left + right)/2);
    last = left;
    for (i = left+1; i <= right; i++)
        if ((*comp)(v[i], v[left]) < 0)
            swap(v, ++last, i);
    swap(v, left, last);
    qsort(v, left, last-1, comp);
    qsort(v, last+1, right, comp);
}

void abort() {};
