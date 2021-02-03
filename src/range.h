#pragma once

#include "regex/alloc.h"
#include "regex/regex.h"

typedef struct UnitRange {
    NoUnionEx* Ex;
    int Start;
    int End;
} UnitRange;

// TODO: UnitRange Helper
// Make functions for getting the first and last units, for convenience.

void MoveUnitsTo(UnitRange range, NoUnionEx* ex, int startIndex);
Unit* ConvertRangeToGroup(UnitRange range);
void DeleteRange(UnitRange range);
