#include "tree.h"

#include "alloc.h"

void doTree_Regex(mu_Context* ctx, Regex* regex);
void doTree_NoUnionEx(mu_Context* ctx, NoUnionEx* ex);
void doTree_Unit(mu_Context* ctx, Unit* unit);
void doTree_LitChar(mu_Context* ctx, LitChar* c);
void doTree_MetaChar(mu_Context* ctx, MetaChar* c);
void doTree_Set(mu_Context* ctx, Set* set);
void doTree_SetItem(mu_Context* ctx, SetItem* item);
void doTree_Group(mu_Context* ctx, Group* group);

int fround(float f) {
	if (f < 0) {
		return (int)(f - 0.5f);
	} else {
		return (int)(f + 0.5f);
	}
}

void doTree(mu_Context* ctx, Regex* regex) {
	doTree_Regex(ctx, regex);
}

void doTree_Regex(mu_Context* ctx, Regex* regex) {
	mu_push_id(ctx, &regex, sizeof(Regex*));

	if (mu_begin_treenode_ex(ctx, "Union", MU_OPT_EXPANDED)) {
		mu_layout_row(ctx, 2, (int[]) { 20, 20 }, 0);
		if (mu_button(ctx, "-")) {
			if (regex->NumUnionMembers > 0) {
				regex->NumUnionMembers--;
				RE_FREE(NoUnionEx, regex->UnionMembers[regex->NumUnionMembers]);
				// TODO: Cascading frees, here and everywhere
			}
		}
		if (mu_button(ctx, "+")) {
			NoUnionEx* ex = NoUnionEx_init(RE_NEW(NoUnionEx));
			Regex_AddUnionMember(regex, ex, -1);

			Unit* initialUnit = Unit_init(RE_NEW(Unit));
			NoUnionEx_AddUnit(ex, initialUnit, -1);
		}

		mu_layout_row(ctx, 1, (int[]) { -1 }, 0);
		for (int i = 0; i < regex->NumUnionMembers; i++) {
			doTree_NoUnionEx(ctx, regex->UnionMembers[i]);
		}

		mu_end_treenode(ctx);
	}

	mu_pop_id(ctx);
}

void doTree_NoUnionEx(mu_Context* ctx, NoUnionEx* ex) {
	mu_push_id(ctx, &ex, sizeof(NoUnionEx*));

	if (mu_begin_treenode_ex(ctx, "Units", MU_OPT_EXPANDED)) {
		mu_layout_row(ctx, 2, (int[]) { 20, 20 }, 0);
		if (mu_button(ctx, "-")) {
			if (ex->NumUnits > 0) {
				ex->NumUnits--;
				RE_FREE(Unit, ex->Units[ex->NumUnits]);
			}
		}
		if (mu_button(ctx, "+")) {
			Unit* unit = Unit_init(RE_NEW(Unit));
			NoUnionEx_AddUnit(ex, unit, -1);
		}

		mu_layout_row(ctx, 1, (int[]) { -1 }, 0);
		for (int i = 0; i < ex->NumUnits; i++) {
			doTree_Unit(ctx, ex->Units[i]);
		}

		mu_end_treenode(ctx);
	}

	mu_pop_id(ctx);
}

void doTree_Unit(mu_Context* ctx, Unit* unit) {
	mu_push_id(ctx, &unit, sizeof(Unit*));

	if (mu_begin_treenode_ex(ctx, "Unit", MU_OPT_EXPANDED)) {
		if (mu_button(ctx, RE_CONTENTS_ToString(unit->Contents.Type))) {
			mu_open_popup(ctx, "Unit Type");
		}

		if (mu_begin_popup(ctx, "Unit Type")) {
			if (mu_button(ctx, RE_CONTENTS_ToString(RE_CONTENTS_LITCHAR))) {
				UnitContents_SetType(&unit->Contents, RE_CONTENTS_LITCHAR);
			}
			if (mu_button(ctx, RE_CONTENTS_ToString(RE_CONTENTS_METACHAR))) {
				UnitContents_SetType(&unit->Contents, RE_CONTENTS_METACHAR);
			}
			if (mu_button(ctx, RE_CONTENTS_ToString(RE_CONTENTS_SPECIAL))) {
				UnitContents_SetType(&unit->Contents, RE_CONTENTS_SPECIAL);
			}
			if (mu_button(ctx, RE_CONTENTS_ToString(RE_CONTENTS_SET))) {
				UnitContents_SetType(&unit->Contents, RE_CONTENTS_SET);
			}
			if (mu_button(ctx, RE_CONTENTS_ToString(RE_CONTENTS_GROUP))) {
				UnitContents_SetType(&unit->Contents, RE_CONTENTS_GROUP);
			}

			// TODO: Initialize other kinds of content types

			mu_end_popup(ctx);
		}

		mu_layout_row(ctx, 4, (int[]) { 40, 40, 40, 40 }, 0);

		mu_label(ctx, "Min:");
		if (unit->_minbuf < 0) {
			unit->_minbuf = 0.0f;
		}
		mu_number_ex(ctx, &unit->_minbuf, 0.02, "%.0f", 0);
		unit->RepeatMin = fround(unit->_minbuf);
		if (unit->RepeatMin < 0) {
			unit->RepeatMin = 0;
		}

		mu_label(ctx, "Max:");
		if (unit->_maxbuf < 0) {
			unit->_maxbuf = 0.0f;
		}
		mu_number_ex(ctx, &unit->_maxbuf, 0.02, "%.0f", 0);
		unit->RepeatMax = fround(unit->_maxbuf);
		if (unit->RepeatMax < 0) {
			unit->RepeatMax = 0;
		}
		mu_layout_row(ctx, 0, NULL, 0);

		// Handle contents
		switch (unit->Contents.Type) {
			case RE_CONTENTS_LITCHAR: {
				doTree_LitChar(ctx, &unit->Contents.LitChar);
			} break;
			case RE_CONTENTS_METACHAR: {
				doTree_MetaChar(ctx, &unit->Contents.MetaChar);
			} break;
			case RE_CONTENTS_SPECIAL: {
				mu_label(ctx, "Special guy!!"); // TODO: You know
			} break;
			case RE_CONTENTS_SET: {
				doTree_Set(ctx, unit->Contents.Set);
			} break;
			case RE_CONTENTS_GROUP: {
				doTree_Group(ctx, unit->Contents.Group);
			} break;
		}

		mu_end_treenode(ctx);
	}

	mu_pop_id(ctx);
}

