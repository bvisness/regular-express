#ifndef VEC_H
#define VEC_H

typedef struct Vec2i {
    union {
        int x;
        int w;
    };
    union {
        int y;
        int h;
    };
} Vec2i;

#endif
