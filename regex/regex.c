#include "regex.h"

char* toString_Regex(char* base, Regex* regex);
char* toString_NoUnionEx(char* base, NoUnionEx* ex);
char* toString_Unit(char* base, Unit* unit);
char* toString_UnitContents(char* base, UnitContents* contents);
char* toString_UnitRepetition(char* base, UnitRepetition* rep);
char* toString_Group(char* base, Group* group);
char* toString_Set(char* base, Set* set);
char* toString_SetItem(char* base, SetItem* item);
char* toString_SetItemRange(char* base, SetItemRange* range);
char* toString_Special(char* base, Special* special);
char* toString_Char(char* base, Char* c);


char toStringBuf[2048];
char* ToString(Regex* regex) {
    toString_Regex(toStringBuf, regex);
    return toStringBuf;
}

char* writeLiteral(char* base, const char* literal) {
    size_t len = strlen(literal);
    memcpy(base, literal, len);
    return base + len;
}

char* toString_Regex(char* base, Regex* regex) {
    base = toString_NoUnionEx(base, regex->UnionMembers[0]);
    for (int i = 1; i < regex->NumUnionMembers; i++) {
        base = writeLiteral(base, "|");
        base = toString_NoUnionEx(base, regex->UnionMembers[i]);
    }

    return base;
}

char* toString_NoUnionEx(char* base, NoUnionEx* ex) {
    for (int i = 0; i < ex->NumUnits; i++) {
        base = toString_Unit(base, ex->Units[i]);
    }

    return base;
}

char* toString_Unit(char* base, Unit* unit) {
    base = toString_UnitContents(base, unit->Contents);
    base = toString_UnitRepetition(base, unit->Repetition);

    return base;
}

char* toString_UnitContents(char* base, UnitContents* contents) {
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
        case RE_CONTENTS_CHAR: {
            base = toString_Char(base, contents->Char);
        } break;
    }

    return base;
}

char* toString_UnitRepetition(char* base, UnitRepetition* rep) {
    if (rep == NULL) {
        return base;
    }

    if (rep->Min == 0 && rep->Max < 0) {
        base = writeLiteral(base, "*");
    } else if (rep->Min == 1 && rep->Max < 0) {
        base = writeLiteral(base, "+");
    } else if (rep->Min == 0 && rep->Max == 1) {
        base = writeLiteral(base, "?");
    } else {
        base = writeLiteral(base, "{");
        base += sprintf(base, "%d", rep->Min);
        base = writeLiteral(base, ",");
        if (rep->Max >= 0) {
            base += sprintf(base, "%d", rep->Max);
        }
        base = writeLiteral(base, "}");
    }

    return base;
}

char* toString_Group(char* base, Group* group) {
    base = writeLiteral(base, "(");
    base = toString_Regex(base, group->Regex);
    base = writeLiteral(base, ")");

    return base;
}

char* toString_Set(char* base, Set* set) {
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
    switch (item->Type) {
        case RE_SETITEM_CHAR: {
            base = toString_Char(base, item->Char);
        } break;
        case RE_SETITEM_RANGE: {
            base = toString_SetItemRange(base, item->Range);
        } break;
    }

    return base;
}

char* toString_SetItemRange(char* base, SetItemRange* range) {
    base = toString_Char(base, range->Min);
    base = writeLiteral(base, "-");
    base = toString_Char(base, range->Max);

    return base;
}

char* toString_Special(char* base, Special* special) {
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

char* toString_Char(char* base, Char* c) {
    switch (c->Type) {
        case RE_CHAR_LITERAL: {
            *base = c->Literal;
            base++;
        } break;
        case RE_CHAR_META: {
            base = writeLiteral(base, "\\");
            *base = c->Meta;
            base++;
        } break;
    }

    return base;
}

