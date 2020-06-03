#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "microui.h"

#include "util/util.h"

#include "regex/alloc.h"
#include "regex/pool.h"
#include "regex/regex.h"
#include "regex/tree.h"
#include "regex/vec.h"

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

// --------------------------------------------------------------------
// --------------------------------------------------------------------

typedef struct DragContext {
	Unit* OriginUnit;

	Unit* HandleBeforeUnit;
	Unit* HandleAfterUnit;
} DragContext;

DragContext drag;

Regex* regex;

void init() {
	regex = Regex_init((Regex*) pool_alloc(getRegexPool()));

	ctx = malloc(sizeof(mu_Context));
	mu_init(ctx);
	ctx->text_width = text_width;
  	ctx->text_height = text_height;
}

const int UNION_VERTICAL_SPACING = 10;
const mu_Color COLOR_WIRE = (mu_Color) { 50, 50, 50, 255 };

void prepass_Regex(Regex* regex);
void prepass_NoUnionEx(NoUnionEx* ex);
void prepass_Unit(Unit* unit);
void prepass_UnitContents(UnitContents* contents);
void prepass_Group(Group* group);

void prepass_Regex(Regex* regex) {
	int w = 0;
	int h = 0;
	for (int i = 0; i < regex->NumUnionMembers; i++) {
		NoUnionEx* member = regex->UnionMembers[i];
		prepass_NoUnionEx(member);

		w = imax(w, member->Size.w);
		h += (i != 0 ? UNION_VERTICAL_SPACING : 0) + member->Size.h;
	}

	regex->Size = (Vec2i) { .w = w, .h = h };
}

void prepass_NoUnionEx(NoUnionEx* ex) {
	int w = 0;
	int h = 0;

	for (int j = 0; j < ex->NumUnits; j++) {
		Unit* unit = ex->Units[j];
		prepass_Unit(unit);

		w += unit->Size.w;
		h = imax(h, unit->Size.h);

		unit->Parent = ex;
		unit->Previous = (j == 0) ? NULL : ex->Units[j-1];
		unit->Next = (j == ex->NumUnits - 1) ? NULL : ex->Units[j+1];
	}

	ex->Size = (Vec2i) { .w = w, .h = h };
}

void prepass_Unit(Unit* unit) {
	UnitContents* contents = unit->Contents;
	prepass_UnitContents(contents);

	unit->Size = (Vec2i) {
		.w = unit->LeftSpacing + contents->Size.w + unit->RightSpacing,
		.h = contents->Size.h,
	};
}

void prepass_UnitContents(UnitContents* contents) {
	switch (contents->Type) {
		case RE_CONTENTS_LITCHAR: {
			contents->Size = (Vec2i) { .w = 15, .h = 20 };
		} break;
		case RE_CONTENTS_METACHAR: {
			contents->Size = (Vec2i) { .w = 15, .h = 20 };
		} break;
		case RE_CONTENTS_SPECIAL: {
			contents->Size = (Vec2i) { .w = 15, .h = 20 };
		} break;
		case RE_CONTENTS_SET: {
			contents->Size = (Vec2i) { .w = 80, .h = 20 };
		} break;
		case RE_CONTENTS_GROUP: {
			Group* group = contents->Group;
			prepass_Group(group);
			contents->Size = (Vec2i) {
				.w = group->Size.w + 20,
				.h = group->Size.h + 20,
			};
		}
	}
}

void prepass_Group(Group* group) {
	Regex* regex = group->Regex;
	prepass_Regex(regex);

	group->Size = (Vec2i) {
		.w = regex->Size.w + 10,
		.h = regex->Size.h + 10,
	};
}

void drawRailroad_Regex(Regex* regex, Vec2i origin);
void drawRailroad_NoUnionEx(NoUnionEx* ex, Vec2i origin);
void drawRailroad_Unit(Unit* unit, Vec2i origin);
void drawRailroad_UnitContents(UnitContents* contents, Vec2i origin);
void drawRailroad_Group(Group* group, Vec2i origin);

void drawRailroad_Regex(Regex* regex, Vec2i origin) {
	Vec2i memberOrigin = origin;

	for (int i = 0; i < regex->NumUnionMembers; i++) {
		NoUnionEx* member = regex->UnionMembers[i];
		drawRailroad_NoUnionEx(member, memberOrigin);

		memberOrigin.y += UNION_VERTICAL_SPACING + member->Size.h;
	}
}

void drawRailroad_NoUnionEx(NoUnionEx* ex, Vec2i origin) {
	Vec2i unitOrigin = origin;
	for (int i = 0; i < ex->NumUnits; i++) {
		Unit* unit = ex->Units[i];
		drawRailroad_Unit(unit, unitOrigin);

		unitOrigin.x += unit->Size.w;
	}
}

