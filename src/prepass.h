#pragma once

#include "drag.h"
#include "globals.h"

#include "regex/alloc.h"
#include "regex/parser.h"
#include "regex/regex.h"

#include "util/math.h"

void prepass_Regex(Regex* regex, NoUnionEx* parentEx);
void prepass_NoUnionEx(NoUnionEx* ex, Regex* regex, NoUnionEx* parentEx);
void prepass_Unit(Unit* unit, NoUnionEx* ex);
void prepass_UnitContents(UnitContents* contents, NoUnionEx* ex);
void prepass_Set(Set* set, NoUnionEx* ex);
void prepass_Group(Group* group, NoUnionEx* ex);
