#include <string.h>

#include "parser.h"
#include "alloc.h"
#include "../globals.h"

Unit* parseSet(char* regexStr, int* i, int len);
Regex* parseRegex(char* regexStr, int* i, int len);

/*
(Hello, )+(world!| or (whatever|something))?[abc-jz]*
*wow
+wow
?wow
*/

Regex* parse(char* regexStr) {
    int i = 0;
    int len = strlen(regexStr);

    return parseRegex(regexStr, &i, len);
}

Regex* parseRegex(char* regexStr, int* i, int len) {
    Regex* regex = Regex_init(RE_NEW(Regex));
    NoUnionEx* ex = regex->UnionMembers[0];

    while (*i < len) {
        char c = regexStr[*i];

        if (c == ')') {
            break;
        } else if (c == '(') {
            (*i)++;

            int groupType = RE_GROUP_NORMAL;
            int groupNameStart = 0;
            int groupNameLength = 0;

            if (regexStr[*i] == '?') {
                (*i)++;

                /*
                ?=          positive lookahead
                ?!          negative lookahead
                ?<=         positive lookbehind
                ?<!         negative lookbehind
                ?:          non-capturing group
                ?<name>     named group
                */

                switch (regexStr[*i]) {
                case '=': {
                    // positive lookahead
                    groupType = RE_GROUP_POSITIVE_LOOKAHEAD;
                } break;
                case '!': {
                    // negative lookahead
                    groupType = RE_GROUP_NEGATIVE_LOOKAHEAD;
                } break;
                case '<': {
                    // lookbehinds, or named group
                    (*i)++;

                    switch (regexStr[*i]) {
                    case '=': {
                        // positive lookbehind
                        groupType = RE_GROUP_POSITIVE_LOOKBEHIND;
                    } break;
                    case '!': {
                        // negative lookbehind
                        groupType = RE_GROUP_NEGATIVE_LOOKBEHIND;
                    } break;
                    default: {
                        // named group!
                        // for now, assume that the first occurrence of > is the end, screw escaping
                        groupType = RE_GROUP_NAMED;

                        groupNameStart = *i;
                        while (regexStr[*i] != '>') {
                            (*i)++;
                            groupNameLength++;
                        }
                    } break;
                    }
                } break;
                case ':': {
                    // non-capturing group
                    groupType = RE_GROUP_NON_CAPTURING;
                } break;
                }

                (*i)++;
            }

            Regex* groupRegex = parseRegex(regexStr, i, len);

            Unit* newUnit = Unit_init(RE_NEW(Unit));
            UnitContents_SetType(&newUnit->Contents, RE_CONTENTS_GROUP);

            // we don't need the regex that was automatically created by
            // switching to type group.
            Regex_delete(newUnit->Contents.Group->Regex);

            newUnit->Contents.Group->Regex = groupRegex;
            newUnit->Contents.Group->Type = groupType;

            if (groupType == RE_GROUP_NAMED) {
                memcpy(newUnit->Contents.Group->Name, &regexStr[groupNameStart], groupNameLength);
                newUnit->Contents.Group->Name[groupNameLength] = 0;
            }

            NoUnionEx_AddUnit(ex, newUnit, -1);
        } else if (c == '|') {
            // TODO: Maybe this should use a helper method on NoUnionEx
            NoUnionEx* newEx = NoUnionEx_init(RE_NEW(NoUnionEx));
            regex->UnionMembers[regex->NumUnionMembers] = newEx;
            regex->NumUnionMembers++;

            ex = newEx;
        } else if (c == '[') {
            (*i)++;
            Unit* setUnit = parseSet(regexStr, i, len);
            NoUnionEx_AddUnit(ex, setUnit, -1);
        } else if (c == '*') {
            // TODO: Recognize as literal character if no previous unit exists
            Unit* lastUnit = ex->Units[ex->NumUnits - 1];
            Unit_SetRepeatMin(lastUnit, 0);
            Unit_SetRepeatMax(lastUnit, 0);
        } else if (c == '+') {
            Unit* lastUnit = ex->Units[ex->NumUnits - 1];
            Unit_SetRepeatMin(lastUnit, 1);
            Unit_SetRepeatMax(lastUnit, 0);
        } else if (c == '?') {
            Unit* lastUnit = ex->Units[ex->NumUnits - 1];
            Unit_SetRepeatMin(lastUnit, 0);
            Unit_SetRepeatMax(lastUnit, 1);
        } else if (c == '^') {
            Unit* newUnit = Unit_init(RE_NEW(Unit));
            UnitContents_SetType(&newUnit->Contents, RE_CONTENTS_SPECIAL);
            newUnit->Contents.Special.Type = RE_SPECIAL_STRINGSTART;
            NoUnionEx_AddUnit(ex, newUnit, -1);
        } else if (c == '$') {
            Unit* newUnit = Unit_init(RE_NEW(Unit));
            UnitContents_SetType(&newUnit->Contents, RE_CONTENTS_SPECIAL);
            newUnit->Contents.Special.Type = RE_SPECIAL_STRINGEND;
            NoUnionEx_AddUnit(ex, newUnit, -1);
        } else if (c == '.') {
            Unit* newUnit = Unit_init(RE_NEW(Unit));
            UnitContents_SetType(&newUnit->Contents, RE_CONTENTS_SPECIAL);
            newUnit->Contents.Special.Type = RE_SPECIAL_ANY;
            NoUnionEx_AddUnit(ex, newUnit, -1);
        } else if (c == '\\') {
            // escape sequence
            int unknownContentStart = *i;
            int unknownContentLength = 0;

            (*i)++;

            Unit* newUnit = NULL;
            switch (regexStr[*i]) {
            case 'u': {
                // unicode escape sequence (\uHHHH or \u{HHH_HH...})
                if (regexStr[*i + 1] == '{') {
                    // weirdo long syntax I guess
                    unknownContentLength = 2;
                    while (regexStr[*i] != '}') {
                        (*i)++;
                        unknownContentLength++;
                    }
                } else {
                    unknownContentLength = 6;
                    (*i) += 4;
                }
            } break;
            case 'x': {
                // character by hex code (\xHH)
                unknownContentLength = 4;
                (*i) += 2;
            } break;
            case 'c': {
                // control character by hex code (\cH)
                unknownContentLength = 3;
                (*i) += 1;
            } break;
            default:
                // literal char...or maybe metachar
                if (strchr(LEGAL_METACHARS, regexStr[*i])) {
                    // metachar
                    newUnit = Unit_init(RE_NEW(Unit));
                    UnitContents_SetType(&newUnit->Contents, RE_CONTENTS_METACHAR);
                    newUnit->Contents.MetaChar.C = regexStr[*i];
                } else {
                    newUnit = Unit_initWithLiteralChar(RE_NEW(Unit), regexStr[*i]);
                }
            }

            if (!newUnit) {
                // I guess it's unknown huh
                newUnit = Unit_init(RE_NEW(Unit));
                UnitContents_SetType(&newUnit->Contents, RE_CONTENTS_UNKNOWN);
                memcpy(newUnit->Contents.Unknown.Str, &regexStr[unknownContentStart], unknownContentLength);
                newUnit->Contents.Unknown.Str[unknownContentLength] = 0;
            }

            NoUnionEx_AddUnit(ex, newUnit, -1);
        } else {
            Unit* newUnit = Unit_initWithLiteralChar(RE_NEW(Unit), c);
            NoUnionEx_AddUnit(ex, newUnit, -1);
        }

        (*i)++;
    }

    return regex;
}