void drawRailroad_Unit(Unit* unit, Vec2i origin) {
	mu_Rect rect = mu_rect(origin.x, origin.y, unit->Size.w, unit->Size.h);
	int isHover = mu_mouse_over(ctx, rect);
	unit->LastRect = rect;

	mu_push_clip_rect(ctx, rect);

	// unit itself
	mu_draw_rect(
		ctx,
		rect,
		mu_color(200, 200, 200, 255)
	);

	int wireY = origin.y + 9;

	// left wire
	mu_draw_rect(
		ctx,
		mu_rect(origin.x, wireY, unit->LeftSpacing, 2),
		COLOR_WIRE
	);
	// right wire
	mu_draw_rect(
		ctx,
		mu_rect(origin.x + unit->LeftSpacing + unit->Contents->Size.x, wireY, unit->LeftSpacing, 2),
		COLOR_WIRE
	);

	const int HANDLE_SIZE = 8;
	int handleY = origin.y + 6;

	mu_Rect leftHandleRect;
	mu_Rect rightHandleRect;

	// left handle
	{
		int wireWidth = (unit->Previous != NULL ? unit->Previous->RightSpacing : 0) + unit->LeftSpacing;
		int handleX = origin.x + wireWidth/2 - HANDLE_SIZE/2;
		leftHandleRect = mu_rect(handleX, handleY, HANDLE_SIZE, HANDLE_SIZE);
		mu_draw_rect(
			ctx,
			leftHandleRect,
			COLOR_WIRE
		);
	}
	// right handle
	{
		int wireWidth = unit->LeftSpacing + (unit->Next != NULL ? unit->Next->LeftSpacing : 0);
		int handleX = origin.x
			+ unit->LeftSpacing
			+ unit->Contents->Size.x
			+ wireWidth/2
			- HANDLE_SIZE/2;
		rightHandleRect = mu_rect(handleX, handleY, HANDLE_SIZE, HANDLE_SIZE);
		mu_draw_rect(
			ctx,
			rightHandleRect,
			COLOR_WIRE
		);
	}

	drawRailroad_UnitContents(unit->Contents, (Vec2i) {
		.x = unit->LeftSpacing + origin.x,
		.y = origin.y,
	});

	mu_pop_clip_rect(ctx);

	int targetWireSpacing = 0;
	if (isHover || drag.OriginUnit == unit) {
		targetWireSpacing = 20;
	}

	int animating = 0;
	unit->LeftSpacing = interp_linear(ctx->dt, unit->LeftSpacing, targetWireSpacing, 160, &animating);
	ctx->animating |= animating;
	unit->RightSpacing = interp_linear(ctx->dt, unit->RightSpacing, targetWireSpacing, 160, &animating);
	ctx->animating |= animating;

	int overLeftHandle = unit->LeftSpacing != 0 && mu_mouse_over(ctx, leftHandleRect);
	int overRightHandle = unit->RightSpacing != 0 && mu_mouse_over(ctx, rightHandleRect);

	if (ctx->mouse_pressed == MU_MOUSE_LEFT && !drag.OriginUnit) {
		// start drag
		if (overLeftHandle) {
			drag = (DragContext) {
				.OriginUnit = unit,
				.HandleBeforeUnit = unit->Previous,
				.HandleAfterUnit = unit,
			};
		} else if (overRightHandle) {
			drag = (DragContext) {
				.OriginUnit = unit,
				.HandleBeforeUnit = unit,
				.HandleAfterUnit = unit->Next,
			};
		}
	} else if (!(ctx->mouse_down & MU_MOUSE_LEFT) && drag.OriginUnit) {
		// drag finished
		ctx->animating = 1;

		// TODO: This should actually just be an early-out optimization
		// for the more general case below. That will prevent
		// unnecessary groups.
		if (drag.OriginUnit == unit) {
			// drag onto this same unit. no group shenanigans!
			if (overRightHandle && drag.HandleAfterUnit == unit) {
				// forward drag, skip
				Unit_SetRepeatMin(unit, 0);
			} else if (overLeftHandle && drag.HandleBeforeUnit == unit) {
				// backward drag, repeat
				Unit_SetRepeatMax(unit, 0);
			} else {
				// must have dropped on the very same handle. do nothing!
			}
		} else {
			// seek back and forth to figure out what the heck kind of drag this is
			Unit* groupStartUnit = NULL;
			Unit* groupEndUnit = NULL;
			int isForward = 0; // vs. backward

			// seek left from the drag origin to find repetition
			for (
				Unit* currentUnit = drag.HandleBeforeUnit;
				currentUnit;
				currentUnit = currentUnit->Previous
			) {
				if (
					(overLeftHandle && currentUnit == unit)
					|| (overRightHandle && currentUnit->Previous == unit)
				) {
					groupStartUnit = currentUnit;
					groupEndUnit = drag.HandleBeforeUnit;
					isForward = 0;
					break;
				}
			}

			// seek right from the drag origin to find skips
			for (
				Unit* currentUnit = drag.HandleAfterUnit;
				currentUnit;
				currentUnit = currentUnit->Next
			) {
				if (
					(overRightHandle && currentUnit == unit)
					|| (overLeftHandle && currentUnit->Next == unit)
				) {
					groupStartUnit = drag.HandleAfterUnit;
					groupEndUnit = currentUnit;
					isForward = 1;
					break;
				}
			}

			// make a group out of it
			if (groupStartUnit) {
				assert(groupEndUnit);

				Unit* newUnit = Unit_init((Unit*) pool_alloc(getRegexPool()));
				newUnit->Contents->Type = RE_CONTENTS_GROUP;

				NoUnionEx* ex = newUnit->Contents->Group->Regex->UnionMembers[0];

				{
					Unit* currentUnit = groupStartUnit;
					ex->NumUnits = 0;
					while (currentUnit) {
						ex->Units[ex->NumUnits] = currentUnit;
						ex->NumUnits++;

						if (currentUnit == groupEndUnit) {
							break;
						}

						currentUnit = currentUnit->Next;
					}
				}

				if (isForward) {
					Unit_SetRepeatMin(newUnit, 0);
				} else {
					Unit_SetRepeatMax(newUnit, 0);
				}

				NoUnionEx* parent = groupStartUnit->Parent;

				newUnit->Parent = parent;
				newUnit->Previous = groupStartUnit->Previous;
				newUnit->Next = groupEndUnit->Next;
				if (newUnit->Previous) {
					newUnit->Previous->Next = newUnit;
				}
				if (newUnit->Next) {
					newUnit->Next->Previous = newUnit;
				}
				groupStartUnit->Previous = NULL;
				groupEndUnit->Next = NULL;

				{
					Unit* currentUnit = newUnit->Previous ? parent->Units[0] : newUnit;
					parent->NumUnits = 0;
					while (currentUnit) {
						parent->Units[parent->NumUnits] = currentUnit;
						parent->NumUnits++;

						currentUnit = currentUnit->Next;
					}
				}
			}
		}
	}
}