void doTree_LitChar(mu_Context* ctx, LitChar* c) {
	mu_push_id(ctx, &c, sizeof(LitChar*));

	mu_layout_row(ctx, 2, (int[]) { 40, -1 }, 0);
	mu_textbox(ctx, c->_buf, sizeof(char) * 2);
	mu_layout_row(ctx, 0, NULL, 0);

	mu_pop_id(ctx);
}

void doTree_MetaChar(mu_Context* ctx, MetaChar* c) {
	mu_push_id(ctx, &c, sizeof(MetaChar*));

	mu_layout_row(ctx, 2, (int[]) { 40, -1 }, 0);
	mu_textbox(ctx, c->_buf, sizeof(char) * 2);
	mu_layout_row(ctx, 0, NULL, 0);

	mu_pop_id(ctx);
}

void doTree_Set(mu_Context* ctx, Set* set) {
	mu_push_id(ctx, &set, sizeof(Set*));

	if (mu_begin_treenode_ex(ctx, "Set", MU_OPT_EXPANDED)) {
		mu_checkbox(ctx, "Negative", &set->IsNegative);

		mu_layout_row(ctx, 2, (int[]) { 20, 20 }, 0);
		if (mu_button(ctx, "-")) {
			if (set->NumItems > 0) {
				set->NumItems--;
				RE_FREE(SetItem, set->Items[set->NumItems]);
			}
		}
		if (mu_button(ctx, "+")) {
			SetItem* item = SetItem_init(RE_NEW(SetItem));
			set->Items[set->NumItems] = item;
			set->NumItems++;
		}

		mu_layout_row(ctx, 1, (int[]) { -1 }, 0);
		for (int i = 0; i < set->NumItems; i++) {
			doTree_SetItem(ctx, set->Items[i]);
		}

		mu_end_treenode(ctx);
	}

	mu_pop_id(ctx);
}

void doTree_SetItem(mu_Context* ctx, SetItem* item) {
	mu_push_id(ctx, &item, sizeof(SetItem*));

	mu_layout_row(ctx, 0, NULL, 0);
	if (mu_button(ctx, RE_SETITEM_ToString(item->Type))) {
		mu_open_popup(ctx, "Set Item Type");
	}
	if (mu_begin_popup(ctx, "Set Item Type")) {
		if (mu_button(ctx, RE_SETITEM_ToString(RE_SETITEM_LITCHAR))) item->Type = RE_SETITEM_LITCHAR;
		if (mu_button(ctx, RE_SETITEM_ToString(RE_SETITEM_RANGE))) item->Type = RE_SETITEM_RANGE;

		// TODO: Initialize other kinds of item types?

		mu_end_popup(ctx);
	}

	switch (item->Type) {
		case RE_SETITEM_LITCHAR: { doTree_LitChar(ctx, &item->LitChar); } break;
		case RE_SETITEM_RANGE: {
			SetItemRange* range = &item->Range;

			mu_layout_row(ctx, 3, (int[]) { 40, 20, 40 }, 0);
			mu_textbox(ctx, range->Min._buf, sizeof(char) * 2);
			mu_label(ctx, "-");
			mu_textbox(ctx, range->Max._buf, sizeof(char) * 2);
			mu_layout_row(ctx, 0, NULL, 0);
		} break;
	}

	mu_pop_id(ctx);
}

void doTree_Group(mu_Context* ctx, Group* group) {
	mu_push_id(ctx, &group, sizeof(Group*));

	if (mu_begin_treenode_ex(ctx, "Group", MU_OPT_EXPANDED)) {
		doTree_Regex(ctx, group->Regex);

		mu_end_treenode(ctx);
	}

	mu_pop_id(ctx);
}
