#pragma once

#include "drag.h"
#include "globals.h"
#include "range.h"

#include "regex/alloc.h"
#include "regex/regex.h"

#include "util/math.h"

void drawRailroad_Regex(Regex* regex, Vec2i origin, int unitDepth);
void drawRailroad_NoUnionEx(NoUnionEx* ex, Vec2i origin, int unitDepth);
void drawRailroad_Unit(Unit* unit, NoUnionEx* parent, Vec2i origin, int depth, UnitRange* selection);
void drawRailroad_UnitContents(UnitContents* contents, Vec2i origin, int unitDepth, int selected);
void drawRailroad_Set(Set* set, Vec2i origin);
void drawRailroad_Group(Group* group, Vec2i origin, int unitDepth, int selected);

void draw_arbitrary_text(mu_Context* ctx, const char* str, mu_Vec2 pos, mu_Color color);
void drawConnector(int startX, int startY, int inx, int dy1, int dx, int dy2, int outx);
void drawVerticalConnector(int startX, int startY, int inx, int dy, int outx);
