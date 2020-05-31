#ifndef REGEX_ALLOC_H
#define REGEX_ALLOC_H

#include "pool.h"
#include "regex.h"

Pool* getRegexPool();

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

#endif
