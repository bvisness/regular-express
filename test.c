#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "microui.h"
#include "regex/regex.h"
#include "regex/pool.h"

#define COLORPARAMS unsigned char r, unsigned char g, unsigned char b, unsigned char a

extern void canvas_clear();
extern void canvas_clip(int x, int y, int w, int h);
extern void canvas_setFillRGB();
extern void canvas_rect(int x, int y, int w, int h, COLORPARAMS);
extern void canvas_text(char* str, int x, int y, COLORPARAMS);
extern void canvas_circle(int x, int y, float radius, COLORPARAMS);

extern int measureText(const char* text, int len);

static int text_width(mu_Font font, const char *text, int len) {
	if (len < 0) {
		len = strlen(text);
	}

	return measureText(text, len);
}

int _textHeight;
static int text_height(mu_Font font) {
	return _textHeight;
}

mu_Context* ctx;

void mouseMove(int x, int y) {
	mu_input_mousemove(ctx, x, y);
}

void mouseDown(int x, int y) {
	mu_input_mousedown(ctx, x, y, MU_MOUSE_LEFT);
}

void mouseUp(int x, int y) {
	mu_input_mouseup(ctx, x, y, MU_MOUSE_LEFT);
}

void keyDown(int key) {
	mu_input_keydown(ctx, key);
}

void keyUp(int key) {
	mu_input_keyup(ctx, key);
}

char textInputBuf[1024];
void textInput() {
	mu_input_text(ctx, textInputBuf);
}

void scroll(int x, int y) {
	mu_input_scroll(ctx, x, y);
}

int fround(float f) {
	if (f < 0) {
		return (int)(f - 0.5f);
	} else {
		return (int)(f + 0.5f);
	}
}

// --------------------------------------------------------------------
// --------------------------------------------------------------------

Regex* regex;

#define POOL_SIZE 256
#define POOL_PARAMS(type) malloc(sizeof(type) * POOL_SIZE), sizeof(type) * POOL_SIZE, sizeof(type)
Pool regexEntityPool;

Regex* Regex_init(Regex* regex);
NoUnionEx* NoUnionEx_init(NoUnionEx* ex);
Unit* Unit_init(Unit* unit);
UnitContents* UnitContents_init(UnitContents* contents);
UnitRepetition* UnitRepetition_init(UnitRepetition* rep);
LitChar* LitChar_init(LitChar* c);
MetaChar* MetaChar_init(MetaChar* c);
Special* Special_init(Special* special);
Set* Set_init(Set* set);
SetItem* SetItem_init(SetItem* item);
SetItemRange* SetItemRange_init(SetItemRange* range);
Group* Group_init(Group* group);

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
	unit->Repetition = UnitRepetition_init((UnitRepetition*) pool_alloc(&regexEntityPool));
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

UnitRepetition* UnitRepetition_init(UnitRepetition* rep) {
	rep->Max = 0;
	rep->_maxbuf = 0.0f;
	return rep;
}

LitChar* LitChar_init(LitChar* c) {
	c->C = 'a';
	return c;
}