Unit* parseSet(char* regexStr, int* i, int len) {
    Unit* setUnit = Unit_init(RE_NEW(Unit));
    UnitContents_SetType(&setUnit->Contents, RE_CONTENTS_SET);

    Set* set = setUnit->Contents.Set;

    if (regexStr[*i] == '^') {
        set->IsNegative = 1;
        (*i)++;
    }

    while (1) {
        char c = regexStr[*i];

        if (*i == len - 1 && c != ']') {
            // malformed regex, quit
            break;
        }

        if (c == ']') {
            break;
        } else if (c == '-'
                    && set->NumItems > 0 // there is a previous item that is a literal char
                    && set->Items[set->NumItems - 1]->Type == RE_SETITEM_LITCHAR
                    && regexStr[*i + 1] != ']' // there is a character after us in the set
        ) {
            // make a range!
            SetItem* item = set->Items[set->NumItems - 1];
            item->Type = RE_SETITEM_RANGE;
            item->Range.Min.C = item->LitChar.C;
            item->Range.Max.C = regexStr[*i + 1];
            (*i)++;
        } else if (c == '\\') {
            (*i)++;
            c = regexStr[*i];
            SetItem* item = SetItem_init(RE_NEW(SetItem));
            item->LitChar.C = c;
            Set_AddItem(set, item, -1);
        } else {
            SetItem* item = SetItem_init(RE_NEW(SetItem));
            item->LitChar.C = c;
            Set_AddItem(set, item, -1);
        }

        (*i)++;
    }

    return setUnit;
}
