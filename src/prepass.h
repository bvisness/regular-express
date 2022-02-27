#pragma once

#include "drag.h"
#include "globals.h"

#include "regex/alloc.h"
#include "regex/parser.h"
#include "regex/regex.h"

#include "util/math.h"

typedef struct {
    int GroupNumber;
} PrepassContext;

void prepass_Regex(PrepassContext* pctx, Regex* regex, NoUnionEx* parentEx, Unit* parentUnit);
void prepass_NoUnionEx(PrepassContext* pctx, NoUnionEx* ex, Regex* regex, NoUnionEx* parentEx, Unit* parentUnit);
void prepass_Unit(PrepassContext* pctx, Unit* unit, NoUnionEx* ex);
void prepass_UnitContents(PrepassContext* pctx, UnitContents* contents, NoUnionEx* ex, Unit* unit);
void prepass_Set(PrepassContext* pctx, Set* set, NoUnionEx* ex, Unit* unit);
void prepass_Group(PrepassContext* pctx, Group* group, NoUnionEx* ex, Unit* unit);
