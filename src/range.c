#include "range.h"
#include "globals.h"
#include "util/math.h"

void MoveUnitsTo(UnitRange range, NoUnionEx* ex, int startIndex) {
    mu_Id exId = NoUnionEx_GetID(range.Ex);

    int doTransferSelection = (ctx->focus == exId && TextState_IsSelecting(range.Ex->TextState));
    TextInputState newTextState;
    if (doTransferSelection) {
        int insertIndexWithinRange = range.Ex->TextState.InsertIndex - range.Start;
        int cursorIndexWithinRange = range.Ex->TextState.CursorIndex - range.Start;
        int selectionBaseWithinRange = range.Ex->TextState.SelectionBase - range.Start;
        newTextState = (TextInputState) {
            .InsertIndex = startIndex + insertIndexWithinRange,
            .CursorIndex = startIndex + cursorIndexWithinRange,
            .CursorRight = range.Ex->TextState.CursorRight,
            .SelectionBase = startIndex + selectionBaseWithinRange,
        };
    }

    if (startIndex < 0) {
        startIndex = ex->NumUnits;
    }

    for (int i = 0; i <= range.End - range.Start; i++) {
        Unit* unit = range.Ex->Units[range.Start]; // grab head of range
        NoUnionEx_RemoveUnit(range.Ex, range.Start);
        NoUnionEx_AddUnit(ex, unit, startIndex + i);
    }

    if (doTransferSelection) {
        ex->TextState = newTextState;
    }
}

Unit* ConvertRangeToGroup(UnitRange range) {
    Unit* newUnit = Unit_init(RE_NEW(Unit));
    UnitContents_SetType(&newUnit->Contents, RE_CONTENTS_GROUP);

    NoUnionEx* ex = newUnit->Contents.Group->Regex->UnionMembers[0];
    MoveUnitsTo(range, ex, 0);

    NoUnionEx_AddUnit(range.Ex, newUnit, range.Start);

    return newUnit;
}

void DeleteRange(UnitRange range) {
    int delMin = iclamp(range.Start, 0, range.Ex->NumUnits);
    int delMax = iclamp(range.End, 0, range.Ex->NumUnits);

    // Adjust text input state, keeping in mind that parts of the state
    // may be either in the deleted range or after the deleted range.
    TextInputState newTextState = range.Ex->TextState;
    if (delMax < range.Ex->TextState.InsertIndex) {
        newTextState.InsertIndex -= (delMax - delMin + 1);
        newTextState.CursorIndex -= (delMax - delMin + 1);
    } else if (
        delMin <= range.Ex->TextState.InsertIndex
        && range.Ex->TextState.InsertIndex <= delMax
    ) {
        newTextState.InsertIndex = delMin;
        newTextState.CursorIndex = delMin;
        newTextState.CursorRight = 0;
    }
    if (delMax < range.Ex->TextState.SelectionBase) {
        newTextState.SelectionBase -= (delMax - delMin + 1);
    } else if (
        delMin <= range.Ex->TextState.SelectionBase
        && range.Ex->TextState.SelectionBase <= delMax
    ) {
        newTextState.SelectionBase = delMin;
    }

    for (int i = 0; i <= delMax - delMin; i++) {
        Unit* deleted = NoUnionEx_RemoveUnit(range.Ex, delMin);
        Unit_delete(deleted);
    }

    range.Ex->TextState = newTextState;
}
