#include "alloc.h"
#include "textinput.h"
#include "../undo.h"

#define RE_DEFINE_POOL(T) 			\
Pool RE_POOL_NAME(T);				\
Pool* RE_GET_POOL(T)() {			\
	if (!pool_##T.buf) {			\
		POOL_INIT(T, &pool_##T);	\
	}								\
									\
	return &pool_##T;				\
}

#define POOL_PUSH_UNDO(T)														\
	Undo_Push(&pool_##T.count, sizeof(pool_##T.count), #T " pool count");		\
	Undo_Push(&pool_##T.head, sizeof(pool_##T.head), #T " pool head pointer");	\
	Undo_Push(pool_##T.head, sizeof(PoolFreeNode), #T " pool head node");

RE_DEFINE_POOL(Regex);
RE_DEFINE_POOL(NoUnionEx);
RE_DEFINE_POOL(Unit);
RE_DEFINE_POOL(Set);
RE_DEFINE_POOL(SetItem);
RE_DEFINE_POOL(Group);

void print_pools() {
	RE_PRINT_POOL(Regex);
	RE_PRINT_POOL(NoUnionEx);
	RE_PRINT_POOL(Unit);
	RE_PRINT_POOL(Set);
	RE_PRINT_POOL(SetItem);
	RE_PRINT_POOL(Group);
}

void pools_push_undo() {
	POOL_PUSH_UNDO(Regex);
	POOL_PUSH_UNDO(NoUnionEx);
	POOL_PUSH_UNDO(Unit);
	POOL_PUSH_UNDO(Set);
	POOL_PUSH_UNDO(SetItem);
	POOL_PUSH_UNDO(Group);
}

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
	UnitContents_init(&unit->Contents);
	unit->RepeatMin = 1;
	unit->RepeatMax = 1;
	unit->_minbuf = 1.0f;
	unit->_maxbuf = 1.0f;
	return unit;
}

void UnitContents_init(UnitContents* contents) {
	UnitContents_SetType(contents, RE_CONTENTS_LITCHAR);
}

void LitChar_init(LitChar* c) {
	// nothing for now; maybe give it a default?
}

void MetaChar_init(MetaChar* c) {
	c->_backslash = '\\';
	c->C = 'a';
}

Special* Special_init(Special* special) {
	return special;
}

Set* Set_init(Set* set) {
	set->NumItems = 0;
	set->TextState = DEFAULT_TEXT_INPUT_STATE;
	return set;
}

SetItem* SetItem_init(SetItem* item) {
	item->Type = RE_SETITEM_LITCHAR;
	LitChar_init(&item->LitChar);
	SetItemRange_init(&item->Range);
	return item;
}

void SetItemRange_init(SetItemRange* range) {
	LitChar_init(&range->Min);
	LitChar_init(&range->Max);
}

Group* Group_init(Group* group) {
	group->Regex = Regex_init(RE_NEW(Regex));
	return group;
}

// convenience thingies that need to allocate memory

Unit* Unit_initWithLiteralChar(Unit* unit, char c) {
	Unit_init(unit);
	unit->Contents.LitChar.C = c;

	return unit;
}

void UnitContents_SetType(UnitContents* contents, int type) {
	contents->Type = type;
	switch (type) {
		case RE_CONTENTS_LITCHAR: {
			LitChar_init(&contents->LitChar);
		} break;
		case RE_CONTENTS_METACHAR: {
			MetaChar_init(&contents->MetaChar);
		} break;
		case RE_CONTENTS_SPECIAL: {
			Special_init(&contents->Special);
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

void SetItem_SetType(SetItem* item, int type) {
	item->Type = type;
	switch (type) {
		case RE_SETITEM_LITCHAR: {
			LitChar_init(&item->LitChar);
		} break;
		case RE_SETITEM_METACHAR: {
			MetaChar_init(&item->MetaChar);
		} break;
	}
}

void Regex_delete(Regex* regex) {
	for (int i = 0; i < regex->NumUnionMembers; i++) {
		NoUnionEx_delete(regex->UnionMembers[i]);
	}

	RE_FREE(Regex, regex);
}

void NoUnionEx_delete(NoUnionEx* ex) {
	for (int i = 0; i < ex->NumUnits; i++) {
		Unit_delete(ex->Units[i]);
	}

	RE_FREE(NoUnionEx, ex);
}

void Unit_delete(Unit* unit) {
	UnitContents* contents = &unit->Contents;
	if (contents->Set) {
		Set_delete(contents->Set);
	}
	if (contents->Group) {
		Group_delete(contents->Group);
	}

	RE_FREE(Unit, unit);
}

void Set_delete(Set* set) {
	for (int i = 0; i < set->NumItems; i++) {
		SetItem_delete(set->Items[i]);
	}

	RE_FREE(Set, set);
}

void SetItem_delete(SetItem* item) {
	RE_FREE(SetItem, item);
}

void Group_delete(Group* group) {
	Regex_delete(group->Regex);

	RE_FREE(Group, group);
}

