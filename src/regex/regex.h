#pragma once

#include <string.h>
#include <stdio.h>

#include "vec.h"
#include "textinput.h"

#include "../microui.h"

#define MAX_UNION_MEMBERS 256
#define MAX_UNITS 256
#define MAX_SET_ITEMS 256
#define MAX_UNIT_CHARS 256

enum {
    RE_CONTENTS_LITCHAR,
    RE_CONTENTS_METACHAR,
    RE_CONTENTS_SPECIAL,
    RE_CONTENTS_SET,
    RE_CONTENTS_GROUP,
};
const char* RE_CONTENTS_ToString(int v);

enum {
    RE_SETITEM_LITCHAR,
    RE_SETITEM_RANGE,
};
const char* RE_SETITEM_ToString(int v);

enum {
    RE_SPECIAL_ANY,
    RE_SPECIAL_STRINGSTART,
    RE_SPECIAL_STRINGEND,
};

typedef struct Regex Regex;
struct Regex {
    int NumUnionMembers;
    struct NoUnionEx* UnionMembers[MAX_UNION_MEMBERS];

    Vec2i Size;
    Vec2i UnionSize;
    int WireHeight;
};

void Regex_AddUnionMember(Regex* regex, struct NoUnionEx* ex, int index);
struct NoUnionEx* Regex_RemoveUnionMember(Regex* regex, int index);
void Regex_PushUndo(Regex* regex);

typedef struct NoUnionEx {
    int NumUnits;
    struct Unit* Units[MAX_UNITS];

    int Index;
    Vec2i Size;
    int WireHeight;
    TextInputState TextState;
    int ClickedUnitIndex;
} NoUnionEx;

void NoUnionEx_AddUnit(NoUnionEx* ex, struct Unit* unit, int index);
struct Unit* NoUnionEx_RemoveUnit(NoUnionEx* ex, int index);
void NoUnionEx_ReplaceUnits(NoUnionEx* ex, int Start, int End, struct Unit* unit);
void NoUnionEx_PushUndo(NoUnionEx* ex);

typedef struct LitChar {
    union {
        char C;
        char _buf[2];
    };
} LitChar;

typedef struct MetaChar {
    char _backslash;
    union {
        char C;
        char _buf[2];
    };
} MetaChar;

typedef struct Special {
    int Type;
} Special;

const char* Special_GetHumanString(Special* s);

typedef struct UnitContents {
    int Type;

    struct LitChar LitChar;
    struct MetaChar MetaChar;
    struct Special Special;
    struct Set* Set;
    struct Group* Group;

    Vec2i Size;
    int WireHeight;
    mu_Rect LastRect;
} UnitContents;

void UnitContents_PushUndo(UnitContents* contents);

typedef struct Unit {
    struct UnitContents Contents;

    int RepeatMin;
    int RepeatMax; // zero means unbounded

    float _minbuf;
    float _maxbuf;

    // UI/layout info
    Vec2i Size;
    int WireHeight;
    mu_Rect LastRect;

    struct NoUnionEx* Parent;
    int Index;

    int IsHover;
    int IsContentHover;
    int IsLeftWireHover;
    int IsRightWireHover;
    int IsShowingLeftHandle;
    int IsShowingRightHandle;
    int IsWireDragOrigin;
    int IsSelected;

    float LeftHandleZoneWidth;
    float RightHandleZoneWidth;
} Unit;

Unit* Unit_Previous(Unit* unit);
Unit* Unit_Next(Unit* unit);
void Unit_SetRepeatMin(Unit* unit, int val);
void Unit_SetRepeatMax(Unit* unit, int val);
int Unit_IsNonSingular(Unit* unit);
int Unit_IsSkip(Unit* unit);
int Unit_IsRepeat(Unit* unit);
int Unit_ShouldShowLeftHandle(Unit* unit);
int Unit_ShouldShowRightHandle(Unit* unit);
void Unit_PushUndo(Unit* unit);

typedef struct Group {
    struct Regex* Regex;
    // more properties eventually, like names

    Vec2i Size;
    int WireHeight;
} Group;

void Group_PushUndo(Group* group);

typedef struct Set {
    int NumItems;
    struct SetItem* Items[MAX_SET_ITEMS];

    int IsNegative;
    TextInputState TextState;

    Vec2i Size;
} Set;

void Set_AddItem(Set* set, struct SetItem* item, int index);
struct SetItem* Set_RemoveItem(Set* set, int index);
void Set_PushUndo(Set* set);

typedef struct SetItemRange {
    struct LitChar Min;
    struct LitChar Max;
} SetItemRange;

typedef struct SetItem {
    int Type;

    struct LitChar LitChar;
    struct SetItemRange Range;

    Vec2i Size;
    mu_Rect LastRect;
} SetItem;

void SetItem_PushUndo(SetItem* item);

char* ToString(Regex* regex);
