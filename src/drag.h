#pragma once

#include "globals.h"
#include "range.h"

#include "regex/regex.h"

enum {
    DRAG_WIRE_LEFT_HANDLE,
    DRAG_WIRE_RIGHT_HANDLE
};

enum {
    DRAG_TYPE_NONE,
    DRAG_TYPE_WIRE,
    DRAG_TYPE_BOX_SELECT,
    DRAG_TYPE_MOVE_UNITS,
    DRAG_TYPE_CREATE_UNION,
};

typedef struct DragWire {
    Unit* OriginUnit;
    Unit* UnitBeforeHandle;
    Unit* UnitAfterHandle;

    Vec2i WireStartPos;
    int WhichHandle;
} DragWire;

typedef struct PotentialSelect {
    int Valid;
    UnitRange Range;
} PotentialSelect;

#define MAX_POTENTIAL_SELECTS 16

typedef struct DragBoxSelect {
    Vec2i StartPos;
    mu_Rect Rect;

    PotentialSelect Potentials[MAX_POTENTIAL_SELECTS];
    UnitRange Result;
} DragBoxSelect;

typedef struct DragMoveUnits {
    NoUnionEx* OriginEx;
    int OriginIndex;
} DragMoveUnits;

typedef struct DragCreateUnion {
    UnitRange Units;
    Unit* OriginUnit;
    Vec2i WireStartPos;
    int WhichHandle;
} DragCreateUnion;

typedef struct DragContext {
    int Type;

    union {
        DragWire Wire;
        DragBoxSelect BoxSelect;
        DragMoveUnits MoveUnits;
        DragCreateUnion CreateUnion;
    };
} DragContext;

GLOBAL NoUnionEx moveUnitsEx;
GLOBAL DragContext drag;

void MoveAllUnitsTo(NoUnionEx* ex, int index);
