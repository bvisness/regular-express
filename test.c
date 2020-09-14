#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "microui.h"

#include "util/util.h"

#include "regex/alloc.h"
#include "regex/parser.h"
#include "regex/pool.h"
#include "regex/regex.h"
#include "regex/tree.h"
#include "regex/vec.h"

#define COLORPARAMS unsigned char r, unsigned char g, unsigned char b, unsigned char a

extern void canvas_clear();
extern void canvas_clip(int x, int y, int w, int h);
extern void canvas_setFillRGB();
extern void canvas_rect(int x, int y, int w, int h, int radius, COLORPARAMS);
extern void canvas_text(char* str, int x, int y, COLORPARAMS);
extern void canvas_circle(int x, int y, float radius, COLORPARAMS);

extern int measureText(const char* text, int len);

extern void copyText(char* text);

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

void setTextHeight(int textHeight) {
	_textHeight = textHeight;
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

// Utilities

void draw_arbitrary_text(mu_Context* ctx, const char* str, mu_Vec2 pos, mu_Color color) {
	mu_draw_text(ctx, NULL, str, -1, pos, color);
}

// --------------------------------------------------------------------
// --------------------------------------------------------------------

enum {
	DRAG_WIRE_LEFT_HANDLE,
	DRAG_WIRE_RIGHT_HANDLE
};

typedef struct UnitRange {
	NoUnionEx* Ex;
	int Start;
	int End;
} UnitRange;

// TODO: UnitRange Helper
// Make functions for getting the first and last units, for convenience.

enum {
	DRAG_TYPE_NONE,
	DRAG_TYPE_WIRE,
	DRAG_TYPE_BOX_SELECT,
	DRAG_TYPE_MOVE_UNITS,
	DRAG_TYPE_CREATE_UNION,
};

typedef struct DragWire {
	Unit* OriginUnit;
	Unit* UnitBeforeHandle;
	Unit* UnitAfterHandle;

	Vec2i WireStartPos;
	int WhichHandle;
} DragWire;

typedef struct PotentialSelect {
	int Valid;
	UnitRange Range;
} PotentialSelect;

#define MAX_POTENTIAL_SELECTS 16

typedef struct DragBoxSelect {
	Vec2i StartPos;
	mu_Rect Rect;

	PotentialSelect Potentials[MAX_POTENTIAL_SELECTS];
	UnitRange Result;
} DragBoxSelect;

typedef struct DragMoveUnits {
	NoUnionEx* OriginEx;
	int OriginIndex;
} DragMoveUnits;

NoUnionEx moveUnitsEx;

typedef struct DragCreateUnion {
	UnitRange Units;
	Unit* OriginUnit;
	Vec2i WireStartPos;
	int WhichHandle;
} DragCreateUnion;

typedef struct DragContext {
	int Type;

	union {
		DragWire Wire;
		DragBoxSelect BoxSelect;
		DragMoveUnits MoveUnits;
		DragCreateUnion CreateUnion;
	};
} DragContext;

DragContext drag;

void MoveUnitsTo(UnitRange range, NoUnionEx* ex, int startIndex) {
	if (startIndex < 0) {
		startIndex = ex->NumUnits;
	}

	for (int i = 0; i <= range.End - range.Start; i++) {
		Unit* unit = range.Ex->Units[range.Start]; // grab head of range
		NoUnionEx_RemoveUnit(range.Ex, range.Start);
		NoUnionEx_AddUnit(ex, unit, startIndex + i);
	}
}

// TODO: Please move this somewhere else. This is all way too big.
Unit* ConvertRangeToGroup(UnitRange range) {
	Unit* newUnit = Unit_init(RE_NEW(Unit));
	UnitContents_SetType(&newUnit->Contents, RE_CONTENTS_GROUP);

	NoUnionEx* ex = newUnit->Contents.Group->Regex->UnionMembers[0];
	MoveUnitsTo(range, ex, 0);

	NoUnionEx_AddUnit(range.Ex, newUnit, range.Start);

	return newUnit;
}

void MoveAllUnitsTo(NoUnionEx* ex, int index) {
	while (moveUnitsEx.NumUnits > 0) {
		Unit* unit = NoUnionEx_RemoveUnit(&moveUnitsEx, -1);
		NoUnionEx_AddUnit(ex, unit, index);
	}
}

Regex* regex;

void init() {
	regex = Regex_init(RE_NEW(Regex));

	const char* initialString = "Hello";
	for (int i = 0; i < strlen(initialString); i++) {
		Unit* unit = Unit_initWithLiteralChar(RE_NEW(Unit), *(initialString + i));
		NoUnionEx_AddUnit(regex->UnionMembers[0], unit, -1);
	}

	ctx = malloc(sizeof(mu_Context));
	mu_init(ctx);
	ctx->text_width = text_width;
  	ctx->text_height = text_height;
}

const int UNION_VERTICAL_SPACING = 0;
const int UNION_GUTTER_WIDTH = 16;
const int NOUNIONEX_MIN_HEIGHT = 20;
const int UNIT_HANDLE_ZONE_WIDTH = 16;
const int UNIT_WIRE_ATTACHMENT_ZONE_WIDTH = 12;
const int UNIT_REPEAT_WIRE_ZONE_HEIGHT = 15;
const int UNIT_REPEAT_WIRE_MARGIN = 5;
const int UNIT_REPEAT_WIRE_SCOOT = 2;
const int UNIT_CONTENTS_MIN_HEIGHT = 20;
const int UNIT_CONTENTS_LITCHAR_WIDTH = 15;
const int WIRE_THICKNESS = 2;
const int SET_PADDING = 2;
const int SET_HORIZONTAL_SPACING = 2;
const int SET_DASH_WIDTH = 10;
const int GROUP_VERTICAL_PADDING = 0;
const int CURSOR_THICKNESS = 2;
const int CURSOR_VERTICAL_PADDING = 2;

const mu_Color COLOR_RE_TEXT = (mu_Color) { 0, 0, 0, 255 };
const mu_Color COLOR_WIRE = (mu_Color) { 50, 50, 50, 255 };
const mu_Color COLOR_CURSOR = (mu_Color) { 45, 83, 252, 255 };
const mu_Color COLOR_UNIT_BACKGROUND = (mu_Color) { 200, 200, 200, 255 };
const mu_Color COLOR_SPECIAL_BACKGROUND = (mu_Color) { 170, 225, 170, 255 };
const mu_Color COLOR_SELECTED_BACKGROUND = (mu_Color) { 122, 130, 255, 255 };

const char* LEGAL_METACHARS = "dDwWsSbBnt";

void prepass_Regex(Regex* regex, NoUnionEx* parentEx);
void prepass_NoUnionEx(NoUnionEx* ex, Regex* regex, NoUnionEx* parentEx);
void prepass_Unit(Unit* unit, NoUnionEx* ex);
void prepass_UnitContents(UnitContents* contents, NoUnionEx* ex);
void prepass_Set(Set* set, NoUnionEx* ex);
void prepass_Group(Group* group, NoUnionEx* ex);

void prepass_Regex(Regex* regex, NoUnionEx* parentEx) {
	int w = 0;
	int h = 0;
	int wireHeight = 0;

	// delete any empty expressions (after the first one) (that are not focused)
	for (int i = regex->NumUnionMembers-1; i >= 0; i--) {
		NoUnionEx* member = regex->UnionMembers[i];
		if (
			member->NumUnits == 0
			&& regex->NumUnionMembers > 1
			&& ctx->focus != mu_get_id_noidstack(ctx, &member, sizeof(NoUnionEx*))
		) {
			NoUnionEx* deleted = Regex_RemoveUnionMember(regex, i);
			NoUnionEx_delete(deleted);
		}
	}

	// set indexes on all the expressions
	for (int i = 0; i < regex->NumUnionMembers; i++) {
		regex->UnionMembers[i]->Index = i;
	}

	for (int i = 0; i < regex->NumUnionMembers; i++) {
		NoUnionEx* member = regex->UnionMembers[i];
		prepass_NoUnionEx(member, regex, parentEx);

		w = imax(w, member->Size.w);
		h += (i != 0 ? UNION_VERTICAL_SPACING : 0) + member->Size.h;

		if (i == 0) {
			wireHeight = member->WireHeight;
		}
	}

	regex->UnionSize = (Vec2i) {
		.w = w,
		.h = h,
	};
	regex->Size = (Vec2i) {
		.w = UNION_GUTTER_WIDTH + regex->UnionSize.w + UNION_GUTTER_WIDTH,
		.h = regex->UnionSize.h + 30,
	};
	regex->WireHeight = wireHeight;
}

void prepass_NoUnionEx(NoUnionEx* ex, Regex* regex, NoUnionEx* parentEx) {
	mu_Id muid = mu_get_id_noidstack(ctx, &ex, sizeof(NoUnionEx*));

	if (ctx->focus == muid) {
		ctx->updated_focus = 1;

		TextEditResult result = (TextEditResult) { .ResultState = ex->TextState };
		int inputTextLength = strlen(ctx->input_text);

		Unit* previousUnit = NULL;
		if (ex->TextState.CursorPosition > 0) {
			previousUnit = ex->Units[ex->TextState.CursorPosition - 1];
		}

		if (ctx->key_pressed & MU_KEY_BACKSPACE
				&& !TextState_IsSelecting(ex->TextState)
				&& previousUnit
				&& previousUnit->Contents.Type == RE_CONTENTS_METACHAR
		) {
			char c = previousUnit->Contents.MetaChar.C;

			Unit* deletedUnit = NoUnionEx_RemoveUnit(ex, previousUnit->Index);
			Unit_delete(deletedUnit);
			ex->TextState.CursorPosition--;

			Unit* backslash = Unit_initWithLiteralChar(RE_NEW(Unit), '\\');
			NoUnionEx_AddUnit(ex, backslash, ex->TextState.CursorPosition);
			ex->TextState.CursorPosition++;

			Unit* character = Unit_initWithLiteralChar(RE_NEW(Unit), c);
			NoUnionEx_AddUnit(ex, character, ex->TextState.CursorPosition);
			ex->TextState.CursorPosition++;
		} else if (ctx->key_pressed & MU_KEY_BACKSPACE
				&& !TextState_IsSelecting(ex->TextState)
				&& ex->TextState.CursorPosition == 0
		) {
			NoUnionEx* previousEx = regex->UnionMembers[ex->Index - 1];
			int index = previousEx->NumUnits;

			MoveUnitsTo(
				(UnitRange) {
					.Ex = ex,
					.Start = 0,
					.End = ex->NumUnits - 1,
				},
				previousEx,
				index
			);
			ctx->focus = mu_get_id_noidstack(ctx, &previousEx, sizeof(NoUnionEx*));
			previousEx->TextState.CursorPosition = index;
		} else if (inputTextLength > 1) {
			// assume we are pasting and want to parse a regex
			// TODO: We should probably explicitly detect that we are pasting.
			Regex* parseResult = parse(ctx->input_text);

			if (parseResult->NumUnionMembers == 1) {
				// can insert all units inline
				NoUnionEx* src = parseResult->UnionMembers[0];
				while (src->NumUnits > 0) {
					Unit* unit = NoUnionEx_RemoveUnit(src, 0);
					NoUnionEx_AddUnit(ex, unit, ex->TextState.CursorPosition);
					ex->TextState.CursorPosition++;
				}
			} else {
				// must create a group
				Unit* newUnit = Unit_init(RE_NEW(Unit));
				UnitContents_SetType(&newUnit->Contents, RE_CONTENTS_GROUP);
				newUnit->Contents.Group->Regex = parseResult;

				NoUnionEx_AddUnit(ex, newUnit, ex->TextState.CursorPosition);
			}

			Regex_delete(parseResult);
		} else {
			result = StandardTextInput(ctx, ex->TextState, ex->NumUnits);
		}

		if (result.DoDelete) {
			result.DeleteMin = iclamp(result.DeleteMin, 0, ex->NumUnits);
			result.DeleteMax = iclamp(result.DeleteMax, 0, ex->NumUnits);

			for (int i = 0; i < result.DeleteMax - result.DeleteMin; i++) {
				Unit* deleted = NoUnionEx_RemoveUnit(ex, result.DeleteMin);
				Unit_delete(deleted);
			}
		}

		ex->TextState = result.ResultState;

		if (result.DoInput && inputTextLength == 1) {
			Unit* newUnit = NULL;
			if (ctx->key_down & MU_KEY_ALT && ctx->input_text[0] == '[') {
				newUnit = Unit_init(RE_NEW(Unit));
				UnitContents_SetType(&newUnit->Contents, RE_CONTENTS_SET);
				mu_set_focus(ctx, mu_get_id_noidstack(ctx, &newUnit->Contents.Set, sizeof(Set*)));
			} else if (ctx->key_down & MU_KEY_ALT && ctx->input_text[0] == '9') {
				newUnit = Unit_init(RE_NEW(Unit));
				UnitContents_SetType(&newUnit->Contents, RE_CONTENTS_GROUP);
				mu_set_focus(ctx, mu_get_id_noidstack(ctx, &newUnit->Contents.Group->Regex->UnionMembers[0], sizeof(NoUnionEx*)));
			} else if (ctx->key_down & MU_KEY_ALT && ctx->input_text[0] == '0') {
				if (parentEx) {
					mu_set_focus(ctx, mu_get_id_noidstack(ctx, &parentEx, sizeof(NoUnionEx*)));
				}
			} else if (ctx->key_down & MU_KEY_ALT && ctx->input_text[0] == '6') {
				// caret
				newUnit = Unit_init(RE_NEW(Unit));
				UnitContents_SetType(&newUnit->Contents, RE_CONTENTS_SPECIAL);
				newUnit->Contents.Special->Type = RE_SPECIAL_STRINGSTART;
			} else if (ctx->key_down & MU_KEY_ALT && ctx->input_text[0] == '4') {
				// dollar sign
				newUnit = Unit_init(RE_NEW(Unit));
				UnitContents_SetType(&newUnit->Contents, RE_CONTENTS_SPECIAL);
				newUnit->Contents.Special->Type = RE_SPECIAL_STRINGEND;
			} else if (ctx->key_down & MU_KEY_ALT && ctx->input_text[0] == '.') {
				newUnit = Unit_init(RE_NEW(Unit));
				UnitContents_SetType(&newUnit->Contents, RE_CONTENTS_SPECIAL);
				newUnit->Contents.Special->Type = RE_SPECIAL_ANY;
			} else if (ctx->key_down & MU_KEY_ALT && ctx->input_text[0] == '/') {
				// question mark
				Unit_SetRepeatMin(previousUnit, 0);
				Unit_SetRepeatMax(previousUnit, 1);
			} else if (ctx->key_down & MU_KEY_ALT && ctx->input_text[0] == '=') {
				// plus
				Unit_SetRepeatMin(previousUnit, 1);
				Unit_SetRepeatMax(previousUnit, -1);
			} else if (ctx->key_down & MU_KEY_ALT && ctx->input_text[0] == '8') {
				// asterisk
				Unit_SetRepeatMin(previousUnit, 0);
				Unit_SetRepeatMax(previousUnit, -1);
			} else if (ctx->key_down & MU_KEY_ALT && ctx->input_text[0] == '\\') {
				// pipe
				NoUnionEx* newEx = NoUnionEx_init(RE_NEW(NoUnionEx));
				MoveUnitsTo(
					(UnitRange) {
						.Ex = ex,
						.Start = ex->TextState.CursorPosition,
						.End = ex->NumUnits - 1,
					},
					newEx,
					0
				);
				Regex_AddUnionMember(regex, newEx, ex->Index + 1);

				ctx->focus = mu_get_id_noidstack(ctx, &newEx, sizeof(NoUnionEx*));
			} else if (previousUnit
						&& previousUnit->Contents.Type == RE_CONTENTS_LITCHAR
						&& previousUnit->Contents.LitChar.C == '\\'
						&& strchr(LEGAL_METACHARS, ctx->input_text[0])
			) {
				// previous unit is a slash, do a metachar
				Unit* deletedUnit = NoUnionEx_RemoveUnit(ex, previousUnit->Index);
				Unit_delete(deletedUnit);
				ex->TextState.CursorPosition--;

				newUnit = Unit_init(RE_NEW(Unit));
				UnitContents_SetType(&newUnit->Contents, RE_CONTENTS_METACHAR);
				newUnit->Contents.MetaChar.C = ctx->input_text[0];
			} else {
				newUnit = Unit_initWithLiteralChar(RE_NEW(Unit), ctx->input_text[0]);
			}

			if (newUnit) {
				NoUnionEx_AddUnit(ex, newUnit, ex->TextState.CursorPosition);
				ex->TextState.CursorPosition++;
			}

			ctx->input_text[0] = 0;
		}

		// fix up cursor position
		ex->TextState.CursorPosition = iclamp(ex->TextState.CursorPosition, 0, ex->NumUnits);
	}

	// calculate sizes
	int w = 0;
	int h = NOUNIONEX_MIN_HEIGHT;
	int wireHeight = NOUNIONEX_MIN_HEIGHT/2;

	for (int i = 0; i < ex->NumUnits; i++) {
		Unit* unit = ex->Units[i];
		prepass_Unit(unit, ex);

		w += unit->Size.w;
		h = imax(h, unit->Size.h);
		wireHeight = imax(wireHeight, unit->WireHeight);

		unit->Parent = ex;

		if (unit->Index != i) {
			printf("WARNING! Unit %p had index %d, expected %d.", unit, unit->Index, i);
		}
		unit->Index = i;
	}

	ex->Size = (Vec2i) { .w = w, .h = h };
	ex->WireHeight = wireHeight;
}

void prepass_Unit(Unit* unit, NoUnionEx* ex) {
	UnitContents* contents = &unit->Contents;
	prepass_UnitContents(contents, ex);

	int attachmentWidth = Unit_IsNonSingular(unit) ? UNIT_WIRE_ATTACHMENT_ZONE_WIDTH : 0;

	unit->Size = (Vec2i) {
		.w = unit->LeftHandleZoneWidth
			+ attachmentWidth
			+ contents->Size.w
			+ attachmentWidth
			+ unit->RightHandleZoneWidth,
		.h = UNIT_REPEAT_WIRE_ZONE_HEIGHT + contents->Size.h + UNIT_REPEAT_WIRE_ZONE_HEIGHT,
	};
	unit->WireHeight = UNIT_REPEAT_WIRE_ZONE_HEIGHT + contents->WireHeight;
}

void prepass_UnitContents(UnitContents* contents, NoUnionEx* ex) {
	switch (contents->Type) {
		case RE_CONTENTS_LITCHAR: {
			contents->Size = (Vec2i) { .w = UNIT_CONTENTS_LITCHAR_WIDTH, .h = UNIT_CONTENTS_MIN_HEIGHT };
			contents->WireHeight = UNIT_CONTENTS_MIN_HEIGHT/2;
		} break;
		case RE_CONTENTS_METACHAR: {
			contents->Size = (Vec2i) { .w = 25, .h = UNIT_CONTENTS_MIN_HEIGHT };
			contents->WireHeight = UNIT_CONTENTS_MIN_HEIGHT/2;
		} break;
		case RE_CONTENTS_SPECIAL: {
			const char* str = Special_GetHumanString(contents->Special);
			int width = measureText(str, strlen(str));

			contents->Size = (Vec2i) { .w = width + 20, .h = UNIT_CONTENTS_MIN_HEIGHT };
			contents->WireHeight = UNIT_CONTENTS_MIN_HEIGHT/2;
		} break;
		case RE_CONTENTS_SET: {
			Set* set = contents->Set;

			prepass_Set(set, ex);

			// calculate sizes
			Vec2i contentsSize = {0};

			for (int i = 0; i < set->NumItems; i++) {
				SetItem* item = set->Items[i];

				if (item->Type == RE_SETITEM_LITCHAR) {
					item->Size = (Vec2i) { .w = UNIT_CONTENTS_LITCHAR_WIDTH, .h = UNIT_CONTENTS_MIN_HEIGHT };
				} else if (item->Type == RE_SETITEM_RANGE) {
					item->Size = (Vec2i) {
						.w = UNIT_CONTENTS_LITCHAR_WIDTH + SET_DASH_WIDTH + UNIT_CONTENTS_LITCHAR_WIDTH,
						.h = UNIT_CONTENTS_MIN_HEIGHT,
					};
				}

				contentsSize.w += (i > 0 ? SET_HORIZONTAL_SPACING : 0) + item->Size.w;
				contentsSize.h = imax(contentsSize.h, item->Size.h);
			}

			contents->Size = (Vec2i) {
				.w = SET_PADDING + contentsSize.w + SET_PADDING,
				.h = SET_PADDING + contentsSize.h + SET_PADDING,
			};
			contents->WireHeight = UNIT_CONTENTS_MIN_HEIGHT/2;
		} break;
		case RE_CONTENTS_GROUP: {
			Group* group = contents->Group;
			prepass_Group(group, ex);
			contents->Size = group->Size;
			contents->WireHeight = group->WireHeight;
		}
	}
}

void prepass_Set(Set* set, NoUnionEx* ex) {
	mu_Id muid = mu_get_id_noidstack(ctx, &set, sizeof(Set*));

	if (ctx->focus == muid) {
		// Handle keyboard input to set up for next frame
		TextEditResult result = {0};
		int inputTextLength = strlen(ctx->input_text);
		SetItem* itemBeforeCursor = (set->TextState.CursorPosition > 0
			? set->Items[set->TextState.CursorPosition - 1]
			: NULL
		);

		if (ctx->key_pressed & MU_KEY_BACKSPACE
				&& itemBeforeCursor
				&& itemBeforeCursor->Type == RE_SETITEM_RANGE
				&& itemBeforeCursor->Range.Max.C != 0
		) {
			itemBeforeCursor->Range.Max.C = 0;
		} else if (ctx->key_pressed & MU_KEY_BACKSPACE
				&& itemBeforeCursor
				&& itemBeforeCursor->Type == RE_SETITEM_RANGE
				&& itemBeforeCursor->Range.Max.C == 0
		) {
			itemBeforeCursor->Type = RE_SETITEM_LITCHAR;
			itemBeforeCursor->LitChar.C = itemBeforeCursor->Range.Min.C;

			SetItem* newItem = SetItem_init(RE_NEW(SetItem));
			newItem->LitChar.C = '-';
			Set_AddItem(set, newItem, set->TextState.CursorPosition);
			set->TextState.CursorPosition++;
		} else {
			result = StandardTextInput(ctx, set->TextState, set->NumItems);
		}

		if (result.DoDelete) {
			result.DeleteMin = iclamp(result.DeleteMin, 0, set->NumItems);
			result.DeleteMax = iclamp(result.DeleteMax, 0, set->NumItems);

			for (int i = 0; i < result.DeleteMax - result.DeleteMin; i++) {
				Set_RemoveItem(set, result.DeleteMin);
			}
		}

		set->TextState = result.ResultState;

		if (result.DoInput) {
			for (int i = 0; i < inputTextLength; i++) {
				do {
					if (inputTextLength == 1) {
						if (ctx->key_down & MU_KEY_ALT && ctx->input_text[i] == ']') {
							mu_set_focus(ctx, mu_get_id_noidstack(ctx, &ex, sizeof(NoUnionEx*)));
							break;
						}

						if (
							ctx->input_text[i] == '-'
							&& itemBeforeCursor
							&& itemBeforeCursor->Type != RE_SETITEM_RANGE
						) {
							itemBeforeCursor->Type = RE_SETITEM_RANGE;
							itemBeforeCursor->Range.Min.C = itemBeforeCursor->LitChar.C;
							break;
						}

						if (
							itemBeforeCursor
							&& itemBeforeCursor->Type == RE_SETITEM_RANGE
							&& itemBeforeCursor->Range.Max.C == 0
						) {
							itemBeforeCursor->Range.Max.C = ctx->input_text[i];
							break;
						}
					}

					SetItem* newItem = SetItem_init(RE_NEW(SetItem));
					newItem->LitChar.C = ctx->input_text[i];
					Set_AddItem(set, newItem, set->TextState.CursorPosition);
					set->TextState.CursorPosition++;
				} while (0);
			}
		}

		// fix up cursor position
		set->TextState.CursorPosition = iclamp(set->TextState.CursorPosition, 0, set->NumItems);
	}
}

void prepass_Group(Group* group, NoUnionEx* ex) {
	Regex* regex = group->Regex;
	prepass_Regex(regex, ex);

	group->Size = (Vec2i) {
		.w = regex->Size.w,
		.h = GROUP_VERTICAL_PADDING + regex->Size.h + GROUP_VERTICAL_PADDING,
	};
	group->WireHeight = GROUP_VERTICAL_PADDING + regex->WireHeight;
}

void drawRailroad_Regex(Regex* regex, Vec2i origin, int unitDepth);
void drawRailroad_NoUnionEx(NoUnionEx* ex, Vec2i origin, int unitDepth);
void drawRailroad_Unit(Unit* unit, NoUnionEx* parent, Vec2i origin, int depth, UnitRange* selection);
void drawRailroad_UnitContents(UnitContents* contents, Vec2i origin, int unitDepth, int selected);
void drawRailroad_Set(Set* set, Vec2i origin);
void drawRailroad_Group(Group* group, Vec2i origin, int unitDepth, int selected);

void drawRailroad_Regex(Regex* regex, Vec2i origin, int unitDepth) {
	mu_push_id(ctx, &regex, sizeof(Regex*));

	Vec2i memberOrigin = (Vec2i) {
		.x = origin.x + UNION_GUTTER_WIDTH,
		.y = origin.y,
	};

	// TODO: What if the union is empty?

	int wireY = origin.y + regex->WireHeight - WIRE_THICKNESS/2;

	// entry/exit wires (half gutter width)
	mu_draw_rect(
		ctx,
		mu_rect(
			origin.x,
			wireY,
			UNION_GUTTER_WIDTH/2,
			WIRE_THICKNESS
		),
		COLOR_WIRE
	);
	mu_draw_rect(
		ctx,
		mu_rect(
			origin.x + UNION_GUTTER_WIDTH + regex->UnionSize.w + UNION_GUTTER_WIDTH/2,
			wireY,
			UNION_GUTTER_WIDTH/2,
			WIRE_THICKNESS
		),
		COLOR_WIRE
	);

	int finalMemberWireY = wireY;

	// union members
	for (int i = 0; i < regex->NumUnionMembers; i++) {
		NoUnionEx* member = regex->UnionMembers[i];
		drawRailroad_NoUnionEx(member, memberOrigin, unitDepth);

		int memberWireY = memberOrigin.y + member->WireHeight - WIRE_THICKNESS/2;
		mu_draw_rect(
			ctx,
			mu_rect(
				origin.x + UNION_GUTTER_WIDTH/2,
				memberWireY,
				UNION_GUTTER_WIDTH/2,
				WIRE_THICKNESS
			),
			COLOR_WIRE
		);
		mu_draw_rect(
			ctx,
			mu_rect(
				origin.x + UNION_GUTTER_WIDTH + member->Size.w,
				memberWireY,
				regex->UnionSize.w - member->Size.w + UNION_GUTTER_WIDTH/2,
				WIRE_THICKNESS
			),
			COLOR_WIRE
		);

		finalMemberWireY = memberWireY;
		memberOrigin.y += UNION_VERTICAL_SPACING + member->Size.h;
	}

	// vertical connecting wires
	mu_draw_rect(
		ctx,
		mu_rect(
			origin.x + UNION_GUTTER_WIDTH/2 - WIRE_THICKNESS/2,
			wireY,
			WIRE_THICKNESS,
			finalMemberWireY - wireY + WIRE_THICKNESS
		),
		COLOR_WIRE
	);
	mu_draw_rect(
		ctx,
		mu_rect(
			origin.x + UNION_GUTTER_WIDTH + regex->UnionSize.w + UNION_GUTTER_WIDTH/2 - WIRE_THICKNESS/2,
			wireY,
			WIRE_THICKNESS,
			finalMemberWireY - wireY + WIRE_THICKNESS
		),
		COLOR_WIRE
	);

	const int PLUS_BUTTON_WIDTH = 30;
	mu_layout_set_next(ctx, mu_rect(origin.x + regex->Size.x/2 - PLUS_BUTTON_WIDTH/2, memberOrigin.y, PLUS_BUTTON_WIDTH, 20), 0);
	if (mu_button(ctx, "+")) {
		NoUnionEx* newMember = NoUnionEx_init(RE_NEW(NoUnionEx));
		Regex_AddUnionMember(regex, newMember, -1);

		Unit* initialUnit = Unit_init(RE_NEW(Unit));
		NoUnionEx_AddUnit(newMember, initialUnit, -1);
	}

	mu_pop_id(ctx);
}

void drawRailroad_NoUnionEx(NoUnionEx* ex, Vec2i origin, int unitDepth) {
	mu_Id muid = mu_get_id_noidstack(ctx, &ex, sizeof(NoUnionEx*));

	ex->ClickedUnitIndex = -1;

	// check if we were selected by a box select
	if (drag.Type == DRAG_TYPE_BOX_SELECT && drag.BoxSelect.Result.Ex == ex) {
		mu_set_focus(ctx, muid);
		ex->TextState = (TextInputState) {
			.SelectionBase = drag.BoxSelect.Result.Start,
			.CursorPosition = drag.BoxSelect.Result.End + 1,
		};
		drag.BoxSelect.Result = (UnitRange) {0};
	}

	UnitRange selection = {0};
	if (ctx->focus == muid && TextState_IsSelecting(ex->TextState)) {
		selection = (UnitRange) {
			.Ex = ex,
			.Start = TextState_SelectionStart(ex->TextState),
			.End = TextState_SelectionEnd(ex->TextState),
		};
	}

	int unitX = origin.x;
	for (int i = 0; i < ex->NumUnits; i++) {
		Unit* unit = ex->Units[i];

		drawRailroad_Unit(
			unit,
			ex,
			(Vec2i) {
				.x = unitX,
				.y = origin.y + ex->WireHeight - unit->WireHeight,
			},
			unitDepth,
			selection.Ex ? &selection : NULL
		);

		unitX += unit->Size.w;
	}

	if (ex->ClickedUnitIndex != -1) {
		ex->TextState = TextState_SetCursorPosition(ex->TextState, ex->ClickedUnitIndex, ctx->key_down & MU_KEY_SHIFT);
	}

	if (ctx->focus == muid) {
		// draw cursor
		Unit* cursorUnit;
		int cursorRight = 0;
		if (ex->TextState.CursorPosition >= ex->NumUnits) {
			cursorUnit = ex->Units[ex->NumUnits - 1];
			cursorRight = 1;
		} else {
			cursorUnit = ex->Units[ex->TextState.CursorPosition];
		}

		mu_Rect contentsRect = cursorUnit->Contents.LastRect;

		if (cursorRight) {
			mu_draw_rect(
				ctx,
				mu_rect(
					contentsRect.x + contentsRect.w - CURSOR_THICKNESS,
					contentsRect.y + CURSOR_VERTICAL_PADDING,
					CURSOR_THICKNESS,
					contentsRect.h - CURSOR_VERTICAL_PADDING*2
				),
				COLOR_CURSOR
			);
		} else {
			mu_draw_rect(
				ctx,
				mu_rect(
					contentsRect.x,
					contentsRect.y + CURSOR_VERTICAL_PADDING,
					CURSOR_THICKNESS,
					contentsRect.h - CURSOR_VERTICAL_PADDING*2
				),
				COLOR_CURSOR
			);
		}

		// draw floating UI
		if (selection.Ex) {
			// selection bounding box
			// TODO: UnitRange Helper
			mu_Rect sbb = selection.Ex->Units[selection.Start]->LastRect;
			for (int i = selection.Start + 1; i <= selection.End; i++) {
				sbb = rect_union(sbb, selection.Ex->Units[i]->LastRect);
			}

			const int BUTTON_WIDTH = 100;

			mu_layout_set_next(ctx, mu_rect(sbb.x + sbb.w/2 - BUTTON_WIDTH/2, sbb.y - 20, BUTTON_WIDTH, 20), 0);
			if (!drag.Type && mu_button(ctx, "Make Group")) {
				ConvertRangeToGroup(selection);
			}
		}
	}
}

void drawRailroad_Unit(Unit* unit, NoUnionEx* parent, Vec2i origin, int depth, UnitRange* selection) {
	mu_Rect rect = mu_rect(origin.x, origin.y, unit->Size.w, unit->Size.h);
	int isHover = !(drag.Type == DRAG_TYPE_BOX_SELECT) && mu_mouse_over(ctx, rect);
	int isWireDragOrigin = (
		(drag.Type == DRAG_TYPE_WIRE && drag.Wire.OriginUnit == unit)
		|| (drag.Type == DRAG_TYPE_CREATE_UNION && drag.CreateUnion.OriginUnit == unit)
	);
	int nonSingular = Unit_IsNonSingular(unit);
	int isSelected = selection ? TextState_IsSelected(parent->TextState, unit->Index) : 0;

	mu_Rect contentsRect = mu_rect(
		origin.x + unit->LeftHandleZoneWidth + (nonSingular ? UNIT_WIRE_ATTACHMENT_ZONE_WIDTH : 0),
		origin.y + UNIT_REPEAT_WIRE_ZONE_HEIGHT,
		unit->Contents.Size.w,
		unit->Contents.Size.h
	);
	int isContentHover = isHover && mu_mouse_over(ctx, contentsRect);

	// save some of 'em for the next pass
	unit->LastRect = rect;
	unit->IsHover = isHover;
	unit->IsContentHover = isContentHover;
	unit->IsWireDragOrigin = isWireDragOrigin;
	unit->IsSelected = isSelected;

	int middleY = origin.y + UNIT_REPEAT_WIRE_ZONE_HEIGHT + unit->Contents.WireHeight;

	// thru-wires
	mu_draw_rect(
		ctx,
		mu_rect(
			origin.x,
			middleY - WIRE_THICKNESS/2,
			unit->LeftHandleZoneWidth + (nonSingular ? UNIT_WIRE_ATTACHMENT_ZONE_WIDTH : 0),
			WIRE_THICKNESS
		),
		COLOR_WIRE
	);
	mu_draw_rect(
		ctx,
		mu_rect(
			origin.x
				+ unit->LeftHandleZoneWidth
				+ (nonSingular ? UNIT_WIRE_ATTACHMENT_ZONE_WIDTH : 0)
				+ unit->Contents.Size.x,
			middleY - WIRE_THICKNESS/2,
			(nonSingular ? UNIT_WIRE_ATTACHMENT_ZONE_WIDTH : 0) + unit->RightHandleZoneWidth,
			WIRE_THICKNESS
		),
		COLOR_WIRE
	);

	const int HANDLE_SIZE = 8;
	int handleY = middleY - HANDLE_SIZE/2;

	mu_Rect leftHandleRect;
	mu_Rect rightHandleRect;
	int overLeftHandle = 0;
	int overRightHandle = 0;
	int shouldShowLeftHandle = Unit_ShouldShowLeftHandle(unit);
	int shouldShowRightHandle = Unit_ShouldShowRightHandle(unit);

	unit->IsShowingLeftHandle = shouldShowLeftHandle;
	unit->IsShowingRightHandle = shouldShowRightHandle;

	// left handle
	if (shouldShowLeftHandle) {
		int handleX = origin.x + unit->LeftHandleZoneWidth/2 - HANDLE_SIZE/2;
		mu_Rect handleRect = mu_rect(handleX, handleY, HANDLE_SIZE, HANDLE_SIZE);
		mu_draw_rounded_rect(ctx, handleRect, COLOR_WIRE, 3);
		leftHandleRect = handleRect;
		overLeftHandle = mu_mouse_over(ctx, handleRect);
	}
	// right handle
	if (shouldShowRightHandle) {
		int handleX = origin.x
			+ unit->LeftHandleZoneWidth
			+ (nonSingular ? UNIT_WIRE_ATTACHMENT_ZONE_WIDTH : 0)
			+ unit->Contents.Size.x
			+ (nonSingular ? UNIT_WIRE_ATTACHMENT_ZONE_WIDTH : 0)
			+ unit->RightHandleZoneWidth/2
			- HANDLE_SIZE/2;
		mu_Rect handleRect = mu_rect(handleX, handleY, HANDLE_SIZE, HANDLE_SIZE);
		mu_draw_rounded_rect(ctx, handleRect, COLOR_WIRE, 3);
		rightHandleRect = handleRect;
		overRightHandle = mu_mouse_over(ctx, handleRect);
	}

	unit->IsLeftWireHover = mu_mouse_over(ctx, mu_rect(
		origin.x,
		contentsRect.y,
		unit->LeftHandleZoneWidth + (nonSingular ? UNIT_WIRE_ATTACHMENT_ZONE_WIDTH : 0),
		unit->Contents.Size.y
	));
	unit->IsRightWireHover = mu_mouse_over(ctx, mu_rect(
		contentsRect.x + contentsRect.w,
		contentsRect.y,
		(nonSingular ? UNIT_WIRE_ATTACHMENT_ZONE_WIDTH : 0)
			+ unit->RightHandleZoneWidth
			+ (Unit_Next(unit) && Unit_IsNonSingular(Unit_Next(unit)) ? UNIT_WIRE_ATTACHMENT_ZONE_WIDTH : 0),
		unit->Contents.Size.y
	));

	int leftWireX = origin.x
		+ unit->LeftHandleZoneWidth
		+ UNIT_WIRE_ATTACHMENT_ZONE_WIDTH/2
		- WIRE_THICKNESS/2;
	int rightWireX = origin.x
		+ unit->LeftHandleZoneWidth
		+ UNIT_WIRE_ATTACHMENT_ZONE_WIDTH
		+ unit->Contents.Size.x
		+ UNIT_WIRE_ATTACHMENT_ZONE_WIDTH/2
		- WIRE_THICKNESS/2;

	int scoot = (Unit_IsSkip(unit) && Unit_IsRepeat(unit) ? UNIT_REPEAT_WIRE_SCOOT : 0);

	if (unit->RepeatMin < 1) {
		// draw the skip wire
		int skipWireY = middleY
			- unit->Contents.WireHeight
			- UNIT_REPEAT_WIRE_MARGIN
			- scoot
			- WIRE_THICKNESS;

		mu_draw_rect(
			ctx,
			mu_rect(leftWireX - scoot, skipWireY, WIRE_THICKNESS, middleY - skipWireY),
			COLOR_WIRE
		);
		mu_draw_rect(
			ctx,
			mu_rect(rightWireX + scoot, skipWireY, WIRE_THICKNESS, middleY - skipWireY),
			COLOR_WIRE
		);
		mu_draw_rect(
			ctx,
			mu_rect(leftWireX - scoot, skipWireY, rightWireX - leftWireX + scoot*2 + WIRE_THICKNESS, WIRE_THICKNESS),
			COLOR_WIRE
		);
	}

	if (unit->RepeatMax != 1) {
		// draw the repeat wire
		int repeatWireY = middleY
			- unit->Contents.WireHeight
			+ unit->Contents.Size.y
			+ UNIT_REPEAT_WIRE_MARGIN
			- scoot;

		mu_draw_rect(
			ctx,
			mu_rect(leftWireX + scoot, middleY, WIRE_THICKNESS, repeatWireY - middleY),
			COLOR_WIRE
		);
		mu_draw_rect(
			ctx,
			mu_rect(rightWireX - scoot, middleY, WIRE_THICKNESS, repeatWireY - middleY),
			COLOR_WIRE
		);
		mu_draw_rect(
			ctx,
			mu_rect(leftWireX + scoot, repeatWireY, rightWireX - leftWireX - scoot*2 + WIRE_THICKNESS, WIRE_THICKNESS),
			COLOR_WIRE
		);
	}

	drawRailroad_UnitContents(
		&unit->Contents,
		(Vec2i) {
			.x = contentsRect.x,
			.y = contentsRect.y,
		},
		depth,
		isSelected
	);

	int targetLeftHandleZoneWidth = shouldShowLeftHandle ? UNIT_HANDLE_ZONE_WIDTH : 0;
	int targetRightHandleZoneWidth = shouldShowRightHandle ? UNIT_HANDLE_ZONE_WIDTH : 0;

	int animating = 0;
	unit->LeftHandleZoneWidth = interp_linear(ctx->dt, unit->LeftHandleZoneWidth, targetLeftHandleZoneWidth, 160, &animating);
	ctx->animating |= animating;
	unit->RightHandleZoneWidth = interp_linear(ctx->dt, unit->RightHandleZoneWidth, targetRightHandleZoneWidth, 160, &animating);
	ctx->animating |= animating;

	mu_Rect leftHandleZoneRect = mu_rect(
		rect.x,
		rect.y,
		unit->LeftHandleZoneWidth,
		rect.h
	);
	mu_Rect rightHandleZoneRect = mu_rect(
		contentsRect.x + contentsRect.w,
		rect.y,
		unit->RightHandleZoneWidth,
		rect.h
	);

	if (ctx->mouse_released & MU_MOUSE_LEFT && mu_mouse_over(ctx, contentsRect)) {
		if (unit->Contents.Type == RE_CONTENTS_SET) {
			// do nothing, let the set handle the click
		} else {
			ctx->mouse_released &= ~MU_MOUSE_LEFT;
			mu_set_focus(ctx, mu_get_id_noidstack(ctx, &parent, sizeof(NoUnionEx*)));
			parent->ClickedUnitIndex = unit->Index;
		}
	}

	if (ctx->mouse_down == MU_MOUSE_LEFT) {
		if (!drag.Type && ctx->mouse_started_drag) {
			// maybe start drag
			if (overLeftHandle) {
				if (ctx->key_down & MU_KEY_ALT) {
					UnitRange units = (UnitRange) {
						.Ex = parent,
						.Start = unit->Index,
						.End = unit->Index,
					};
					if (selection && selection->Start == units.Start) {
						units = *selection;
					}

					drag = (DragContext) {
						.Type = DRAG_TYPE_CREATE_UNION,
						.CreateUnion = (DragCreateUnion) {
							.Units = units,
							.OriginUnit = unit,
							.WhichHandle = DRAG_WIRE_LEFT_HANDLE,
						},
					};
				} else {
					drag = (DragContext) {
						.Type = DRAG_TYPE_WIRE,
						.Wire = (DragWire) {
							.OriginUnit = unit,
							.UnitBeforeHandle = Unit_Previous(unit),
							.UnitAfterHandle = unit,
							.WhichHandle = DRAG_WIRE_LEFT_HANDLE,
						},
					};
				}
			} else if (overRightHandle) {
				if (ctx->key_down & MU_KEY_ALT) {
					Unit* next = Unit_Next(unit);
					if (next) {
						UnitRange units = (UnitRange) {
							.Ex = parent,
							.Start = next->Index,
							.End = next->Index,
						};
						if (selection && selection->Start == units.Start) {
							units = *selection;
						}

						drag = (DragContext) {
							.Type = DRAG_TYPE_CREATE_UNION,
							.CreateUnion = (DragCreateUnion) {
								.Units = units,
								.OriginUnit = unit,
								.WhichHandle = DRAG_WIRE_RIGHT_HANDLE,
							},
						};
					}
				} else {
					drag = (DragContext) {
						.Type = DRAG_TYPE_WIRE,
						.Wire = (DragWire) {
							.OriginUnit = unit,
							.UnitBeforeHandle = unit,
							.UnitAfterHandle = Unit_Next(unit),
							.WhichHandle = DRAG_WIRE_RIGHT_HANDLE,
						},
					};
				}
			} else if (mu_mouse_over(ctx, contentsRect)) {
				moveUnitsEx = (NoUnionEx) {0};

				UnitRange unitsToMove = (UnitRange) {
					.Ex = parent,
					.Start = unit->Index,
					.End = unit->Index,
				};

				if (isSelected) {
					unitsToMove = *selection;
				}

				MoveUnitsTo(unitsToMove, &moveUnitsEx, 0);

				drag = (DragContext) {
					.Type = DRAG_TYPE_MOVE_UNITS,
					.MoveUnits = (DragMoveUnits) {
						.OriginEx = parent,
						.OriginIndex = unitsToMove.Start,
					},
				};
			}

			if (drag.Type) {
				// consume the mouse input so no other drags start
				ctx->mouse_pressed &= ~MU_MOUSE_LEFT;
			}
		}
	} else if (!(ctx->mouse_down & MU_MOUSE_LEFT) && drag.Type) {
		int didHandleDrag = 0;

		if (drag.Type == DRAG_TYPE_WIRE) {
			// drag finished
			ctx->animating = 1;

			// seek back and forth to figure out what the heck kind of drag this is
			UnitRange groupUnits = {0};
			int isForward = 0; // vs. backward

			// seek left from the drag origin to find repetition
			if (drag.Wire.UnitBeforeHandle) {
				NoUnionEx* ex = drag.Wire.UnitBeforeHandle->Parent;
				for (int i = drag.Wire.UnitBeforeHandle->Index; i >= 0; i--) {
					Unit* visitingUnit = ex->Units[i];

					if (
						(unit == visitingUnit && overLeftHandle)
						|| (Unit_Next(unit) == visitingUnit && overRightHandle)
					) {
						groupUnits = (UnitRange) {
							.Ex = parent,
							.Start = visitingUnit->Index,
							.End = drag.Wire.UnitBeforeHandle->Index,
						};
						isForward = 0;
						break;
					}
				}
			}

			// seek right from the drag origin to find skips
			if (drag.Wire.UnitAfterHandle) {
				NoUnionEx* ex = drag.Wire.UnitAfterHandle->Parent;
				for (int i = drag.Wire.UnitAfterHandle->Index; i < ex->NumUnits; i++) {
					Unit* visitingUnit = ex->Units[i];

					if (
						(unit == visitingUnit && overRightHandle)
						|| (Unit_Previous(unit) == visitingUnit && overLeftHandle)
					) {
						groupUnits = (UnitRange) {
							.Ex = parent,
							.Start = drag.Wire.UnitAfterHandle->Index,
							.End = visitingUnit->Index,
						};
						isForward = 1;
						break;
					}
				}
			}

			if (groupUnits.Ex) {
				didHandleDrag = 1;

				if (groupUnits.Start == groupUnits.End) {
					// drag onto this same unit. no group shenanigans!
					if (isForward) {
						// forward drag, skip
						Unit_SetRepeatMin(groupUnits.Ex->Units[groupUnits.Start], 0);
					} else {
						// backward drag, repeat
						Unit_SetRepeatMax(groupUnits.Ex->Units[groupUnits.Start], 0);
					}
				} else {
					Unit* newUnit = ConvertRangeToGroup(groupUnits);

					if (isForward) {
						Unit_SetRepeatMin(newUnit, 0);
					} else {
						Unit_SetRepeatMax(newUnit, 0);
					}
				}
			}
		} else if (drag.Type == DRAG_TYPE_MOVE_UNITS) {
			if (mu_mouse_over(ctx, leftHandleZoneRect)) {
				didHandleDrag = 1;
				MoveAllUnitsTo(unit->Parent, unit->Index);
			} else if (mu_mouse_over(ctx, rightHandleZoneRect)) {
				didHandleDrag = 1;
				MoveAllUnitsTo(unit->Parent, unit->Index+1);
			}
		}

		if (didHandleDrag) {
			drag = (DragContext) {0};
		}
	}

	if (ctx->mouse_down & MU_MOUSE_LEFT && drag.Type) {
		if (drag.Type == DRAG_TYPE_WIRE && drag.Wire.OriginUnit == unit) {
			if (drag.Wire.WhichHandle == DRAG_WIRE_LEFT_HANDLE) {
				drag.Wire.WireStartPos = (Vec2i) {
					.x = leftHandleRect.x + leftHandleRect.w/2,
					.y = leftHandleRect.y + leftHandleRect.h/2,
				};
			} else if (drag.Wire.WhichHandle == DRAG_WIRE_RIGHT_HANDLE) {
				drag.Wire.WireStartPos = (Vec2i) {
					.x = rightHandleRect.x + rightHandleRect.w/2,
					.y = rightHandleRect.y + rightHandleRect.h/2,
				};
			}
		} else if (drag.Type == DRAG_TYPE_CREATE_UNION && drag.CreateUnion.OriginUnit == unit) {
			if (drag.CreateUnion.WhichHandle == DRAG_WIRE_LEFT_HANDLE) {
				drag.CreateUnion.WireStartPos = (Vec2i) {
					.x = leftHandleRect.x + leftHandleRect.w/2,
					.y = leftHandleRect.y + leftHandleRect.h/2,
				};
			} else if (drag.CreateUnion.WhichHandle == DRAG_WIRE_RIGHT_HANDLE) {
				drag.Wire.WireStartPos = (Vec2i) {
					.x = rightHandleRect.x + rightHandleRect.w/2,
					.y = rightHandleRect.y + rightHandleRect.h/2,
				};
			}
		} else if (drag.Type == DRAG_TYPE_BOX_SELECT) {
			if (
				rect_overlaps(rect, drag.BoxSelect.Rect)
				&& !rect_contains(rect, drag.BoxSelect.Rect)
			) {
				PotentialSelect* potential = &drag.BoxSelect.Potentials[depth];
				if (!potential->Range.Ex) {
					// initialize a selection at this level
					(*potential) = (PotentialSelect) {
						.Valid = 1,
						.Range = (UnitRange) {
							.Ex = parent,
							.Start = unit->Index,
							.End = unit->Index,
						},
					};
				} else {
					// we already have a selection at this level;
					// either continue it or destroy it
					if (Unit_Previous(unit) && potential->Range.End == Unit_Previous(unit)->Index) { // TODO: This check feels very wrong but I don't feel like understanding it right now.
						potential->Range.End = unit->Index;
					} else {
						potential->Valid = 0;
					}
				}
			}
		}
	}
}

void drawRailroad_UnitContents(UnitContents* contents, Vec2i origin, int unitDepth, int selected) {
	mu_Rect r = mu_rect(origin.x, origin.y, contents->Size.w, contents->Size.h);
	contents->LastRect = r;

	mu_Color backgroundColor = selected ? COLOR_SELECTED_BACKGROUND : COLOR_UNIT_BACKGROUND;

	mu_layout_set_next(ctx, r, 0);
	switch (contents->Type) {
		case RE_CONTENTS_LITCHAR: {
			mu_draw_rect(ctx, r, backgroundColor);

			char* str = contents->LitChar._buf;
			mu_Vec2 pos = mu_position_text(ctx, str, mu_layout_next(ctx), NULL, MU_OPT_ALIGNCENTER);
			draw_arbitrary_text(ctx, str, pos, COLOR_RE_TEXT);
		} break;
		case RE_CONTENTS_METACHAR: {
			mu_draw_rect(ctx, r, backgroundColor);

			char* str = &contents->MetaChar._backslash;
			mu_Vec2 pos = mu_position_text(ctx, str, mu_layout_next(ctx), NULL, 0);
			draw_arbitrary_text(ctx, str, pos, COLOR_RE_TEXT);
		} break;
		case RE_CONTENTS_SPECIAL: {
			mu_draw_rect(ctx, r, selected ? backgroundColor : COLOR_SPECIAL_BACKGROUND);

			Special* s = contents->Special;

			const char* str = Special_GetHumanString(s);
			mu_Vec2 pos = mu_position_text(ctx, str, mu_layout_next(ctx), NULL, MU_OPT_ALIGNCENTER);
			draw_arbitrary_text(ctx, str, pos, COLOR_RE_TEXT);
		} break;
		case RE_CONTENTS_SET: {
			// TODO: Set
			mu_draw_rect(ctx, r, backgroundColor);
			drawRailroad_Set(contents->Set, origin);
		} break;
		case RE_CONTENTS_GROUP: {
			drawRailroad_Group(contents->Group, origin, unitDepth, selected);
		} break;
	}
}

void drawRailroad_Set(Set* set, Vec2i origin) {
	mu_Id muid = mu_get_id_noidstack(ctx, &set, sizeof(Set*));

	// TODO: Selections

	int itemX = origin.x + SET_PADDING;
	int itemY = origin.y + SET_PADDING;

	const char* dashStr = "-";

	for (int i = 0; i < set->NumItems; i++) {
		SetItem* item = set->Items[i];

		int selected = TextState_IsSelected(set->TextState, i);

		mu_Rect itemRect = mu_rect(itemX, itemY, item->Size.w, item->Size.h);
		mu_draw_rect(ctx, itemRect, selected ? COLOR_SELECTED_BACKGROUND : mu_color(160, 160, 160, 255));

		if (item->Type == RE_SETITEM_LITCHAR) {
			mu_layout_set_next(ctx, itemRect, 0);
			char* str = item->LitChar._buf;
			mu_Vec2 pos = mu_position_text(ctx, str, mu_layout_next(ctx), NULL, MU_OPT_ALIGNCENTER);
			draw_arbitrary_text(ctx, str, pos, COLOR_RE_TEXT);
		} else if (item->Type == RE_SETITEM_RANGE) {
			{
				mu_Rect r = mu_rect(itemX, itemY, UNIT_CONTENTS_LITCHAR_WIDTH, itemRect.h);
				mu_layout_set_next(ctx, r, 0);
				char* str = item->Range.Min._buf;
				mu_Vec2 pos = mu_position_text(ctx, str, mu_layout_next(ctx), NULL, MU_OPT_ALIGNCENTER);
				draw_arbitrary_text(ctx, str, pos, COLOR_RE_TEXT);
			}

			{
				mu_Rect r = mu_rect(itemX + UNIT_CONTENTS_LITCHAR_WIDTH, itemY, SET_DASH_WIDTH, itemRect.h);
				mu_layout_set_next(ctx, r, 0);
				mu_Vec2 pos = mu_position_text(ctx, dashStr, mu_layout_next(ctx), NULL, MU_OPT_ALIGNCENTER);
				draw_arbitrary_text(ctx, dashStr, pos, COLOR_RE_TEXT);
			}

			{
				mu_Rect r = mu_rect(itemX + UNIT_CONTENTS_LITCHAR_WIDTH + SET_DASH_WIDTH, itemY, UNIT_CONTENTS_LITCHAR_WIDTH, itemRect.h);
				mu_layout_set_next(ctx, r, 0);
				char* str = item->Range.Max._buf;
				mu_Vec2 pos = mu_position_text(ctx, str, mu_layout_next(ctx), NULL, MU_OPT_ALIGNCENTER);
				draw_arbitrary_text(ctx, str, pos, COLOR_RE_TEXT);
			}
		}

		item->LastRect = itemRect;

		itemX += itemRect.w + SET_HORIZONTAL_SPACING;

		if (ctx->mouse_released & MU_MOUSE_LEFT && mu_mouse_over(ctx, itemRect)) {
			ctx->mouse_released &= ~MU_MOUSE_LEFT;
			mu_set_focus(ctx, muid);
			set->TextState = TextState_SetCursorPosition(set->TextState, i, 0); // TODO: Selection
		}
	}

	if (ctx->focus == muid) {
		ctx->updated_focus = 1;

		// Draw cursor according to what we have _now_
		SetItem* cursorItem;
		int cursorRight = 0;
		if (set->TextState.CursorPosition >= set->NumItems) {
			cursorItem = set->Items[set->NumItems - 1];
			cursorRight = 1;
		} else {
			cursorItem = set->Items[set->TextState.CursorPosition];
		}

		mu_Rect itemRect = cursorItem->LastRect;

		if (cursorRight) {
			mu_draw_rect(
				ctx,
				mu_rect(
					itemRect.x + itemRect.w - CURSOR_THICKNESS,
					itemRect.y + CURSOR_VERTICAL_PADDING,
					CURSOR_THICKNESS,
					itemRect.h - CURSOR_VERTICAL_PADDING*2
				),
				COLOR_CURSOR
			);
		} else {
			mu_draw_rect(
				ctx,
				mu_rect(
					itemRect.x,
					itemRect.y + CURSOR_VERTICAL_PADDING,
					CURSOR_THICKNESS,
					itemRect.h - CURSOR_VERTICAL_PADDING*2
				),
				COLOR_CURSOR
			);
		}
	}
}

void drawRailroad_Group(Group* group, Vec2i origin, int unitDepth, int selected) {
	mu_draw_rounded_rect(
		ctx,
		mu_rect(origin.x, origin.y, group->Size.w, group->Size.h),
		selected ? COLOR_SELECTED_BACKGROUND : mu_color(0, 0, 0, 25),
		4
	);

	drawRailroad_Regex(
		group->Regex,
		(Vec2i) {
			.x = origin.x,
			.y = GROUP_VERTICAL_PADDING + origin.y,
		},
		unitDepth + 1
	);
}

void printPools() {
	RE_PRINT_POOL(Regex);
	RE_PRINT_POOL(NoUnionEx);
	RE_PRINT_POOL(Unit);
	RE_PRINT_POOL(MetaChar);
	RE_PRINT_POOL(Special);
	RE_PRINT_POOL(Set);
	RE_PRINT_POOL(SetItem);
	RE_PRINT_POOL(Group);
}

int frame(float dt) {
	mu_begin(ctx, dt);

	const int PAGE_WIDTH = 800;
	const int WINDOW_PADDING = 10;
	const int GUI_HEIGHT = 500;

	prepass_Regex(regex, NULL);
	prepass_NoUnionEx(&moveUnitsEx, NULL, NULL);

	if (mu_begin_window_ex(ctx, "Test", mu_rect(WINDOW_PADDING, WINDOW_PADDING, PAGE_WIDTH - WINDOW_PADDING*2, GUI_HEIGHT), MU_OPT_NOFRAME | MU_OPT_NOTITLE)) {
		drawRailroad_Regex(
			regex,
			(Vec2i) {
				.x = PAGE_WIDTH/2 - regex->Size.x/2,
				.y = WINDOW_PADDING + GUI_HEIGHT/2 - regex->Size.y/2,
			},
			0
		);

		if (ctx->mouse_down & MU_MOUSE_LEFT) {
			if (drag.Type == DRAG_TYPE_WIRE) {
				// draw preview
				int offsetX = ctx->mouse_pos.x - drag.Wire.WireStartPos.x;
				int previewY = drag.Wire.WireStartPos.y - iclamp(1/1.618f * offsetX, -40, 40);
				mu_draw_rect(
					ctx,
					mu_rect(
						imin(ctx->mouse_pos.x, drag.Wire.WireStartPos.x) - WIRE_THICKNESS/2,
						previewY - WIRE_THICKNESS/2,
						WIRE_THICKNESS/2 + iabs(offsetX) + WIRE_THICKNESS/2,
						WIRE_THICKNESS
					),
					COLOR_WIRE
				);
				mu_draw_rect(
					ctx,
					mu_rect(
						drag.Wire.WireStartPos.x - WIRE_THICKNESS/2,
						imin(previewY, drag.Wire.WireStartPos.y) - WIRE_THICKNESS/2,
						WIRE_THICKNESS,
						WIRE_THICKNESS/2 + iabs(previewY - drag.Wire.WireStartPos.y) + WIRE_THICKNESS/2
					),
					COLOR_WIRE
				);
				mu_draw_rect(
					ctx,
					mu_rect(
						ctx->mouse_pos.x - WIRE_THICKNESS/2,
						imin(previewY, ctx->mouse_pos.y) - WIRE_THICKNESS/2,
						WIRE_THICKNESS,
						WIRE_THICKNESS/2 + iabs(previewY - ctx->mouse_pos.y) + WIRE_THICKNESS/2
					),
					COLOR_WIRE
				);
			} else if (drag.Type == DRAG_TYPE_BOX_SELECT) {
				UnitRange newSelection = {0};
				for (int i = 0; i < MAX_POTENTIAL_SELECTS; i++) {
					PotentialSelect* potential = &drag.BoxSelect.Potentials[i];
					if (potential->Valid) {
						newSelection = potential->Range;
						break;
					}
				}

				drag.BoxSelect.Result = newSelection;
				for (int i = 0; i < MAX_POTENTIAL_SELECTS; i++) {
					drag.BoxSelect.Potentials[i] = (PotentialSelect) {0};
				}

				// update select rect for next frame
				drag.BoxSelect.Rect = mu_rect(
					imin(ctx->mouse_pos.x, drag.BoxSelect.StartPos.x),
					imin(ctx->mouse_pos.y, drag.BoxSelect.StartPos.y),
					iabs(ctx->mouse_pos.x - drag.BoxSelect.StartPos.x),
					iabs(ctx->mouse_pos.y - drag.BoxSelect.StartPos.y)
				);

				mu_draw_rect(
					ctx,
					drag.BoxSelect.Rect,
					mu_color(14, 108, 255, 76)
				);
			} else if (drag.Type == DRAG_TYPE_MOVE_UNITS) {
				drawRailroad_NoUnionEx(&moveUnitsEx, (Vec2i) { .x = ctx->mouse_pos.x, .y = ctx->mouse_pos.y }, 0);
			} else if (drag.Type == DRAG_TYPE_CREATE_UNION) {
				mu_draw_rect(
					ctx,
					mu_rect(
						drag.CreateUnion.WireStartPos.x - WIRE_THICKNESS/2,
						imin(ctx->mouse_pos.y, drag.CreateUnion.WireStartPos.y) - WIRE_THICKNESS/2,
						WIRE_THICKNESS,
						WIRE_THICKNESS/2 + iabs(ctx->mouse_pos.y - drag.CreateUnion.WireStartPos.y) + WIRE_THICKNESS/2
					),
					COLOR_WIRE
				);
				mu_draw_rect(
					ctx,
					mu_rect(
						drag.CreateUnion.WireStartPos.x - WIRE_THICKNESS/2,
						ctx->mouse_pos.y - WIRE_THICKNESS/2,
						WIRE_THICKNESS/2 + imax(0, ctx->mouse_pos.x - drag.CreateUnion.WireStartPos.x) + WIRE_THICKNESS/2,
						WIRE_THICKNESS
					),
					COLOR_WIRE
				);
			}
		}

		if (ctx->mouse_down & MU_MOUSE_LEFT && ctx->mouse_started_drag && !drag.Type) {
			// nothing else consumed the mouse click, so start a box select
			drag = (DragContext) {
				.Type = DRAG_TYPE_BOX_SELECT,
				.BoxSelect = (DragBoxSelect) {
					.StartPos = (Vec2i) { .x = ctx->mouse_down_pos.x, .y = ctx->mouse_down_pos.y },
				},
			};
			mu_set_focus(ctx, 0);
		}

		mu_end_window(ctx);
	}

	if (mu_begin_window(ctx, "Final Regex", mu_rect(WINDOW_PADDING, WINDOW_PADDING + GUI_HEIGHT + WINDOW_PADDING, PAGE_WIDTH - WINDOW_PADDING*2, 80))) {
		mu_layout_row(ctx, 2, (int[]) { 500, -10 }, -1);

		char* regexString = ToString(regex);
		mu_label(ctx, regexString);

		if (mu_button(ctx, "Copy")) {
			copyText(regexString);
		}

		mu_end_window(ctx);
	}

	if (mu_begin_window(ctx, "Tree View", mu_rect(WINDOW_PADDING, WINDOW_PADDING + GUI_HEIGHT + WINDOW_PADDING + 80 + WINDOW_PADDING, PAGE_WIDTH - WINDOW_PADDING*2, 300))) {
		doTree(ctx, regex);

		mu_end_window(ctx);
	}

	// mouse up; end all drags
	if (!(ctx->mouse_down & MU_MOUSE_LEFT)) {
		if (drag.Type == DRAG_TYPE_MOVE_UNITS) {
			MoveAllUnitsTo(drag.MoveUnits.OriginEx, drag.MoveUnits.OriginIndex);
		} else if (drag.Type == DRAG_TYPE_CREATE_UNION) {
			UnitRange units = drag.CreateUnion.Units;

			Unit* newUnit = ConvertRangeToGroup(units);

			NoUnionEx* newUnionMember = NoUnionEx_init(RE_NEW(NoUnionEx));
			Unit* initialUnit = Unit_init(RE_NEW(Unit));
			NoUnionEx_AddUnit(newUnionMember, initialUnit, -1);
			Regex_AddUnionMember(newUnit->Contents.Group->Regex, newUnionMember, -1);
		}

		drag = (DragContext) {0};
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
				canvas_rect(rect.x, rect.y, rect.w, rect.h, cmd->rect.radius, color.r, color.g, color.b, color.a);
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

    return ctx->animating;
}
