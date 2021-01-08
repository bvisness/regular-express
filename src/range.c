#include "range.h"

void MoveUnitsTo(UnitRange range, NoUnionEx* ex, int startIndex) {
    if (startIndex < 0) {
        startIndex = ex->NumUnits;
    }

    for (int i = 0; i <= range.End - range.Start; i++) {
        Unit* unit = range.Ex->Units[range.Start]; // grab head of range
        NoUnionEx_RemoveUnit(range.Ex, range.Start);
        NoUnionEx_AddUnit(ex, unit, startIndex + i);
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
