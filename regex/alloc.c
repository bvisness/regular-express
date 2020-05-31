#include "alloc.h"

int regexEntityPoolInitialized = 0;
Pool regexEntityPool;

Pool* getRegexPool() {
	if (!regexEntityPoolInitialized) {
		POOL_INIT(RegexType, &regexEntityPool);
		regexEntityPoolInitialized = 1;
	}

	return &regexEntityPool;
}

Regex* Regex_init(Regex* regex) {
	regex->UnionMembers[0] = NoUnionEx_init((NoUnionEx*) pool_alloc(&regexEntityPool));;
	regex->NumUnionMembers = 1;
	return regex;
}

NoUnionEx* NoUnionEx_init(NoUnionEx* ex) {
	return ex;
}

Unit* Unit_init(Unit* unit) {
	unit->Contents = UnitContents_init((UnitContents*) pool_alloc(&regexEntityPool));
	unit->RepeatMin = 1;
	unit->RepeatMax = 1;
	unit->_minbuf = 1.0f;
	unit->_maxbuf = 1.0f;
	return unit;
}

UnitContents* UnitContents_init(UnitContents* contents) {
	contents->Type = RE_CONTENTS_LITCHAR;
	contents->LitChar = LitChar_init((LitChar*) pool_alloc(&regexEntityPool));
	contents->MetaChar = MetaChar_init((MetaChar*) pool_alloc(&regexEntityPool));
	contents->Special = Special_init((Special*) pool_alloc(&regexEntityPool));
	contents->Set = Set_init((Set*) pool_alloc(&regexEntityPool));
	contents->Group = Group_init((Group*) pool_alloc(&regexEntityPool));
	return contents;
}

LitChar* LitChar_init(LitChar* c) {
	c->C = 'a';
	return c;
}

MetaChar* MetaChar_init(MetaChar* c) {
	c->_backslash = '\\';
	c->C = 'a';
	return c;
}

Special* Special_init(Special* special) {
	return special;
}

Set* Set_init(Set* set) {
	set->Items[0] = SetItem_init((SetItem*) pool_alloc(&regexEntityPool));;
	set->NumItems = 1;
	return set;
}

SetItem* SetItem_init(SetItem* item) {
	item->Type = RE_SETITEM_LITCHAR;
	item->LitChar = LitChar_init((LitChar*) pool_alloc(&regexEntityPool));
	item->Range = SetItemRange_init((SetItemRange*) pool_alloc(&regexEntityPool));
	return item;
}

SetItemRange* SetItemRange_init(SetItemRange* range) {
	range->Min = LitChar_init((LitChar*) pool_alloc(&regexEntityPool));
	range->Max = LitChar_init((LitChar*) pool_alloc(&regexEntityPool));
	return range;
}

Group* Group_init(Group* group) {
	group->Regex = Regex_init((Regex*) pool_alloc(&regexEntityPool));
	return group;
}
