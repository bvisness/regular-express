#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "microui.h"

#define MAIN_IMPL
#include "drag.h"
#include "draw.h"
#include "globals.h"
#include "prepass.h"
#include "undo.h"

#include "regex/alloc.h"
#include "regex/parser.h"
#include "regex/pool.h"
#include "regex/regex.h"
#include "regex/tree.h"
#include "regex/vec.h"

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

void blur() {
	mu_input_clear(ctx);
}

char initialRegexBuf[2048];

Regex* regex;

int mu_auto_width_button(mu_Context* ctx, const char* str, int padding) {
	mu_layout_width(ctx, measureText(str, strlen(str)) + (2 * padding));
	return mu_button(ctx, str);
}

void insertNewUnit(NoUnionEx* ex, Unit* unit) {
	NoUnionEx_RemoveSelection(ex);
	NoUnionEx_AddUnit(ex, unit, ex->TextState.InsertIndex);
	ex->TextState = TextState_BumpCursor(ex->TextState, 1, 0);
	mu_set_focus(ctx, NoUnionEx_GetID(ex));
}

void init() {
	regex = parse(initialRegexBuf);
	NoUnionEx* ex = regex->UnionMembers[0];

	ctx = malloc(sizeof(mu_Context));
	mu_init(ctx);
	ctx->text_width = text_width;
  	ctx->text_height = text_height;
  	// ctx->style->padding = 0;
  	ctx->style->spacing = 6;

  	mu_set_focus(ctx, NoUnionEx_GetID(ex));
  	ex->TextState = TextState_SetInsertIndex(ex->TextState, ex->NumUnits, 0);

  	Undo_Reset();
}

