#include "alloc.h"
#include "textinput.h"

#define RE_DEFINE_POOL(T) 			\
Pool RE_POOL_NAME(T);				\
Pool* RE_GET_POOL(T)() {				\
	if (!pool_##T.buf) {			\
		POOL_INIT(T, &pool_##T);	\
	}								\
									\
	return &pool_##T;				\
}

RE_DEFINE_POOL(Regex);
RE_DEFINE_POOL(NoUnionEx);
RE_DEFINE_POOL(Unit);
RE_DEFINE_POOL(UnitContents);
RE_DEFINE_POOL(LitChar);
RE_DEFINE_POOL(MetaChar);
RE_DEFINE_POOL(Special);
RE_DEFINE_POOL(Set);
RE_DEFINE_POOL(SetItem);
RE_DEFINE_POOL(SetItemRange);
RE_DEFINE_POOL(Group);

Regex* Regex_init(Regex* regex) {
	regex->UnionMembers[0] = NoUnionEx_init(RE_NEW(NoUnionEx));
	regex->NumUnionMembers = 1;
	return regex;
}

NoUnionEx* NoUnionEx_init(NoUnionEx* ex) {
	ex->TextState = DEFAULT_TEXT_INPUT_STATE;
	return ex;
}

Unit* Unit_init(Unit* unit) {
	unit->Contents = UnitContents_init(RE_NEW(UnitContents));
	unit->RepeatMin = 1;
	unit->RepeatMax = 1;
	unit->_minbuf = 1.0f;
	unit->_maxbuf = 1.0f;
	return unit;
}

UnitContents* UnitContents_init(UnitContents* contents) {
	UnitContents_SetType(contents, RE_CONTENTS_LITCHAR);
	return contents;
}

LitChar* LitChar_init(LitChar* c) {
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
	set->Items[0] = SetItem_init(RE_NEW(SetItem));
	set->NumItems = 1;
	return set;
}

SetItem* SetItem_init(SetItem* item) {
	item->Type = RE_SETITEM_LITCHAR;
	item->LitChar = LitChar_init(RE_NEW(LitChar));
	item->Range = SetItemRange_init(RE_NEW(SetItemRange));
	return item;
}

SetItemRange* SetItemRange_init(SetItemRange* range) {
	range->Min = LitChar_init(RE_NEW(LitChar));
	range->Max = LitChar_init(RE_NEW(LitChar));
	return range;
}

Group* Group_init(Group* group) {
	group->Regex = Regex_init(RE_NEW(Regex));
	return group;
}

// convenience thingies that need to allocate memory

Unit* Unit_initWithLiteralChar(Unit* unit, char c) {
	Unit_init(unit);
	unit->Contents->LitChar->C = c;

	return unit;
}

void UnitContents_SetType(UnitContents* contents, int type) {
	contents->Type = type;
	switch (type) {
		case RE_CONTENTS_LITCHAR: {
			if (!contents->LitChar) {
				contents->LitChar = LitChar_init(RE_NEW(LitChar));
			}
		} break;
		case RE_CONTENTS_METACHAR: {
			if (!contents->MetaChar) {
				contents->MetaChar = MetaChar_init(RE_NEW(MetaChar));
			}
		} break;
		case RE_CONTENTS_SPECIAL: {
			if (!contents->Special) {
				contents->Special = Special_init(RE_NEW(Special));
			}
		} break;
		case RE_CONTENTS_SET: {
			if (!contents->Set) {
				contents->Set = Set_init(RE_NEW(Set));
			}
		} break;
		case RE_CONTENTS_GROUP: {
			if (!contents->Group) {
				contents->Group = Group_init(RE_NEW(Group));
			}
		} break;
	}
}
