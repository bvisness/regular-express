#include "drag.h"

void MoveAllUnitsTo(NoUnionEx* ex, int index) {
    while (moveUnitsEx.NumUnits > 0) {
        Unit* unit = NoUnionEx_RemoveUnit(&moveUnitsEx, -1);
        NoUnionEx_AddUnit(ex, unit, index);
    }
}
