#ifndef REGEX_ALLOC_H
#define REGEX_ALLOC_H

#include "pool.h"
#include "regex.h"

#define RE_POOL_NAME(T) pool_##T
#define RE_GET_POOL(T) getPool_##T
#define RE_GET_POOL_DEF(T) Pool* RE_GET_POOL(T)()
#define RE_NEW(T) ((T*) pool_alloc(getPool_##T()))
#define RE_FREE(T, ptr) pool_free(RE_GET_POOL(T)(), ptr)
#define RE_PRINT_POOL(T) printf(#T " pool (max %d): %d", POOL_SIZE, RE_GET_POOL(T)()->count);

RE_GET_POOL_DEF(Regex);
RE_GET_POOL_DEF(NoUnionEx);
RE_GET_POOL_DEF(Unit);
RE_GET_POOL_DEF(MetaChar);
RE_GET_POOL_DEF(Special);
RE_GET_POOL_DEF(Set);
RE_GET_POOL_DEF(SetItem);
RE_GET_POOL_DEF(Group);

Regex* Regex_init(Regex* regex);
NoUnionEx* NoUnionEx_init(NoUnionEx* ex);
Unit* Unit_init(Unit* unit);
void UnitContents_init(UnitContents* contents);
void LitChar_init(LitChar* c);
MetaChar* MetaChar_init(MetaChar* c);
Special* Special_init(Special* special);
Set* Set_init(Set* set);
SetItem* SetItem_init(SetItem* item);
void SetItemRange_init(SetItemRange* range);
Group* Group_init(Group* group);

Unit* Unit_initWithLiteralChar(Unit* unit, char c);

void UnitContents_SetType(UnitContents* contents, int type);

void Regex_delete(Regex* regex);
void NoUnionEx_delete(NoUnionEx* ex);
void Unit_delete(Unit* unit);
void MetaChar_delete(MetaChar* c);
void Special_delete(Special* special);
void Set_delete(Set* set);
void SetItem_delete(SetItem* item);
void Group_delete(Group* group);

#endif