MetaChar* MetaChar_init(MetaChar* c) {
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

void init(int textHeight) {
	_textHeight = textHeight;

	pool_init(&regexEntityPool, POOL_PARAMS(RegexType));
	regex = Regex_init((Regex*) pool_alloc(&regexEntityPool));

	ctx = malloc(sizeof(mu_Context));
	mu_init(ctx);
	ctx->text_width = text_width;
  	ctx->text_height = text_height;
}

void doTree_Regex(Regex* regex);
void doTree_NoUnionEx(NoUnionEx* ex);
void doTree_Unit(Unit* unit);
void doTree_LitChar(LitChar* c);
void doTree_MetaChar(MetaChar* c);
void doTree_Set(Set* set);
void doTree_SetItem(SetItem* item);
void doTree_Group(Group* group);

void doTree(Regex* regex) {
	doTree_Regex(regex);
}

void doTree_Regex(Regex* regex) {
	mu_push_id(ctx, regex, sizeof(Regex));

	if (mu_begin_treenode_ex(ctx, "Union", MU_OPT_EXPANDED)) {
		mu_layout_row(ctx, 2, (int[]) { 20, 20 }, 0);
		if (mu_button(ctx, "-")) {
			if (regex->NumUnionMembers > 0) {
				regex->NumUnionMembers--;
				pool_free(&regexEntityPool, regex->UnionMembers[regex->NumUnionMembers]);
				// TODO: Cascading frees, here and everywhere
			}
		}
		if (mu_button(ctx, "+")) {
			NoUnionEx* ex = NoUnionEx_init((NoUnionEx*) pool_alloc(&regexEntityPool));
			regex->UnionMembers[regex->NumUnionMembers] = ex;
			regex->NumUnionMembers++;
		}

		mu_layout_row(ctx, 1, (int[]) { -1 }, 0);
		for (int i = 0; i < regex->NumUnionMembers; i++) {
			doTree_NoUnionEx(regex->UnionMembers[i]);
		}

		mu_end_treenode(ctx);
	}

	mu_pop_id(ctx);
}

void doTree_NoUnionEx(NoUnionEx* ex) {
	mu_push_id(ctx, ex, sizeof(NoUnionEx));

	if (mu_begin_treenode_ex(ctx, "Units", MU_OPT_EXPANDED)) {
		mu_layout_row(ctx, 2, (int[]) { 20, 20 }, 0);
		if (mu_button(ctx, "-")) {
			if (ex->NumUnits > 0) {
				ex->NumUnits--;
				pool_free(&regexEntityPool, ex->Units[ex->NumUnits]);
			}
		}
		if (mu_button(ctx, "+")) {
			Unit* unit = Unit_init((Unit*) pool_alloc(&regexEntityPool));
			ex->Units[ex->NumUnits] = unit;
			ex->NumUnits++;
		}

		mu_layout_row(ctx, 1, (int[]) { -1 }, 0);
		for (int i = 0; i < ex->NumUnits; i++) {
			doTree_Unit(ex->Units[i]);
		}

		mu_end_treenode(ctx);
	}

	mu_pop_id(ctx);
}

void doTree_Unit(Unit* unit) {
	mu_push_id(ctx, unit, sizeof(Unit));

	if (mu_begin_treenode_ex(ctx, "Unit", MU_OPT_EXPANDED)) {
		if (mu_button(ctx, RE_CONTENTS_ToString(unit->Contents->Type))) {
			mu_open_popup(ctx, "Unit Type");
		}

		if (mu_begin_popup(ctx, "Unit Type")) {
			if (mu_button(ctx, RE_CONTENTS_ToString(RE_CONTENTS_LITCHAR))) unit->Contents->Type = RE_CONTENTS_LITCHAR;
			if (mu_button(ctx, RE_CONTENTS_ToString(RE_CONTENTS_METACHAR))) unit->Contents->Type = RE_CONTENTS_METACHAR;
			if (mu_button(ctx, RE_CONTENTS_ToString(RE_CONTENTS_SPECIAL))) unit->Contents->Type = RE_CONTENTS_SPECIAL;
			if (mu_button(ctx, RE_CONTENTS_ToString(RE_CONTENTS_SET))) unit->Contents->Type = RE_CONTENTS_SET;
			if (mu_button(ctx, RE_CONTENTS_ToString(RE_CONTENTS_GROUP))) unit->Contents->Type = RE_CONTENTS_GROUP;

			// TODO: Initialize other kinds of content types

			mu_end_popup(ctx);
		}

		mu_checkbox(ctx, "Repeats", &unit->Repeats);
		if (unit->Repeats) {
			mu_layout_row(ctx, 4, (int[]) { 40, 40, 40, 40 }, 0);

			mu_label(ctx, "Min:");
			if (unit->Repetition->_minbuf < 0) {
				unit->Repetition->_minbuf = 0.0f;
			}
			mu_number_ex(ctx, &unit->Repetition->_minbuf, 0.02, "%.0f", 0);
			unit->Repetition->Min = fround(unit->Repetition->_minbuf);
			if (unit->Repetition->Min < 0) {
				unit->Repetition->Min = 0;
			}

			mu_label(ctx, "Max:");
			if (unit->Repetition->_maxbuf < 0) {
				unit->Repetition->_maxbuf = 0.0f;
			}
			mu_number_ex(ctx, &unit->Repetition->_maxbuf, 0.02, "%.0f", 0);
			unit->Repetition->Max = fround(unit->Repetition->_maxbuf);
			if (unit->Repetition->Max < 0) {
				unit->Repetition->Max = 0;
			}
			mu_layout_row(ctx, 0, NULL, 0);
		}

		// Handle contents
		switch (unit->Contents->Type) {
			case RE_CONTENTS_LITCHAR: {
				doTree_LitChar(unit->Contents->LitChar);
			} break;
			case RE_CONTENTS_METACHAR: {
				doTree_MetaChar(unit->Contents->MetaChar);
			} break;
			case RE_CONTENTS_SPECIAL: {
				mu_label(ctx, "Special guy!!"); // TODO: You know
			} break;
			case RE_CONTENTS_SET: {
				doTree_Set(unit->Contents->Set);
			} break;
			case RE_CONTENTS_GROUP: {
				doTree_Group(unit->Contents->Group);
			} break;
		}

		mu_end_treenode(ctx);
	}

	mu_pop_id(ctx);
}

void doTree_LitChar(LitChar* c) {
	mu_push_id(ctx, c, sizeof(LitChar));

	mu_layout_row(ctx, 2, (int[]) { 40, -1 }, 0);
	mu_textbox(ctx, c->_buf, sizeof(char) * 2);
	mu_layout_row(ctx, 0, NULL, 0);

	mu_pop_id(ctx);
}

void doTree_MetaChar(MetaChar* c) {
	mu_push_id(ctx, c, sizeof(MetaChar));

	mu_layout_row(ctx, 2, (int[]) { 40, -1 }, 0);
	mu_textbox(ctx, c->_buf, sizeof(char) * 2);
	mu_layout_row(ctx, 0, NULL, 0);

	mu_pop_id(ctx);
}

void doTree_Set(Set* set) {
	mu_push_id(ctx, set, sizeof(Set));

	if (mu_begin_treenode_ex(ctx, "Set", MU_OPT_EXPANDED)) {
		mu_checkbox(ctx, "Negative", &set->IsNegative);

		mu_layout_row(ctx, 2, (int[]) { 20, 20 }, 0);
		if (mu_button(ctx, "-")) {
			if (set->NumItems > 0) {
				set->NumItems--;
				pool_free(&regexEntityPool, set->Items[set->NumItems]);
			}
		}
		if (mu_button(ctx, "+")) {
			SetItem* item = SetItem_init((SetItem*) pool_alloc(&regexEntityPool));
			set->Items[set->NumItems] = item;
			set->NumItems++;
		}

		mu_layout_row(ctx, 1, (int[]) { -1 }, 0);
		for (int i = 0; i < set->NumItems; i++) {
			doTree_SetItem(set->Items[i]);
		}

		mu_end_treenode(ctx);
	}

	mu_pop_id(ctx);
}

void doTree_SetItem(SetItem* item) {
	mu_push_id(ctx, item, sizeof(SetItem));

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
		case RE_SETITEM_LITCHAR: { doTree_LitChar(item->LitChar); } break;
		case RE_SETITEM_RANGE: {
			SetItemRange* range = item->Range;

			mu_layout_row(ctx, 3, (int[]) { 40, 20, 40 }, 0);
			mu_textbox(ctx, range->Min->_buf, sizeof(char) * 2);
			mu_label(ctx, "-");
			mu_textbox(ctx, range->Max->_buf, sizeof(char) * 2);
			mu_layout_row(ctx, 0, NULL, 0);
		} break;
	}

	mu_pop_id(ctx);
}

