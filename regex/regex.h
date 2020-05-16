#ifndef REGEX_H
#define REGEX_H

#include <string.h>
#include <stdio.h>

#define MAX_UNION_MEMBERS 256
#define MAX_UNITS 256
#define MAX_SET_ITEMS 256
#define MAX_UNIT_CHARS 256

enum {
    RE_CONTENTS_GROUP,
    RE_CONTENTS_SET,
    RE_CONTENTS_SPECIAL,
    RE_CONTENTS_CHAR,
};

enum {
    RE_SETITEM_CHAR,
    RE_SETITEM_RANGE,
};

enum {
    RE_SPECIAL_ANY,
    RE_SPECIAL_STRINGSTART,
    RE_SPECIAL_STRINGEND,
};

enum {
    RE_CHAR_LITERAL,
    RE_CHAR_META,
};

typedef struct Regex {
    int NumUnionMembers;
    struct NoUnionEx* UnionMembers[MAX_UNION_MEMBERS];
} Regex;

typedef struct NoUnionEx {
    int NumUnits;
    struct Unit* Units[MAX_UNITS];
} NoUnionEx;

typedef struct Unit {
    struct UnitContents* Contents;
    struct UnitRepetition* Repetition;
} Unit;

typedef struct UnitContents {
    int Type;

    union {
        struct Group* Group;
        struct Set* Set;
        struct Special* Special;
        struct Char* Char;
    };
} UnitContents;

typedef struct UnitRepetition {
    int Min;
    int Max; // assume that less than zero means unbounded
} UnitRepetition;

typedef struct Group {
    struct Regex* Regex;
    // more properties eventually, like names
} Group;

typedef struct Set {
    int NumItems;
    struct SetItem* Items[MAX_SET_ITEMS];

    int IsNegative;
} Set;

typedef struct SetItem {
    int Type;

    union {
        struct Char* Char;
        struct SetItemRange* Range;
    };
} SetItem;

typedef struct SetItemRange {
    struct Char* Min;
    struct Char* Max;
} SetItemRange;

typedef struct Special {
    int Type;
} Special;

typedef struct Char {
    int Type;

    union {
        char Literal;
        char Meta;
    };
} Char;

char* ToString(Regex* regex);

#endif
