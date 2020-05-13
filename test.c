#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "microui.h"

extern void canvas_clear();
extern void canvas_clip(int x, int y, int w, int h);
extern void canvas_setFillRGB(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
extern void canvas_rect(int x, int y, int w, int h);
extern void canvas_text(char* str, int x, int y);

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

	if (mu_begin_window(ctx, "Window A", mu_rect(10, 10, 300, 200))) {
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
			// case MU_COMMAND_TEXT: r_draw_text(cmd->text.str, cmd->text.pos, cmd->text.color); break;
			// case MU_COMMAND_RECT: r_draw_rect(cmd->rect.rect, cmd->rect.color); break;
			// case MU_COMMAND_ICON: r_draw_icon(cmd->icon.id, cmd->icon.rect, cmd->icon.color); break;
			// case MU_COMMAND_CLIP: r_set_clip_rect(cmd->clip.rect); break;
			case MU_COMMAND_TEXT: {
				mu_Vec2 pos = cmd->text.pos;
				mu_Color color = cmd->text.color;
				canvas_setFillRGB(color.r, color.g, color.b, color.a);
				canvas_text(cmd->text.str, pos.x, pos.y);
				break;
			}
			case MU_COMMAND_RECT: {
				mu_Color color = cmd->rect.color;
				mu_Rect rect = cmd->rect.rect;
				canvas_setFillRGB(color.r, color.g, color.b, color.a);
				canvas_rect(rect.x, rect.y, rect.w, rect.h);
				break;
			}
			// case MU_COMMAND_ICON: r_draw_icon(cmd->icon.id, cmd->icon.rect, cmd->icon.color); break;
			case MU_COMMAND_CLIP: {
				mu_Rect rect = cmd->clip.rect;
				canvas_clip(rect.x, rect.y, rect.w, rect.h);
				break;
			}
		}
    }
}
