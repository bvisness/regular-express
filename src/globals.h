#pragma once

#include "microui.h"
#include "regex/regex.h"

#ifdef MAIN_IMPL
#define GLOBAL
#else
#define GLOBAL extern
#endif

#define PI 3.14159265358979323846
#define COLORPARAMS unsigned char r, unsigned char g, unsigned char b, unsigned char a

extern int measureText(const char* text, int len);

extern void canvas_clear();
extern void canvas_clip(int x, int y, int w, int h);
extern void canvas_setFillRGB();
extern void canvas_rect(int x, int y, int w, int h, int radius, COLORPARAMS);
extern void canvas_text(char* str, int x, int y, COLORPARAMS);
extern void canvas_line(int x1, int y1, int x2, int y2, COLORPARAMS, int strokeWidth);
extern void canvas_arc(int x, int y, int radius, float angleStart, float angleEnd, COLORPARAMS, int strokeWidth);

extern void setOutput(char* text);

#define UNION_VERTICAL_SPACING 4
#define UNION_GUTTER_WIDTH 30
#define NOUNIONEX_MIN_HEIGHT 20
#define UNIT_HANDLE_ZONE_WIDTH 16
#define HANDLE_SIZE 8
#define UNIT_WIRE_ATTACHMENT_ZONE_WIDTH 24
#define UNIT_REPEAT_WIRE_ZONE_HEIGHT 15
#define UNIT_REPEAT_WIRE_MARGIN 12
#define UNIT_REPEAT_WIRE_RADIUS 10
#define UNIT_REPEAT_WIRE_SCOOT 0
#define UNIT_CONTENTS_MIN_HEIGHT 20
#define UNIT_CONTENTS_LITCHAR_WIDTH 15
#define WIRE_THICKNESS 2
#define SET_PADDING 4
#define SET_HORIZONTAL_SPACING 2
#define SET_DASH_WIDTH 10
#define SET_ONEOF_TEXT "one of"
#define SET_ONEOF_TEXT_NEG "anything but"
#define SET_ONEOF_HEIGHT 22
#define GROUP_VERTICAL_PADDING 4
#define CURSOR_THICKNESS 2
#define CURSOR_VERTICAL_PADDING 2

#define COLOR_RE_TEXT (mu_Color) { 0, 0, 0, 255 }
#define COLOR_RE_TEXT_DIM (mu_Color) { 140, 140, 140, 255 }
#define COLOR_WIRE (mu_Color) { 50, 50, 50, 255 }
#define COLOR_CURSOR (mu_Color) { 45, 83, 252, 255 }
#define COLOR_UNIT_BACKGROUND (mu_Color) { 200, 200, 200, 255 }
#define COLOR_METACHAR_BACKGROUND (mu_Color) { 188, 170, 224, 255 }
#define COLOR_SPECIAL_BACKGROUND (mu_Color) { 170, 225, 170, 255 }
#define COLOR_SELECTED_BACKGROUND (mu_Color) { 122, 130, 255, 255 }
#define COLOR_UNKNOWN_CONSTRUCT_BACKGROUND (mu_Color) { 252, 159, 160, 255 }

#define LEGAL_METACHARS "dDwWsSbBfnrtv0"
#define UNKNOWN_CONSTRUCT_TEXT "???"

GLOBAL mu_Context* ctx;
GLOBAL NoUnionEx* lastFocusedEx;
