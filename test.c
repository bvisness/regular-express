#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "microui.h"
#include "regex/regex.h"

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

void init(int textHeight) {
	_textHeight = textHeight;

	Special start = { .Type = RE_SPECIAL_STRINGSTART };
	Char a = { .Type = RE_CHAR_LITERAL, .Literal = 'a' };
	Char b = { .Type = RE_CHAR_LITERAL, .Literal = 'b' };
	Char z = { .Type = RE_CHAR_LITERAL, .Literal = 'z' };
	Char s = { .Type = RE_CHAR_META, .Meta = 's' };
	SetItemRange az = { .Min = &a, .Max = &z };
	SetItem azItem = { .Type = RE_SETITEM_RANGE, .Range = &az };
	SetItem bItem = { .Type = RE_SETITEM_CHAR, .Char = &b };
	Set pos = { .NumItems = 2, .Items[0] = &azItem, .Items[1] = &bItem, .IsNegative = 0 };
	Set neg = { .NumItems = 1, .Items[0] = &azItem, .IsNegative = 1 };
	UnitRepetition plus = { .Min = 1, .Max = -1 };
	UnitRepetition star = { .Min = 0, .Max = -1 };
	UnitContents aContents = { .Type = RE_CONTENTS_CHAR, .Char = &b };
	UnitContents sContents = { .Type = RE_CONTENTS_CHAR, .Char = &s };
	UnitContents startContents = { .Type = RE_CONTENTS_SPECIAL, .Special = &start };
	UnitContents posContents = { .Type = RE_CONTENTS_SET, .Set = &pos };
	UnitContents negContents = { .Type = RE_CONTENTS_SET, .Set = &neg };
	Unit u1 = { .Contents = &aContents, .Repetition = NULL };
	Unit u2 = { .Contents = &sContents, .Repetition = &plus };
	Unit u3 = { .Contents = &posContents, .Repetition = NULL };
	Unit u4 = { .Contents = &negContents, .Repetition = &star };
	NoUnionEx nex = {
		.NumUnits = 4,
		.Units[0] = &u1,
		.Units[1] = &u2,
		.Units[2] = &u3,
		.Units[3] = &u4,
	};
	Regex regex = { .NumUnionMembers = 1, .UnionMembers[0] = &nex };

	printString(ToString(&regex));

	ctx = malloc(sizeof(mu_Context));
	mu_init(ctx);
	ctx->text_width = text_width;
  	ctx->text_height = text_height;
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

static  char logbuf[32000];
static   int logbuf_updated = 0;

void frame() {
	mu_begin(ctx);

	if (mu_begin_window_ex(ctx, "Window A", mu_rect(10, 10, 300, 200), MU_OPT_NOTITLE)) {
		mu_button(ctx, "YO");

		static float value;
		mu_slider_ex(ctx, &value, 0, 20, 0, "(%.2f)", MU_OPT_ALIGNCENTER);

		mu_Container* cont = mu_get_current_container(ctx);
		mu_layout_set_next(ctx, mu_rect(cont->rect.x + cont->rect.w - 20, cont->rect.y + 30, 40, 40), 0);
		mu_draw_circle(ctx, cont->rect.x + cont->rect.w, cont->rect.y + 30, 10.0f, mu_color(100, 100, 100, 255));
		// mu_button(ctx, "OVER HERE");

		mu_end_window(ctx);
	}

	if (mu_begin_window(ctx, "Window B", mu_rect(400, 10, 300, 200))) {
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
