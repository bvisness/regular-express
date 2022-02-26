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
#define MAX_UNKNOWN_CONTENT_CHARS 32

enum {
    RE_CONTENTS_LITCHAR,
    RE_CONTENTS_METACHAR,
    RE_CONTENTS_SPECIAL,
    RE_CONTENTS_SET,
    RE_CONTENTS_GROUP,
    RE_CONTENTS_UNKNOWN,
};
const char* RE_CONTENTS_ToString(int v);

enum {
    RE_SETITEM_LITCHAR,
    RE_SETITEM_METACHAR,
    RE_SETITEM_RANGE,
};
const char* RE_SETITEM_ToString(int v);

enum {
    RE_SPECIAL_ANY,
    RE_SPECIAL_STRINGSTART,
    RE_SPECIAL_STRINGEND,
};

enum {
    RE_GROUP_NORMAL,
    RE_GROUP_NON_CAPTURING,
    RE_GROUP_NAMED,
    RE_GROUP_POSITIVE_LOOKAHEAD,
    RE_GROUP_NEGATIVE_LOOKAHEAD,
    RE_GROUP_POSITIVE_LOOKBEHIND,
    RE_GROUP_NEGATIVE_LOOKBEHIND,
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

mu_Id NoUnionEx_GetID(NoUnionEx* ex);
void NoUnionEx_AddUnit(NoUnionEx* ex, struct Unit* unit, int index);
struct Unit* NoUnionEx_RemoveUnit(NoUnionEx* ex, int index);
void NoUnionEx_RemoveUnits(NoUnionEx* ex, int minIndex, int maxIndex);
void NoUnionEx_RemoveSelection(NoUnionEx* ex);
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

char* MetaChar_GetHumanString(MetaChar* m);

typedef struct Special {
    int Type;
} Special;

const char* Special_GetHumanString(Special* s);

typedef struct UnknownContents {
    char Str[MAX_UNKNOWN_CONTENT_CHARS];
} UnknownContents;

typedef struct UnitContents {
    int Type;

    struct LitChar LitChar;
    struct MetaChar MetaChar;
    struct Special Special;
    struct Set* Set;
    struct Group* Group;
    struct UnknownContents Unknown;

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
    int Type;
    struct Regex* Regex;

    char Name[128];

    Vec2i Size;
    int WireHeight;
} Group;

void Group_PushUndo(Group* group);
int Group_CanRender(Group* group);

typedef struct Set {
    int NumItems;
    struct SetItem* Items[MAX_SET_ITEMS];

    int IsNegative;
    TextInputState TextState;

    Vec2i ItemsSize;
} Set;

mu_Id Set_GetID(Set* set);
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
    struct MetaChar MetaChar;
    struct SetItemRange Range;

    Vec2i Size;
    mu_Rect LastRect;
} SetItem;

void SetItem_MakeRange(SetItem* item, char start, char end);
void SetItem_PushUndo(SetItem* item);

char* ToString(Regex* regex);
