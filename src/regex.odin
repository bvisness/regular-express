package main

Regex :: struct {
    union_members: [dynamic]^NoUnionEx,
}

NoUnionEx :: struct {
    units: [dynamic]^Unit,

    index: int, // TODO: unclear if indexes are truly "secondary data" when it comes to undo/redo.
}

Unit :: struct {
    contents: UnitContents,

    repeat_min: int,
    repeat_max: int, // zero means unbounded

    parent: ^NoUnionEx,
    index: int,
}

UnitContents :: union #no_nil {
    LitChar,
    MetaChar,
    Special,
    ^Set,
    ^Group,
    UnknownContents,
}

LitChar :: struct {
    c: u8, // unicode someday?
}

MetaChar :: struct {
    c: u8,
}

Special :: enum {
    Any,
    StringStart,
    StringEnd,
}

Set :: struct {
    items: [dynamic]^SetItem,
    is_negative: bool,
}

SetItem :: union #no_nil {
    LitChar,
    MetaChar,
    SetItemRange,
}

SetItemRange :: struct {
    min, max: LitChar,
}

Group :: struct {
    ty: GroupType,
    regex: ^Regex,
    name: string,
}

GroupType :: enum {
    Normal,
    NonCapturing,
    Named,
    PositiveLookahead,
    NegativeLookahead,
    PositiveLookbehind,
    NegativeLookbehind,
}

UnknownContents :: struct {
    str: string,
}
