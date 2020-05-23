#ifndef REGEX_H
#define REGEX_H

#include <string.h>
#include <stdio.h>

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
};

typedef struct NoUnionEx {
    int NumUnits;
    struct Unit* Units[MAX_UNITS];
} NoUnionEx;

typedef struct Unit {
    struct UnitContents* Contents;

    int Repeats;
    struct UnitRepetition* Repetition;
} Unit;

typedef struct UnitContents {
    int Type;

    struct LitChar* LitChar;
    struct MetaChar* MetaChar;
    struct Special* Special;
    struct Set* Set;
    struct Group* Group;
} UnitContents;

typedef struct UnitRepetition {
    int Min;
    int Max; // assume that zero means unbounded

    float _minbuf;
    float _maxbuf;
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

    struct LitChar* LitChar;
    struct SetItemRange* Range;
} SetItem;

typedef struct SetItemRange {
    struct LitChar* Min;
    struct LitChar* Max;
} SetItemRange;

typedef struct Special {
    int Type;
} Special;

typedef struct LitChar {
    union {
        char C;
        char _buf[2];
    };
} LitChar;

typedef struct MetaChar {
    union {
        char C;
        char _buf[2];
    };
} MetaChar;

typedef union RegexType {
    Regex Regex;
    NoUnionEx NoUnionEx;
    Unit Unit;
    UnitContents UnitContents;
    UnitRepetition UnitRepetition;
    Group Group;
    Set Set;
    SetItem SetItem;
    SetItemRange SetItemRange;
    Special Special;
    LitChar LitChar;
    MetaChar MetaChar;
} RegexType;


char* ToString(Regex* regex);


#endif