void drawRailroad_UnitContents(UnitContents* contents, Vec2i origin) {
	mu_Rect r = mu_rect(origin.x, origin.y, contents->Size.w, contents->Size.h);

	mu_draw_rect(
		ctx,
		r,
		mu_color(150, 150, 150, 255)
	);

	mu_layout_set_next(ctx, r, 0);
	switch (contents->Type) {
		case RE_CONTENTS_LITCHAR: {
			mu_label(ctx, contents->LitChar->_buf);
		} break;
		case RE_CONTENTS_METACHAR: {
			mu_label(ctx, &contents->MetaChar->_backslash);
		} break;
		case RE_CONTENTS_SPECIAL: {
			// TODO: DO IT
		} break;
		case RE_CONTENTS_SET: {
			// TODO: Set
		} break;
		case RE_CONTENTS_GROUP: {
			// TODO: Change the origin?
			drawRailroad_Group(contents->Group, (Vec2i) { .x = origin.x + 10, .y = origin.y + 10 });
		} break;
	}
}

void drawRailroad_Group(Group* group, Vec2i origin) {
	mu_draw_rect(
		ctx,
		mu_rect(origin.x, origin.y, group->Size.w, group->Size.h),
		mu_color(100, 100, 100, 255)
	);

	drawRailroad_Regex(group->Regex, (Vec2i) { .x = origin.x + 5, .y = origin.y + 5 });
}

int frame(float dt) {
	mu_begin(ctx, dt);

	if (mu_begin_window(ctx, "Tree View", mu_rect(10, 10, 500, 800))) {
		doTree(ctx, regex);

		mu_end_window(ctx);
	}

	if (mu_begin_window(ctx, "Final Regex", mu_rect(520, 10, 500, 80))) {
		mu_layout_row(ctx, 1, (int[]) { -1 }, -1);
		mu_label(ctx, ToString(regex));

		mu_end_window(ctx);
	}

	prepass_Regex(regex);
	if (mu_begin_window_ex(ctx, "Test", mu_rect(520, 300, 500, 500), MU_OPT_NOFRAME | MU_OPT_NOTITLE)) {
		drawRailroad_Regex(regex, (Vec2i) { .x = 520, .y = 300 });
		mu_end_window(ctx);
	}

	// reset drag
	if (!(ctx->mouse_down & MU_MOUSE_LEFT)) {
		drag = (DragContext) {
			.OriginUnit = NULL,
			.HandleBeforeUnit = NULL,
			.HandleAfterUnit = NULL,
		};
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

    return ctx->animating;
}
