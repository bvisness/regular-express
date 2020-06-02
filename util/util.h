#ifndef REGEX_INTERP_H
#define REGEX_INTERP_H

static inline int iabs(int a) {
    return (a < 0) ? -a : a;
}

static inline int isign(int a) {
    return (a < 0) ? -1 : 1;
}

static inline int imin(int a, int b) {
    return a < b ? a : b;
}

static inline int imax(int a, int b) {
    return a > b ? a : b;
}

static inline int iclamp(int x, int min, int max) {
    return imin(max, imax(min, x));
}

static inline float fabs(float a) {
    return (a < 0) ? -a : a;
}

static inline float fsign(float a) {
    return (a < 0) ? -1 : 1;
}

static inline float fmin(float a, float b) {
    return a < b ? a : b;
}

static inline float fmax(float a, float b) {
    return a > b ? a : b;
}

static inline float fclamp(float x, float min, float max) {
    return fmin(max, fmax(min, x));
}

static inline float interp_linear(float dt, float current, float target, float rate, int* changed) {
    float result = current;

    if (current > target) {
        result = fmax(target, current - rate * dt);
    } else if (current < target) {
        result =  fmin(target, current + rate * dt);
    }

    if (changed != NULL && result != current) {
        *changed = 1;
    }

    return result;
}

#endif