void doTree_Group(Group* group) {
	mu_push_id(ctx, group, sizeof(Group));

	if (mu_begin_treenode_ex(ctx, "Group", MU_OPT_EXPANDED)) {
		doTree_Regex(group->Regex);

		mu_end_treenode(ctx);
	}

	mu_pop_id(ctx);
}

void frame() {
	mu_begin(ctx);

	if (mu_begin_window(ctx, "Tree View", mu_rect(10, 10, 500, 800))) {
		doTree_Regex(regex);

		mu_end_window(ctx);
	}

	if (mu_begin_window(ctx, "Final Regex", mu_rect(520, 10, 500, 80))) {
		mu_layout_row(ctx, 1, (int[]) { -1 }, -1);
		mu_label(ctx, ToString(regex));

		mu_end_window(ctx);
	}

	mu_end(ctx);

	canvas_clear();
	mu_Command *cmd = NULL;
    while (mu_next_command(ctx, &cmd)) {
		switch (cmd->type) {
			case MU_COMMAND_TEXT: {
				mu_Vec2 pos = cmd->text.pos;
				mu_Color color = cmd->text.color;
				canvas_text(cmd->text.str, pos.x, pos.y, color.r, color.g, color.b, color.a);
				break;
			}
			case MU_COMMAND_RECT: {
				mu_Color color = cmd->rect.color;
				mu_Rect rect = cmd->rect.rect;
				canvas_rect(rect.x, rect.y, rect.w, rect.h, color.r, color.g, color.b, color.a);
				break;
			}
			// case MU_COMMAND_ICON: r_draw_icon(cmd->icon.id, cmd->icon.rect, cmd->icon.color); break;
			case MU_COMMAND_CLIP: {
				mu_Rect rect = cmd->clip.rect;
				canvas_clip(rect.x, rect.y, rect.w, rect.h);
				break;
			}
			case MU_COMMAND_CIRCLE: {
				mu_Color color = cmd->circle.color;
				canvas_circle(cmd->circle.x, cmd->circle.y, cmd->circle.radius, color.r, color.g, color.b, color.a);
				break;
			}
		}
    }
}