int frame(int width, int height, int contentWidth, float dt) {
	mu_begin(ctx, dt);

	const int WINDOW_PADDING = 10;
	const int TOOLBAR_HEIGHT = 40;

	if (
		ctx->key_down & MU_KEY_CTRL
		&& ctx->key_down & MU_KEY_SHIFT
		&& ctx->input_text[0] == 'Z' // TODO: It is so awkward to have to deal with keys being capitalized when shift is pressed.
	) {
		ctx->input_text[0] = 0;
		Undo_Redo();
		// PrintUndoData();
		// print_pools();
	} else if (
		ctx->key_down & MU_KEY_CTRL
		&& ctx->input_text[0] == 'z'
	) {
		ctx->input_text[0] = 0;
		Undo_Undo();
		// PrintUndoData();
		// print_pools();
	}

	UNDOPUSH(ctx->focus);
	Regex_PushUndo(regex);

	prepass_Regex(regex, NULL, NULL);
	prepass_NoUnionEx(&moveUnitsEx, NULL, NULL, NULL);

	int guiHeight = height - WINDOW_PADDING - TOOLBAR_HEIGHT;
	int windowX = 0;
	int windowY = TOOLBAR_HEIGHT;

	if (mu_begin_window_ex(ctx, "UI", mu_rect(windowX, windowY, width, guiHeight), MU_OPT_NOFRAME | MU_OPT_NOTITLE)) {
		drawRailroad_Regex(
			regex,
			(Vec2i) {
				.x = windowX + width/2 - regex->Size.x/2,
				.y = windowY + WINDOW_PADDING + guiHeight/2 - regex->Size.y/2,
			},
			0
		);

		if (ctx->mouse_down & MU_MOUSE_LEFT) {
			if (drag.Type == DRAG_TYPE_WIRE) {
				// draw preview
				int offsetX = ctx->mouse_pos.x - drag.Wire.WireStartPos.x;
				int dy = -iclamp(1/1.618f * offsetX, -40, 40);

				drawConnector(
					drag.Wire.WireStartPos.x, drag.Wire.WireStartPos.y,
					10,
					dy,
					offsetX,
					ctx->mouse_pos.y - (drag.Wire.WireStartPos.y + dy),
					0
				);
			} else if (drag.Type == DRAG_TYPE_BOX_SELECT) {
				mu_set_focus(ctx, 0);

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
				// TODO: Fix this crap
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

	int toolbarX = (width - contentWidth) / 2;

	if (mu_begin_window_ex(ctx, "Toolbar", mu_rect(toolbarX, 0, width, TOOLBAR_HEIGHT), MU_OPT_NOFRAME | MU_OPT_NOTITLE | MU_OPT_NOSCROLL)) {
		mu_layout_row(ctx, 0, NULL, -1);

		if (mu_auto_width_button(ctx, "Character Set", 10)) {
			Unit* newUnit = Unit_init(RE_NEW(Unit));
			UnitContents_SetType(&newUnit->Contents, RE_CONTENTS_SET);
			Set* newSet = newUnit->Contents.Set;

			SetItem* lowercase = SetItem_init(RE_NEW(SetItem));
			SetItem_MakeRange(lowercase, 'a', 'z');
			Set_AddItem(newSet, lowercase, -1);

			NoUnionEx_RemoveSelection(lastFocusedEx);
			NoUnionEx_AddUnit(lastFocusedEx, newUnit, lastFocusedEx->TextState.InsertIndex);
			lastFocusedEx->TextState = TextState_BumpCursor(lastFocusedEx->TextState, 1, 0);

			newSet->TextState = TextState_SetInsertIndex(newSet->TextState, 1, 0);
			mu_set_focus(ctx, Set_GetID(newSet));
		}

		if (mu_auto_width_button(ctx, "Group", 10)) {
			Unit* newUnit = Unit_init(RE_NEW(Unit));
			UnitContents_SetType(&newUnit->Contents, RE_CONTENTS_GROUP);

			NoUnionEx_RemoveSelection(lastFocusedEx);
			NoUnionEx_AddUnit(lastFocusedEx, newUnit, lastFocusedEx->TextState.InsertIndex);
			lastFocusedEx->TextState = TextState_BumpCursor(lastFocusedEx->TextState, 1, 0);

			mu_set_focus(ctx, NoUnionEx_GetID(newUnit->Contents.Group->Regex->UnionMembers[0]));
		}

		if (mu_auto_width_button(ctx, "Special Character", 10)) {
			mu_open_popup(ctx, "Specials");
		}

		if (mu_auto_width_button(ctx, "Common Sets", 10)) {
			mu_open_popup(ctx, "Common Sets");
		}

		if (mu_begin_popup(ctx, "Specials")) {
			int buttonWidth = 130;
			int buttonHeight = 20;

			mu_layout_row(ctx, 1, (int[]){ buttonWidth }, buttonHeight);
			if (mu_button(ctx, "any character")) {
				Unit* newUnit = Unit_init(RE_NEW(Unit));
				UnitContents_SetType(&newUnit->Contents, RE_CONTENTS_SPECIAL);
				newUnit->Contents.Special.Type = RE_SPECIAL_ANY;
				insertNewUnit(lastFocusedEx, newUnit);
			}

			mu_layout_row(ctx, 1, (int[]){ buttonWidth }, buttonHeight);
			if (mu_button(ctx, "start of string")) {
				Unit* newUnit = Unit_init(RE_NEW(Unit));
				UnitContents_SetType(&newUnit->Contents, RE_CONTENTS_SPECIAL);
				newUnit->Contents.Special.Type = RE_SPECIAL_STRINGSTART;
				insertNewUnit(lastFocusedEx, newUnit);
			}

			mu_layout_row(ctx, 1, (int[]){ buttonWidth }, buttonHeight);
			if (mu_button(ctx, "end of string")) {
				Unit* newUnit = Unit_init(RE_NEW(Unit));
				UnitContents_SetType(&newUnit->Contents, RE_CONTENTS_SPECIAL);
				newUnit->Contents.Special.Type = RE_SPECIAL_STRINGEND;
				insertNewUnit(lastFocusedEx, newUnit);
			}

			mu_layout_row(ctx, 1, (int[]){ buttonWidth }, buttonHeight);
			if (mu_button(ctx, "word boundary")) {
				Unit* newUnit = Unit_init(RE_NEW(Unit));
				UnitContents_SetType(&newUnit->Contents, RE_CONTENTS_METACHAR);
				newUnit->Contents.MetaChar.C = 'b';
				insertNewUnit(lastFocusedEx, newUnit);
			}

			mu_layout_row(ctx, 1, (int[]){ buttonWidth }, buttonHeight);
			if (mu_button(ctx, "non-word-boundary")) {
				Unit* newUnit = Unit_init(RE_NEW(Unit));
				UnitContents_SetType(&newUnit->Contents, RE_CONTENTS_METACHAR);
				newUnit->Contents.MetaChar.C = 'B';
				insertNewUnit(lastFocusedEx, newUnit);
			}

			mu_end_popup(ctx);
		}

		if (mu_begin_popup(ctx, "Common Sets")) {
			int buttonWidth = 130;
			int buttonHeight = 20;

			mu_layout_row(ctx, 1, (int[]){ buttonWidth }, buttonHeight);
			if (mu_button(ctx, "digit")) {
				Unit* newUnit = Unit_init(RE_NEW(Unit));
				UnitContents_SetType(&newUnit->Contents, RE_CONTENTS_METACHAR);
				newUnit->Contents.MetaChar.C = 'd';
				insertNewUnit(lastFocusedEx, newUnit);
			}

			mu_layout_row(ctx, 1, (int[]){ buttonWidth }, buttonHeight);
			if (mu_button(ctx, "non-digit")) {
				Unit* newUnit = Unit_init(RE_NEW(Unit));
				UnitContents_SetType(&newUnit->Contents, RE_CONTENTS_METACHAR);
				newUnit->Contents.MetaChar.C = 'D';
				insertNewUnit(lastFocusedEx, newUnit);
			}

			mu_layout_row(ctx, 1, (int[]){ buttonWidth }, buttonHeight);
			if (mu_button(ctx, "whitespace")) {
				Unit* newUnit = Unit_init(RE_NEW(Unit));
				UnitContents_SetType(&newUnit->Contents, RE_CONTENTS_METACHAR);
				newUnit->Contents.MetaChar.C = 's';
				insertNewUnit(lastFocusedEx, newUnit);
			}

			mu_layout_row(ctx, 1, (int[]){ buttonWidth }, buttonHeight);
			if (mu_button(ctx, "non-whitespace")) {
				Unit* newUnit = Unit_init(RE_NEW(Unit));
				UnitContents_SetType(&newUnit->Contents, RE_CONTENTS_METACHAR);
				newUnit->Contents.MetaChar.C = 'S';
				insertNewUnit(lastFocusedEx, newUnit);
			}

			mu_layout_row(ctx, 1, (int[]){ buttonWidth }, buttonHeight);
			if (mu_button(ctx, "word character")) {
				Unit* newUnit = Unit_init(RE_NEW(Unit));
				UnitContents_SetType(&newUnit->Contents, RE_CONTENTS_METACHAR);
				newUnit->Contents.MetaChar.C = 'w';
				insertNewUnit(lastFocusedEx, newUnit);
			}

			mu_layout_row(ctx, 1, (int[]){ buttonWidth }, buttonHeight);
			if (mu_button(ctx, "non-word character")) {
				Unit* newUnit = Unit_init(RE_NEW(Unit));
				UnitContents_SetType(&newUnit->Contents, RE_CONTENTS_METACHAR);
				newUnit->Contents.MetaChar.C = 'W';
				insertNewUnit(lastFocusedEx, newUnit);
			}

			mu_end_popup(ctx);
		}

		mu_end_window(ctx);
	}

	char* outputRegex = ToString(regex);
	setOutput(outputRegex);

	// if (mu_begin_window(ctx, "Tree View", mu_rect(WINDOW_PADDING, WINDOW_PADDING + GUI_HEIGHT + WINDOW_PADDING + 80 + WINDOW_PADDING, width - WINDOW_PADDING*2, 300))) {
	// 	doTree(ctx, regex);

	// 	mu_end_window(ctx);
	// }

	// mouse up; end all drags
	if (!(ctx->mouse_down & MU_MOUSE_LEFT)) {
		if (drag.Type == DRAG_TYPE_MOVE_UNITS) {
			// Drop the units back in their original place
			MoveAllUnitsTo(drag.MoveUnits.OriginEx, drag.MoveUnits.OriginIndex);
			mu_set_focus(ctx, NoUnionEx_GetID(drag.MoveUnits.OriginEx));
		} else if (drag.Type == DRAG_TYPE_CREATE_UNION) {
			UnitRange units = drag.CreateUnion.Units;

			Unit* newUnit = ConvertRangeToGroup(units);

			NoUnionEx* newUnionMember = NoUnionEx_init(RE_NEW(NoUnionEx));
			Unit* initialUnit = Unit_init(RE_NEW(Unit));
			NoUnionEx_AddUnit(newUnionMember, initialUnit, -1);
			Regex_AddUnionMember(newUnit->Contents.Group->Regex, newUnionMember, -1);
		}

		drag = (DragContext) {0};

		ctx->animating = 1;
	}

	mu_end(ctx);

	if (!ctx->mouse_down) {
		Undo_Commit();
	}

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
			case MU_COMMAND_LINE: {
				mu_Color color = cmd->line.color;
				canvas_line(cmd->line.x1, cmd->line.y1, cmd->line.x2, cmd->line.y2, color.r, color.g, color.b, color.a, cmd->line.strokeWidth);
				break;
			}
			case MU_COMMAND_ARC: {
				mu_Color color = cmd->arc.color;
				canvas_arc(cmd->arc.x, cmd->arc.y, cmd->arc.radius, cmd->arc.angleStart, cmd->arc.angleEnd, color.r, color.g, color.b, color.a, cmd->arc.strokeWidth);
				break;
			}
		}
    }

    return ctx->animating;
}
