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

  	Undo_Reset();
}

int frame(float dt) {
	mu_begin(ctx, dt);

	const int PAGE_WIDTH = 900;
	const int WINDOW_PADDING = 10;
	const int GUI_HEIGHT = 400;

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
