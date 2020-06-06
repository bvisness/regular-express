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

// Utilities

void draw_arbitrary_text(mu_Context* ctx, const char* str, mu_Vec2 pos, mu_Color color) {
	mu_draw_text(ctx, NULL, str, -1, pos, color);
}

// --------------------------------------------------------------------
// --------------------------------------------------------------------

typedef struct DragContext {
	Unit* OriginUnit;

	Unit* UnitBeforeHandle;
	Unit* UnitAfterHandle;
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

const int UNION_VERTICAL_SPACING = 0;
const int UNION_GUTTER_WIDTH = 16;
const int NOUNIONEX_MIN_HEIGHT = 20;
const int UNIT_HANDLE_ZONE_WIDTH = 16;
const int UNIT_WIRE_ATTACHMENT_ZONE_WIDTH = 12;
const int UNIT_REPEAT_WIRE_ZONE_HEIGHT = 15;
const int UNIT_REPEAT_WIRE_MARGIN = 5;
const int UNIT_REPEAT_WIRE_SCOOT = 2;
const int UNIT_CONTENTS_MIN_HEIGHT = 20;
const int WIRE_THICKNESS = 2;
const int GROUP_VERTICAL_PADDING = 0;

const mu_Color COLOR_RE_TEXT = (mu_Color) { 0, 0, 0, 255 };
const mu_Color COLOR_WIRE = (mu_Color) { 50, 50, 50, 255 };

void prepass_Regex(Regex* regex);
void prepass_NoUnionEx(NoUnionEx* ex);
void prepass_Unit(Unit* unit);
void prepass_UnitContents(UnitContents* contents);
void prepass_Group(Group* group);

void prepass_Regex(Regex* regex) {
	int w = 0;
	int h = 0;
	int wireHeight = 0;

	for (int i = 0; i < regex->NumUnionMembers; i++) {
		NoUnionEx* member = regex->UnionMembers[i];
		prepass_NoUnionEx(member);

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
		.h = regex->UnionSize.h,
	};
	regex->WireHeight = wireHeight;
}

void prepass_NoUnionEx(NoUnionEx* ex) {
	int w = 0;
	int h = NOUNIONEX_MIN_HEIGHT;
	int wireHeight = NOUNIONEX_MIN_HEIGHT/2;

	for (int j = 0; j < ex->NumUnits; j++) {
		Unit* unit = ex->Units[j];
		prepass_Unit(unit);

		w += unit->Size.w;
		h = imax(h, unit->Size.h);
		wireHeight = imax(wireHeight, unit->WireHeight);

		unit->Parent = ex;
		unit->Previous = (j == 0) ? NULL : ex->Units[j-1];
		unit->Next = (j == ex->NumUnits - 1) ? NULL : ex->Units[j+1];
	}

	ex->Size = (Vec2i) { .w = w, .h = h };
	ex->WireHeight = wireHeight;
}

void prepass_Unit(Unit* unit) {
	UnitContents* contents = unit->Contents;
	prepass_UnitContents(contents);

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

void prepass_UnitContents(UnitContents* contents) {
	switch (contents->Type) {
		case RE_CONTENTS_LITCHAR: {
			contents->Size = (Vec2i) { .w = 15, .h = UNIT_CONTENTS_MIN_HEIGHT };
			contents->WireHeight = UNIT_CONTENTS_MIN_HEIGHT/2;
		} break;
		case RE_CONTENTS_METACHAR: {
			contents->Size = (Vec2i) { .w = 25, .h = UNIT_CONTENTS_MIN_HEIGHT };
			contents->WireHeight = UNIT_CONTENTS_MIN_HEIGHT/2;
		} break;
		case RE_CONTENTS_SPECIAL: {
			contents->Size = (Vec2i) { .w = 15, .h = UNIT_CONTENTS_MIN_HEIGHT };
			contents->WireHeight = UNIT_CONTENTS_MIN_HEIGHT/2;
		} break;
		case RE_CONTENTS_SET: {
			contents->Size = (Vec2i) { .w = 80, .h = UNIT_CONTENTS_MIN_HEIGHT };
			contents->WireHeight = UNIT_CONTENTS_MIN_HEIGHT/2;
		} break;
		case RE_CONTENTS_GROUP: {
			Group* group = contents->Group;
			prepass_Group(group);
			contents->Size = group->Size;
			contents->WireHeight = group->WireHeight;
		}
	}
}

void prepass_Group(Group* group) {
	Regex* regex = group->Regex;
	prepass_Regex(regex);

	group->Size = (Vec2i) {
		.w = regex->Size.w,
		.h = GROUP_VERTICAL_PADDING + regex->Size.h + GROUP_VERTICAL_PADDING,
	};
	group->WireHeight = GROUP_VERTICAL_PADDING + regex->WireHeight;
}

void drawRailroad_Regex(Regex* regex, Vec2i origin);
void drawRailroad_NoUnionEx(NoUnionEx* ex, Vec2i origin);
void drawRailroad_Unit(Unit* unit, Vec2i origin);
void drawRailroad_UnitContents(UnitContents* contents, Vec2i origin);
void drawRailroad_Group(Group* group, Vec2i origin);

void drawRailroad_Regex(Regex* regex, Vec2i origin) {
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
		drawRailroad_NoUnionEx(member, memberOrigin);

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
}

void drawRailroad_NoUnionEx(NoUnionEx* ex, Vec2i origin) {
	int unitX = origin.x;
	for (int i = 0; i < ex->NumUnits; i++) {
		Unit* unit = ex->Units[i];
		drawRailroad_Unit(unit, (Vec2i) {
			.x = unitX,
			.y = origin.y + ex->WireHeight - unit->WireHeight,
		});

		unitX += unit->Size.w;
	}
}

void drawRailroad_Unit(Unit* unit, Vec2i origin) {
	mu_Rect rect = mu_rect(origin.x, origin.y, unit->Size.w, unit->Size.h);
	int isHover = mu_mouse_over(ctx, rect);
	int isDragOrigin = (drag.OriginUnit == unit);
	int nonSingular = Unit_IsNonSingular(unit);

	// save some of 'em for the next pass
	unit->LastRect = rect;
	unit->IsHover = isHover;
	unit->IsDragOrigin = isDragOrigin;

	mu_push_clip_rect(ctx, rect);

	int middleY = origin.y + UNIT_REPEAT_WIRE_ZONE_HEIGHT + unit->Contents->WireHeight;

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
				+ unit->Contents->Size.x,
			middleY - WIRE_THICKNESS/2,
			(nonSingular ? UNIT_WIRE_ATTACHMENT_ZONE_WIDTH : 0) + unit->RightHandleZoneWidth,
			WIRE_THICKNESS
		),
		COLOR_WIRE
	);

	const int HANDLE_SIZE = 8;
	int handleY = middleY - HANDLE_SIZE/2;

	int overLeftHandle = 0;
	int overRightHandle = 0;

	// left handle
	if (Unit_ShouldShowLeftHandle(unit)) {
		int handleX = origin.x + unit->LeftHandleZoneWidth/2 - HANDLE_SIZE/2;
		mu_Rect handleRect = mu_rect(handleX, handleY, HANDLE_SIZE, HANDLE_SIZE);
		mu_draw_rect(ctx, handleRect, COLOR_WIRE);
		overLeftHandle = mu_mouse_over(ctx, handleRect);
	}
	// right handle
	if (Unit_ShouldShowRightHandle(unit)) {
		int handleX = origin.x
			+ unit->LeftHandleZoneWidth
			+ (nonSingular ? UNIT_WIRE_ATTACHMENT_ZONE_WIDTH : 0)
			+ unit->Contents->Size.x
			+ (nonSingular ? UNIT_WIRE_ATTACHMENT_ZONE_WIDTH : 0)
			+ unit->RightHandleZoneWidth/2
			- HANDLE_SIZE/2;
		mu_Rect handleRect = mu_rect(handleX, handleY, HANDLE_SIZE, HANDLE_SIZE);
		mu_draw_rect(ctx, handleRect, COLOR_WIRE);
		overRightHandle = mu_mouse_over(ctx, handleRect);
	}

	int leftWireX = origin.x
		+ unit->LeftHandleZoneWidth
		+ UNIT_WIRE_ATTACHMENT_ZONE_WIDTH/2
		- WIRE_THICKNESS/2;
	int rightWireX = origin.x
		+ unit->LeftHandleZoneWidth
		+ UNIT_WIRE_ATTACHMENT_ZONE_WIDTH
		+ unit->Contents->Size.x
		+ UNIT_WIRE_ATTACHMENT_ZONE_WIDTH/2
		- WIRE_THICKNESS/2;

	int scoot = (Unit_IsSkip(unit) && Unit_IsRepeat(unit) ? UNIT_REPEAT_WIRE_SCOOT : 0);

	if (unit->RepeatMin < 1) {
		// draw the skip wire
		int skipWireY = middleY
			- unit->Contents->WireHeight
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
			- unit->Contents->WireHeight
			+ unit->Contents->Size.y
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

	drawRailroad_UnitContents(unit->Contents, (Vec2i) {
		.x = origin.x + unit->LeftHandleZoneWidth + (nonSingular ? UNIT_WIRE_ATTACHMENT_ZONE_WIDTH : 0),
		.y = origin.y + UNIT_REPEAT_WIRE_ZONE_HEIGHT,
	});

	mu_pop_clip_rect(ctx);

	int targetLeftHandleZoneWidth = Unit_ShouldShowLeftHandle(unit) ? UNIT_HANDLE_ZONE_WIDTH : 0;
	int targetRightHandleZoneWidth = Unit_ShouldShowRightHandle(unit) ? UNIT_HANDLE_ZONE_WIDTH : 0;

	int animating = 0;
	unit->LeftHandleZoneWidth = interp_linear(ctx->dt, unit->LeftHandleZoneWidth, targetLeftHandleZoneWidth, 160, &animating);
	ctx->animating |= animating;
	unit->RightHandleZoneWidth = interp_linear(ctx->dt, unit->RightHandleZoneWidth, targetRightHandleZoneWidth, 160, &animating);
	ctx->animating |= animating;

	if (ctx->mouse_pressed == MU_MOUSE_LEFT && !drag.OriginUnit) {
		// start drag
		if (overLeftHandle) {
			drag = (DragContext) {
				.OriginUnit = unit,
				.UnitBeforeHandle = unit->Previous,
				.UnitAfterHandle = unit,
			};
		} else if (overRightHandle) {
			drag = (DragContext) {
				.OriginUnit = unit,
				.UnitBeforeHandle = unit,
				.UnitAfterHandle = unit->Next,
			};
		}
	} else if (!(ctx->mouse_down & MU_MOUSE_LEFT) && drag.OriginUnit) {
		// drag finished
		ctx->animating = 1;

		// seek back and forth to figure out what the heck kind of drag this is
		Unit* groupStartUnit = NULL;
		Unit* groupEndUnit = NULL;
		int isForward = 0; // vs. backward

		// seek left from the drag origin to find repetition
		for (
			Unit* visitingUnit = drag.UnitBeforeHandle;
			visitingUnit;
			visitingUnit = visitingUnit->Previous
		) {
			if (
				(unit == visitingUnit && overLeftHandle)
				|| (unit->Next == visitingUnit && overRightHandle)
			) {
				groupStartUnit = visitingUnit;
				groupEndUnit = drag.UnitBeforeHandle;
				isForward = 0;
				break;
			}
		}

		// seek right from the drag origin to find skips
		for (
			Unit* visitingUnit = drag.UnitAfterHandle;
			visitingUnit;
			visitingUnit = visitingUnit->Next
		) {
			if (
				(unit == visitingUnit && overRightHandle)
				|| (unit->Previous == visitingUnit && overLeftHandle)
			) {
				groupStartUnit = drag.UnitAfterHandle;
				groupEndUnit = visitingUnit;
				isForward = 1;
				break;
			}
		}

		if (groupStartUnit) {
			assert(groupEndUnit);

			if (groupStartUnit == groupEndUnit) {
				// drag onto this same unit. no group shenanigans!
				if (isForward) {
					// forward drag, skip
					Unit_SetRepeatMin(groupStartUnit, 0);
				} else {
					// backward drag, repeat
					Unit_SetRepeatMax(groupStartUnit, 0);
				}
			} else {
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

	mu_layout_set_next(ctx, r, 0);
	switch (contents->Type) {
		case RE_CONTENTS_LITCHAR: {
			mu_draw_rect(
				ctx,
				r,
				mu_color(200, 200, 200, 255)
			);

			char* str = contents->LitChar->_buf;
			mu_Vec2 pos = mu_position_text(ctx, str, mu_layout_next(ctx), NULL, 0);
			draw_arbitrary_text(ctx, str, pos, COLOR_RE_TEXT);
		} break;
		case RE_CONTENTS_METACHAR: {
			mu_draw_rect(
				ctx,
				r,
				mu_color(200, 200, 200, 255)
			);

			char* str = &contents->MetaChar->_backslash;
			mu_Vec2 pos = mu_position_text(ctx, str, mu_layout_next(ctx), NULL, 0);
			draw_arbitrary_text(ctx, str, pos, COLOR_RE_TEXT);
		} break;
		case RE_CONTENTS_SPECIAL: {
			// TODO: DO IT
			mu_draw_rect(
				ctx,
				r,
				mu_color(200, 200, 200, 255)
			);
		} break;
		case RE_CONTENTS_SET: {
			// TODO: Set
			mu_draw_rect(
				ctx,
				r,
				mu_color(200, 200, 200, 255)
			);
		} break;
		case RE_CONTENTS_GROUP: {
			drawRailroad_Group(contents->Group, origin);
		} break;
	}
}

void drawRailroad_Group(Group* group, Vec2i origin) {
	mu_draw_rect(
		ctx,
		mu_rect(origin.x, origin.y, group->Size.w, group->Size.h),
		mu_color(0, 0, 0, 25)
	);

	drawRailroad_Regex(group->Regex, (Vec2i) {
		.x = origin.x,
		.y = GROUP_VERTICAL_PADDING + origin.y,
	});
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
			.UnitBeforeHandle = NULL,
			.UnitAfterHandle = NULL,
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
