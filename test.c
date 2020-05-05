#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "microui.h"

extern void canvas_setFillRGB(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
extern void canvas_rect(int x, int y, int w, int h);
extern void canvas_text(char* str, int x, int y);

static int text_width(mu_Font font, const char *text, int len) {
  return strlen(text);
}

static int text_height(mu_Font font) {
  return 18;
}

mu_Context* ctx;

void init() {
	ctx = malloc(sizeof(mu_Context));
	mu_init(ctx);
	ctx->text_width = text_width;
  	ctx->text_height = text_height;
}

void frame() {
	// mu_input_mousemove(ctx, 0, 0);

	mu_begin(ctx);

	if (mu_begin_window(ctx, "My Window", mu_rect(10, 10, 300, 400))) {
		if (mu_button(ctx, "x")) {
			printf("Pressed button!");
		}
		mu_end_window(ctx);
	}

	mu_end(ctx);

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
			// case MU_COMMAND_CLIP: r_set_clip_rect(cmd->clip.rect); break;
		}
    }
}
