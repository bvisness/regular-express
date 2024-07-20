#include "viz.h"

unsigned char _vizbuf[VIZBUF_SIZE];

unsigned char* vizbuf() {
    return _vizbuf;
}
