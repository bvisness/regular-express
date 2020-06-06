#include "regex.h"

void Unit_SetRepeatMin(Unit* unit, int val) {
    unit->RepeatMin = val;
    unit->_minbuf = (float) val;
}

void Unit_SetRepeatMax(Unit* unit, int val) {
    unit->RepeatMax = val;
    unit->_maxbuf = (float) val;
}

int Unit_IsRepeat(Unit* unit) {
    return unit->RepeatMin < 1 || unit->RepeatMax != 1;
}

int Unit_ShouldShowWires(Unit* unit) {
    return unit->IsHover || unit->IsDragOrigin || Unit_IsRepeat(unit);
}

int Unit_ShouldShowLeftHandle(Unit* unit) {
    return Unit_ShouldShowWires(unit) && !unit->Previous;
}

int Unit_ShouldShowRightHandle(Unit* unit) {
    return (
        Unit_ShouldShowWires(unit)
        || (unit->Next && Unit_ShouldShowWires(unit->Next))
    );
}

char* toString_Regex(char* base, Regex* regex);
char* toString_NoUnionEx(char* base, NoUnionEx* ex);
char* toString_Unit(char* base, Unit* unit);
char* toString_UnitContents(char* base, UnitContents* contents);
char* toString_Group(char* base, Group* group);
char* toString_Set(char* base, Set* set);
char* toString_SetItem(char* base, SetItem* item);
char* toString_SetItemRange(char* base, SetItemRange* range);
char* toString_Special(char* base, Special* special);
char* toString_LitChar(char* base, LitChar* c);
char* toString_MetaChar(char* base, MetaChar* c);

/*
Enum Functions
*/

const char* RE_CONTENTS_ToString(int v) {
    switch (v) {
        case RE_CONTENTS_LITCHAR: return "Character";
        case RE_CONTENTS_METACHAR: return "Metacharacter";
        case RE_CONTENTS_SPECIAL: return "Special";
        case RE_CONTENTS_SET: return "Set";
        case RE_CONTENTS_GROUP: return "Group";
    }

    return "UNKNOWN RE_CONTENTS";
}

const char* RE_SETITEM_ToString(int v){
    switch (v) {
        case RE_SETITEM_LITCHAR: return "Character";
        case RE_SETITEM_RANGE: return "Range";
    }

    return "UNKNOWN RE_SETITEM";
}


/*
ToString
*/

char toStringBuf[2048];
char* ToString(Regex* regex) {
    char* finalBase = toString_Regex(toStringBuf, regex);
    *finalBase = 0;

    return toStringBuf;
}

char* writeLiteral(char* base, const char* literal) {
    size_t len = strlen(literal);
    memcpy(base, literal, len);
    return base + len;
}

char* toString_Regex(char* base, Regex* regex) {
    if (regex == NULL) {
        return base;
    }

    for (int i = 0; i < regex->NumUnionMembers; i++) {
        if (i > 0) {
            base = writeLiteral(base, "|");
        }
        base = toString_NoUnionEx(base, regex->UnionMembers[i]);
    }

    return base;
}

char* toString_NoUnionEx(char* base, NoUnionEx* ex) {
    if (ex == NULL) {
        return base;
    }

    for (int i = 0; i < ex->NumUnits; i++) {
        base = toString_Unit(base, ex->Units[i]);
    }

    return base;
}

char* toString_Unit(char* base, Unit* unit) {
    if (unit == NULL) {
        return base;
    }

    base = toString_UnitContents(base, unit->Contents);

    if (unit->RepeatMin == 1 && unit->RepeatMax == 1) {
        // do nothing, this is the default
    } else if (unit->RepeatMin == 0 && unit->RepeatMax == 0) {
        base = writeLiteral(base, "*");
    } else if (unit->RepeatMin == 1 && unit->RepeatMax == 0) {
        base = writeLiteral(base, "+");
    } else if (unit->RepeatMin == 0 && unit->RepeatMax == 1) {
        base = writeLiteral(base, "?");
    } else {
        base = writeLiteral(base, "{");
        base += sprintf(base, "%d", unit->RepeatMin);
        base = writeLiteral(base, ",");
        if (unit->RepeatMax >= 0) {
            base += sprintf(base, "%d", unit->RepeatMax);
        }
        base = writeLiteral(base, "}");
    }

    return base;
}

char* toString_UnitContents(char* base, UnitContents* contents) {
    if (contents == NULL) {
        return base;
    }

    switch (contents->Type) {
        case RE_CONTENTS_GROUP: {
            base = toString_Group(base, contents->Group);
        } break;
        case RE_CONTENTS_SET: {
            base = toString_Set(base, contents->Set);
        } break;
        case RE_CONTENTS_SPECIAL: {
            base = toString_Special(base, contents->Special);
        } break;
        case RE_CONTENTS_LITCHAR: {
            base = toString_LitChar(base, contents->LitChar);
        } break;
        case RE_CONTENTS_METACHAR: {
            base = toString_MetaChar(base, contents->MetaChar);
        } break;
    }

    return base;
}

char* toString_Group(char* base, Group* group) {
    if (group == NULL) {
        return base;
    }

    base = writeLiteral(base, "(");
    base = toString_Regex(base, group->Regex);
    base = writeLiteral(base, ")");

    return base;
}

char* toString_Set(char* base, Set* set) {
    if (set == NULL) {
        return base;
    }

    base = writeLiteral(base, "[");
    if (set->IsNegative) {
        base = writeLiteral(base, "^");
    }
    for (int i = 0; i < set->NumItems; i++) {
        base = toString_SetItem(base, set->Items[i]);
    }
    base = writeLiteral(base, "]");

    return base;
}

char* toString_SetItem(char* base, SetItem* item) {
    if (item == NULL) {
        return base;
    }

    switch (item->Type) {
        case RE_SETITEM_LITCHAR: {
            base = toString_LitChar(base, item->LitChar);
        } break;
        case RE_SETITEM_RANGE: {
            base = toString_SetItemRange(base, item->Range);
        } break;
    }

    return base;
}

char* toString_SetItemRange(char* base, SetItemRange* range) {
    if (range == NULL) {
        return base;
    }

    base = toString_LitChar(base, range->Min);
    base = writeLiteral(base, "-");
    base = toString_LitChar(base, range->Max);

    return base;
}

char* toString_Special(char* base, Special* special) {
    if (special == NULL) {
        return base;
    }

    switch (special->Type) {
        case RE_SPECIAL_ANY: {
            base = writeLiteral(base, ".");
        } break;
        case RE_SPECIAL_STRINGSTART: {
            base = writeLiteral(base, "^");
        } break;
        case RE_SPECIAL_STRINGEND: {
            base = writeLiteral(base, "$");
        } break;
    }

    return base;
}

char* toString_LitChar(char* base, LitChar* c) {
    if (c == NULL) {
        return base;
    }

    *base = c->C;
    base++;

    return base;
}

char* toString_MetaChar(char* base, MetaChar* c) {
    if (c == NULL) {
        return base;
    }

    base = writeLiteral(base, "\\");

    *base = c->C;
    base++;

    return base;
}

