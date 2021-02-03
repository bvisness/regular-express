#include "drag.h"
#include "microui.h"
#include "regex/textinput.h"
#include "range.h"

void MoveAllUnitsTo(NoUnionEx* ex, int index) {
    UnitRange all = (UnitRange) {
        .Ex = &moveUnitsEx,
        .Start = 0,
        .End = moveUnitsEx.NumUnits - 1,
    };
    MoveUnitsTo(all, ex, index);
}
