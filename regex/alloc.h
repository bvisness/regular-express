#ifndef REGEX_ALLOC_H
#define REGEX_ALLOC_H

#include "pool.h"
#include "regex.h"

#define RE_POOL_NAME(T) pool_##T
#define RE_GET_POOL(T) getPool_##T
#define RE_GET_POOL_DEF(T) Pool* RE_GET_POOL(T)()
#define RE_NEW(T) ((T*) pool_alloc(getPool_##T()))
#define RE_FREE(T, ptr) pool_free(RE_GET_POOL(T)(), ptr)

RE_GET_POOL_DEF(Regex);
RE_GET_POOL_DEF(NoUnionEx);
RE_GET_POOL_DEF(Unit);
RE_GET_POOL_DEF(UnitContents);
RE_GET_POOL_DEF(LitChar);
RE_GET_POOL_DEF(MetaChar);
RE_GET_POOL_DEF(Special);
RE_GET_POOL_DEF(Set);
RE_GET_POOL_DEF(SetItem);
RE_GET_POOL_DEF(SetItemRange);
RE_GET_POOL_DEF(Group);

Regex* Regex_init(Regex* regex);
NoUnionEx* NoUnionEx_init(NoUnionEx* ex);
Unit* Unit_init(Unit* unit);
UnitContents* UnitContents_init(UnitContents* contents);
LitChar* LitChar_init(LitChar* c);
MetaChar* MetaChar_init(MetaChar* c);
Special* Special_init(Special* special);
Set* Set_init(Set* set);
SetItem* SetItem_init(SetItem* item);
SetItemRange* SetItemRange_init(SetItemRange* range);
Group* Group_init(Group* group);

Unit* Unit_initWithLiteralChar(Unit* unit, char c);

void UnitContents_SetType(UnitContents* contents, int type);

#endif
