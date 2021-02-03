#include "range.h"
#include "globals.h"

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
