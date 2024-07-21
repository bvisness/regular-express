#include "viz.h"

unsigned char _vizbuf[VIZBUF_SIZE];

unsigned char* vizbuf() {
    return _vizbuf;
}

size_t vizbuf_size() {
    return VIZBUF_SIZE;
}
