#ifndef REGEX_INTERP_H
#define REGEX_INTERP_H

#include "../microui.h"

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

    if (changed != 0 && result != current) {
        *changed = 1;
    }

    return result;
}

static inline mu_Rect rect_union(mu_Rect r1, mu_Rect r2) {
    return mu_rect(
        imin(r1.x, r2.x),
        imin(r1.y, r2.y),
        imax(r1.x + r1.w, r2.x + r2.w) - imin(r1.x, r2.x),
        imax(r1.y + r1.h, r2.y + r2.h) - imin(r1.y, r2.y)
    );
}

static inline mu_Rect rect_intersect(mu_Rect r1, mu_Rect r2) {
    int x1 = imax(r1.x, r2.x);
    int y1 = imax(r1.y, r2.y);
    int x2 = imin(r1.x + r1.w, r2.x + r2.w);
    int y2 = imin(r1.y + r1.h, r2.y + r2.h);
    if (x2 < x1) { x2 = x1; }
    if (y2 < y1) { y2 = y1; }
    return mu_rect(x1, y1, x2 - x1, y2 - y1);
}

static inline int rect_contains(mu_Rect r1, mu_Rect r2) {
    mu_Rect intersection = rect_intersect(r1, r2);
    return (
        intersection.x == r2.x
        && intersection.y == r2.y
        && intersection.w == r2.w
        && intersection.h == r2.h
    );
}

static inline int rect_overlaps(mu_Rect r1, mu_Rect r2) {
    mu_Rect intersection = rect_intersect(r1, r2);
    return (intersection.w && intersection.h);
}

#endif
