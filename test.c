#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "microui.h"

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

int imax(int a, int b) {
	if (a > b) {
		return a;
	} else {
		return b;
	}
}

int imin(int a, int b) {
	if (a < b) {
		return a;
	} else {
		return b;
	}
}

// --------------------------------------------------------------------
// --------------------------------------------------------------------

typedef struct DragContext {
	Unit* Unit;
	int StartedWhere;
} DragContext;

enum {
	DRAG_START_BEFORE,
	DRAG_START_AFTER,
};

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
const int UNIT_HORIZONTAL_SPACING = 5;
const int UNIT_HPAD = 16;
const int UNIT_VPAD = 3;

Vec2i computeSize_Regex(Regex* regex);
Vec2i computeSize_NoUnionEx(NoUnionEx* ex);
Vec2i computeSize_Unit(Unit* unit);
Vec2i computeSize_UnitContents(UnitContents* contents);
Vec2i computeSize_Group(Group* group);

Vec2i computeSize_Regex(Regex* regex) {
	int w = 0;
	int h = 0;
	for (int i = 0; i < regex->NumUnionMembers; i++) {
		NoUnionEx* member = regex->UnionMembers[i];
		Vec2i memberSize = computeSize_NoUnionEx(member);

		w = imax(w, memberSize.w);
		h += (i != 0 ? UNION_VERTICAL_SPACING : 0) + memberSize.h;
	}

	regex->Size = (Vec2i) { .w = w, .h = h };

	return regex->Size;
}

Vec2i computeSize_NoUnionEx(NoUnionEx* ex) {
	int w = 0;
	int h = 0;

	for (int j = 0; j < ex->NumUnits; j++) {
		Unit* unit = ex->Units[j];
		Vec2i unitSize = computeSize_Unit(unit);

		w += (j != 0 ? UNIT_HORIZONTAL_SPACING : 0) + unitSize.w;
		h = imax(h, unitSize.h);
	}

	ex->Size = (Vec2i) { .w = w, .h = h };

	return ex->Size;
}

Vec2i computeSize_Unit(Unit* unit) {
	Vec2i contentsSize = computeSize_UnitContents(unit->Contents);
	unit->Size = (Vec2i) { .w = contentsSize.w + UNIT_HPAD * 2, .h = contentsSize.h + UNIT_VPAD * 2 };

	return unit->Size;
}

Vec2i computeSize_UnitContents(UnitContents* contents) {
	switch (contents->Type) {
		case RE_CONTENTS_LITCHAR: {
			contents->Size = (Vec2i) { .w = 20, .h = 20 };
		} break;
		case RE_CONTENTS_METACHAR: {
			contents->Size = (Vec2i) { .w = 20, .h = 20 };
		} break;
		case RE_CONTENTS_SPECIAL: {
			contents->Size = (Vec2i) { .w = 20, .h = 20 };
		} break;
		case RE_CONTENTS_SET: {
			contents->Size = (Vec2i) { .w = 80, .h = 20 };
		} break;
		case RE_CONTENTS_GROUP: {
			Vec2i groupSize = computeSize_Group(contents->Group);
			contents->Size = (Vec2i) { .w = groupSize.w + 20, .h = groupSize.h + 20 };
		}
	}

	return contents->Size;
}

Vec2i computeSize_Group(Group* group) {
	Vec2i regexSize = computeSize_Regex(group->Regex);
	group->Size = (Vec2i) { .w = regexSize.w + 10, .h = regexSize.h + 10};

	return group->Size;
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

		unitOrigin.x += UNIT_HORIZONTAL_SPACING + unit->Size.w;
	}
}

void drawRailroad_Unit(Unit* unit, Vec2i origin) {
	mu_draw_rect(
		ctx,
		mu_rect(origin.x, origin.y, unit->Size.w, unit->Size.h),
		mu_color(200, 200, 200, 255)
	);
	drawRailroad_UnitContents(unit->Contents, (Vec2i) {
		.x = origin.x + UNIT_HPAD,
		.y = origin.y + UNIT_VPAD,
	});

	mu_Rect beforeHandle = mu_rect(origin.x + 6, origin.y + 10, 5, 5);
	mu_Rect afterHandle = mu_rect(origin.x + unit->Size.w - 11, origin.y + 10, 5, 5);
	mu_draw_rect(
		ctx,
		beforeHandle,
		mu_color(100, 100, 100, 255)
	);
	mu_draw_rect(
		ctx,
		afterHandle,
		mu_color(100, 100, 100, 255)
	);

	if (ctx->mouse_pressed == MU_MOUSE_LEFT && !drag.Unit) {
		// start drag
		if (mu_mouse_over(ctx, beforeHandle)) {
			drag = (DragContext) {
				.Unit = unit,
				.StartedWhere = DRAG_START_BEFORE,
			};
		} else if (mu_mouse_over(ctx, afterHandle)) {
			drag = (DragContext) {
				.Unit = unit,
				.StartedWhere = DRAG_START_AFTER,
			};
		}
	} else if (!(ctx->mouse_down & MU_MOUSE_LEFT) && drag.Unit) {
		// drag finished
		if (unit == drag.Unit) {
			if (mu_mouse_over(ctx, beforeHandle) && drag.StartedWhere == DRAG_START_AFTER) {
				// after -> before, so repeat
				Unit_SetRepeatMax(unit, 0);
			} else if (mu_mouse_over(ctx, afterHandle) && drag.StartedWhere == DRAG_START_BEFORE) {
				// before -> after, so skip
				Unit_SetRepeatMin(unit, 0);
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

void frame() {
	mu_begin(ctx);

	if (mu_begin_window(ctx, "Tree View", mu_rect(10, 10, 500, 800))) {
		doTree(ctx, regex);

		mu_end_window(ctx);
	}

	if (mu_begin_window(ctx, "Final Regex", mu_rect(520, 10, 500, 80))) {
		mu_layout_row(ctx, 1, (int[]) { -1 }, -1);
		mu_label(ctx, ToString(regex));

		mu_end_window(ctx);
	}

	computeSize_Regex(regex);
	if (mu_begin_window_ex(ctx, "Test", mu_rect(520, 300, 500, 500), MU_OPT_NOFRAME | MU_OPT_NOTITLE)) {
		drawRailroad_Regex(regex, (Vec2i) { .x = 520, .y = 300 });
		mu_end_window(ctx);
	}

	// reset drag
	if (!(ctx->mouse_down & MU_MOUSE_LEFT)) {
		drag = (DragContext) {
			.Unit = NULL,
			.StartedWhere = 0,
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
}
